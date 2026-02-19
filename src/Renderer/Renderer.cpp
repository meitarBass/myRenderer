#include "Renderer.h"
#include "../Shaders/PhongShader.h"
#include "../Shaders/DepthShader.h"
#include "../Utils/ThreadPool.h"
#include <iostream>
#include <thread>

void Renderer::render(const Scene& scene, RenderBuffers& target) {
    const Matrix4f4 lightView = Matrix4f4::lookat(scene.lightPos, scene.camera.lookAt, scene.camera.up);
    const Matrix4f4 lightProj = Matrix4f4::projection(LIGHT_PROJECTION_SIZE);
    const Matrix4f4 lightProjView = lightProj * lightView;

    target.reset();
    // --- STEP 1: Shadow Pass ---
    std::cout << "1. Running Shadow Pass..." << std::endl;
    runShadowPass(scene, target, lightProjView);

    // --- STEP 2: Fill Z-Buffer (Crucial for SSAO) ---
    std::cout << "2. Filling Depth Buffer..." << std::endl;
    runColorPass(scene, target, lightProjView);

    // --- STEP 3: Apply SSAO ---
    std::cout << "3. Applying SSAO..." << std::endl;
    applySSAO(target);
}

void Renderer::runShadowPass(const Scene& scene,
                             RenderBuffers &target,
                             const Matrix4f4& lightProjView) {

    const Matrix4f4 lightViewport = Matrix4f4::viewport(0, 0, target.shadowW, target.shadowH);

    for (const auto& object : scene.models) {
        Uniforms depthUniforms;
        depthUniforms.projection = Matrix4f4::identity();
        depthUniforms.viewport = lightViewport;

        Matrix4f4 modelMat = object.getModelMatrix();
        depthUniforms.modelView = lightProjView * modelMat;

        DepthShader depthShader(depthUniforms);

        const RenderContext ctx = { object.model, target.shadowMap,
                          nullptr, nullptr,
                              target.shadowW, target.shadowH };
        drawModel(ctx, depthShader);
    }
}

void Renderer::runColorPass(const Scene& scene,
                            RenderBuffers& target,
                            const Matrix4f4& lightProjView) {

    const Matrix4f4 view = Matrix4f4::lookat(scene.camera.pos, scene.camera.lookAt, scene.camera.up);
    const Matrix4f4 projection = Matrix4f4::projection(scene.camera.focalLength);
    const Matrix4f4 viewport = Matrix4f4::viewport(0, 0, target.width, target.height);

    for (const auto& object : scene.models) {
        Uniforms uniforms;

        uniforms.model = object.getModelMatrix();
        uniforms.modelView = view * uniforms.model;
        uniforms.projection = projection;
        uniforms.viewport = viewport;

        uniforms.lightDir = scene.lightDir;
        uniforms.lightProjView = lightProjView;
        uniforms.shadowMap = &target.shadowMap;
        uniforms.shadowWidth = target.shadowW;
        uniforms.shadowHeight = target.shadowH;

        uniforms.normalMatrix = uniforms.model.inverseTranspose3x3();
        uniforms.cameraPos = scene.camera.pos;

        PhongShader shader(object.diffuse, object.normal, object.specular, uniforms, object.useAlphaTest);

        RenderContext ctx = { object.model, target.zbuffer,
                              &target.framebuffer, &target.normalBuffer,
                              target.width, target.height };
        drawModel(ctx, shader);
    }
}

float linearInterpolation(float a, float b, float f) {
    return a + f * (b - a);
}

void Renderer::applySSAO(RenderBuffers& target) {
    const int width = target.width;
    const int height = target.height;

    std::uint8_t* rawFB = target.framebuffer.buffer();

    static std::vector<Vec2f> kernel;
    static std::vector<Vec2f> noise;
    if (kernel.empty()) {
        for (int i = 0; i < SSAO_RANDOM_PIXEL_SAMPLES; ++i) {
            Vec2f sample(randf() * 2.0f - 1.0f, randf() * 2.0f - 1.0f);
            sample = sample.normalize() * (0.1f + 0.9f * (float)i / SSAO_RANDOM_PIXEL_SAMPLES);
            kernel.push_back(sample);
        }
        for (int i = 0; i < SSAO_RANDOM_PIXEL_SAMPLES; i++) {
            noise.push_back(Vec2f(randf() * 2.0f - 1.0f, randf() * 2.0f - 1.0f).normalize());
        }
    }

    const unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
    const int rowsPerThread = height / numThreads;

    for (unsigned int t = 0; t < numThreads; ++t) {
        ThreadPool::instance().enqueue([&, t]() {
            int startY = t * rowsPerThread;
              int endY = (t == numThreads - 1) ? height : (startY + rowsPerThread);

              for (int y = startY; y < endY; y++) {
                  for (int x = 0; x < width; x++) {
                      const int idx = x + y * width;
                      const float currentZ = target.zbuffer[idx];

                      if (currentZ <= -std::numeric_limits<float>::max() + SSAO_BACKGROUND_THRESHOLD) continue;

                      float occlusion = 0.0f;
                      Vec2f rot = noise[(x % 4) + (y % 4) * 4];

                      for (int i = 0; i < SSAO_RANDOM_PIXEL_SAMPLES; i++) {
                          float rx = kernel[i].x() * rot.x() - kernel[i].y() * rot.y();
                          float ry = kernel[i].x() * rot.y() + kernel[i].y() * rot.x();

                          int sx = x + static_cast<int>(rx * SSAO_SAMPLE_RADIUS);
                          int sy = y + static_cast<int>(ry * SSAO_SAMPLE_RADIUS);

                          if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
                              const float sampleZ = target.zbuffer[sx + sy * width];

                              if (sampleZ > currentZ + SSAO_BIAS) {
                                  const float dist = std::abs(currentZ - sampleZ);
                                  if (dist < SSAO_MAX_OCCLUSION_DISTANCE) {
                                      occlusion += 1.0f;
                                  }
                              }
                          }
                      }

                      const float intensity = 1.0f - std::min(1.0f, (occlusion / SSAO_RANDOM_PIXEL_SAMPLES) * SSAO_STRENGTH);

                      const int offset = idx * 3;
                      rawFB[offset]     = static_cast<uint8_t>(rawFB[offset]     * intensity);
                      rawFB[offset + 1] = static_cast<uint8_t>(rawFB[offset + 1] * intensity);
                      rawFB[offset + 2] = static_cast<uint8_t>(rawFB[offset + 2] * intensity);
                  }
              }
        });
    }

    ThreadPool::instance().waitFinished();
}
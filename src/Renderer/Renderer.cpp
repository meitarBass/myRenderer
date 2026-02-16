#include "Renderer.h"
#include "../Shaders/PhongShader.h"
#include "../Shaders/DepthShader.h"
#include <iostream>
#include <thread>

void Renderer::render(const Scene& scene, RenderBuffers& target) {
    const Matrix4f4 lightView = Matrix4f4::lookat(scene.lightPos, scene.camera.lookAt, scene.camera.up);
    const Matrix4f4 lightProj = Matrix4f4::projection(3.0f);

    const Matrix4f4 lightProjView = lightProj * lightView;

    // --- STEP 1: Shadow Pass ---
    std::cout << "1. Running Shadow Pass..." << std::endl;
    std::vector<float> shadowMap(2048 * 2048, -std::numeric_limits<float>::max());
    runShadowPass(scene, shadowMap, lightProjView);

    // --- STEP 2: Color Pass -
    std::cout << "Running Color Pass..." << std::endl;
    runColorPass(scene, target, shadowMap, lightProjView);

    // --- STEP 3: SSAO & Post Processing ---
    std::cout << "3. Applying SSAO..." << std::endl;
    applySSAO(target);
}

void Renderer::runShadowPass(const Scene& scene,
                             std::vector<float>& shadowMap,
                             const Matrix4f4& lightProjView) {
    constexpr int shadowW = 2048;
    constexpr int shadowH = 2048;

    const Matrix4f4 lightViewport = Matrix4f4::viewport(0, 0, shadowW, shadowH);

    for (const auto& object : scene.models) {
        Uniforms depthUniforms;
        depthUniforms.projection = Matrix4f4::identity();
        depthUniforms.viewport = lightViewport;

        Matrix4f4 modelMat = object.getModelMatrix();
        depthUniforms.modelView = lightProjView * modelMat;

        DepthShader depthShader(depthUniforms);

        RenderContext ctx = { object.model, shadowMap, nullptr, nullptr, shadowW, shadowH };
        drawModel(ctx, depthShader);
    }
}

void Renderer::runColorPass(const Scene& scene,
                            RenderBuffers& target,
                            const std::vector<float>& shadowMap,
                            const Matrix4f4& lightProjView) {

    const int width = target.framebuffer.width();
    const int height = target.framebuffer.height();

    const Matrix4f4 view = Matrix4f4::lookat(scene.camera.pos, scene.camera.lookAt, scene.camera.up);
    const Matrix4f4 projection = Matrix4f4::projection(scene.camera.focalLength);
    const Matrix4f4 viewport = Matrix4f4::viewport(0, 0, width, height);

    for (const auto& object : scene.models) {
        Uniforms uniforms;

        uniforms.model = object.getModelMatrix();
        uniforms.modelView = view * uniforms.model;
        uniforms.projection = projection;
        uniforms.viewport = viewport;

        uniforms.lightDir = scene.lightDir;
        uniforms.lightProjView = lightProjView;
        uniforms.shadowMap = &shadowMap;
        uniforms.shadowWidth = 2048;
        uniforms.shadowHeight = 2048;

        uniforms.normalMatrix = uniforms.model.inverseTranspose3x3();
        uniforms.cameraPos = scene.camera.pos;

        PhongShader shader(object.diffuse, object.normal, object.specular, uniforms, object.useAlphaTest);

        RenderContext ctx = { object.model, target.zbuffer, &target.framebuffer, &target.normalBuffer, width, height };
        drawModel(ctx, shader);
    }
}

float linearInterpolation(float a, float b, float f) {
    return a + f * (b - a);
}

inline void Renderer::processPixelSSAO(int x, int y, int width, int height,
                                       const std::vector<float>& zbuffer,
                                       TGAImage& framebuffer,
                                       const std::vector<Vec2f>& kernel,
                                       const std::vector<Vec2f>& noise) {
    const int idx = x + y * width;
    const float currentZ = zbuffer[idx];

    if (currentZ < -10000.0f) return;

    constexpr float strength = 2.0f;
    constexpr float bias = 0.12f;
    constexpr int kernelSizePixels = 10;
    constexpr int pixelSamples = 16;

    float occlusion = 0.0f;
    int noiseIdx = (x % 4) + (y % 4) * 4;
    Vec2f rotation = noise[noiseIdx];

    float cosTheta = rotation.x();
    float sinTheta = rotation.y();

    for (int i = 0; i < pixelSamples; i++) {
        Vec2f k = kernel[i];
        Vec2f rotatedSample;
        rotatedSample.x() = k.x() * cosTheta - k.y() * sinTheta;
        rotatedSample.y() = k.x() * sinTheta + k.y() * cosTheta;

        int sampleX = x + static_cast<int>(rotatedSample.x() * kernelSizePixels);
        int sampleY = y + static_cast<int>(rotatedSample.y() * kernelSizePixels);

        if (sampleX >= 0 && sampleX < width && sampleY >= 0 && sampleY < height) {
            float sampleZ = zbuffer[sampleX + sampleY * width];
            if (sampleZ > currentZ + bias) {
                float rangeCheck = std::abs(currentZ - sampleZ) < 50.0f ? 1.0f : 0.0f;
                occlusion += 1.0f * rangeCheck;
            }
        }
    }

    occlusion = (occlusion / pixelSamples) * strength;
    if (occlusion > 1.0f) occlusion = 1.0f;
    float intensity = 1.0f - occlusion;

    TGAColor c = framebuffer.get(x, y);
    for (int i = 0; i < 3; i++) {
        c[i] = static_cast<unsigned char>(c[i] * intensity);
    }
    framebuffer.set(x, y, c);
}

void Renderer::applySSAO(RenderBuffers& target) {
    const int width = target.framebuffer.width();
    const int height = target.framebuffer.height();
    std::uint8_t* rawFB = target.framebuffer.buffer();

    static std::vector<Vec2f> kernel;
    static std::vector<Vec2f> noise;
    if (kernel.empty()) {
        for (int i = 0; i < 16; ++i) {
            Vec2f sample(randf() * 2.0f - 1.0f, randf() * 2.0f - 1.0f);
            sample = sample.normalize();
            float scale = (float)i / 16.0f;
            scale = 0.1f + (scale * scale) * (1.0f - 0.1f);
            kernel.push_back(sample * scale);
        }
        for (int i = 0; i < 16; i++) {
            noise.push_back(Vec2f(randf() * 2.0f - 1.0f, randf() * 2.0f - 1.0f).normalize());
        }
    }

    unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
    std::vector<std::thread> threads;
    threads.reserve(numThreads);
    int rowsPerThread = height / numThreads;

    for (unsigned int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&, t]() {
            int startY = t * rowsPerThread;
            int endY = (t == numThreads - 1) ? height : (t + 1) * rowsPerThread;

            for (int y = startY; y < endY; y++) {
                for (int x = 0; x < width; x++) {
                    const int idx = x + y * width;
                    const float currentZ = target.zbuffer[idx];
                    if (currentZ < -10000.0f) continue;

                    float occlusion = 0.0f;
                    Vec2f rot = noise[(x % 4) + (y % 4) * 4];
                    for (int i = 0; i < 16; i++) {
                        Vec2f k = kernel[i];
                        float rx = k.x() * rot.x() - k.y() * rot.y();
                        float ry = k.x() * rot.y() + k.y() * rot.x();
                        int sx = x + (int)(rx * 10.0f);
                        int sy = y + (int)(ry * 10.0f);
                        if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
                            float sampleZ = target.zbuffer[sx + sy * width];
                            if (sampleZ > currentZ + 0.12f && std::abs(currentZ - sampleZ) < 50.0f) occlusion += 1.0f;
                        }
                    }

                    float intensity = 1.0f - std::min(1.0f, (occlusion / 16.0f) * 2.0f);
                    int offset = idx * 3;
                    rawFB[offset]     = (std::uint8_t)(rawFB[offset]     * intensity);
                    rawFB[offset + 1] = (std::uint8_t)(rawFB[offset + 1] * intensity);
                    rawFB[offset + 2] = (std::uint8_t)(rawFB[offset + 2] * intensity);
                }
            }
        });
    }
    for (auto& worker : threads) worker.join();
}
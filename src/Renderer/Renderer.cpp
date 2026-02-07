#include "Renderer.h"
#include "../Shaders/PhongShader.h"
#include "../Shaders/DepthShader.h"
#include <iostream>

void Renderer::render(const Scene& scene, RenderBuffers& target) {
    const Vec3f lightPos = Vec3f(1, 1, 1).normalize() * 3.0f;
    const Matrix4f4 lightView = Matrix4f4::lookat(lightPos, scene.camera.lookAt, scene.camera.up);
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

    TGAImage dummyImage(shadowW, shadowH, TGAImage::RGB);
    const Matrix4f4 lightViewport = Matrix4f4::viewport(0, 0, shadowW, shadowH);

    for (const auto& object : scene.models) {
        Uniforms depthUniforms;
        depthUniforms.projection = Matrix4f4::identity();
        depthUniforms.viewport = lightViewport;

        Matrix4f4 modelMat = object.getModelMatrix();
        depthUniforms.modelView = lightProjView * modelMat;

        DepthShader depthShader(depthUniforms);

        RenderContext ctx = { object.model, dummyImage, shadowMap, nullptr };
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

        RenderContext ctx = { object.model, target.framebuffer, target.zbuffer, &target.normalBuffer };
        drawModel(ctx, shader);
    }
}

void Renderer::applySSAO(RenderBuffers& target) {
    const int width = target.framebuffer.width();
    const int height = target.framebuffer.height();

    const auto ssaoMap = computeSSAO(target.zbuffer, target.normalBuffer, width, height);

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            TGAColor c = target.framebuffer.get(x, y);
            const float intensity = ssaoMap[x + y * width];
            for (int i = 0; i < 3; i++) {
                c[i] = static_cast<unsigned char>(c[i] * intensity);
            }
            target.framebuffer.set(x, y, c);
        }
    }
}

float linearInterpolation(float a, float b, float f) {
    return a + f * (b - a);
}

std::vector<float> Renderer::computeSSAO(const std::vector<float>& zbuffer,
                                         const std::vector<Vec3f>& normalBuffer,
                                         int width, int height) {
    std::vector<float> occlusionBuffer(width * height, 0.0f);

    static std::vector<Vec2f> kernel;
    static std::vector<Vec2f> noise;
    constexpr int kernelSizePixels = 10;
    constexpr int pixelSamples = 16;

    if (kernel.empty()) {
        std::cout << "Initializing SSAO Kernel..." << std::endl;
        for (int i = 0; i < pixelSamples; ++i) {
            Vec2f sample(randf() * 2.0f - 1.0f, randf() * 2.0f - 1.0f);
            sample = sample.normalize();

            float scale = float(i) / float(pixelSamples);
            scale = linearInterpolation(0.1f, 1.0f, scale * scale);
            sample = sample * scale;

            kernel.push_back(sample);
        }

        for (int i = 0; i < 16; i++) {
            Vec2f rot(randf() * 2.0f - 1.0f, randf() * 2.0f - 1.0f);
            noise.push_back(rot.normalize());
        }
    }

    constexpr float strength = 2.0f;
    constexpr float bias = 0.025f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            const int idx = x + y * width;
            const float currentZ = zbuffer[idx];

            if (currentZ < -10000.0f) continue;

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
                    int sampleIdx = sampleX + sampleY * width;
                    float sampleZ = zbuffer[sampleIdx];

                    if (sampleZ > currentZ + bias) {
                        float rangeCheck = std::abs(currentZ - sampleZ) < 50.0f ? 1.0f : 0.0f;
                        occlusion += 1.0f * rangeCheck;
                    }
                }
            }

            occlusion = (occlusion / pixelSamples) * strength;
            if (occlusion > 1.0f) occlusion = 1.0f;
            occlusionBuffer[idx] = 1.0f - occlusion;
        }
    }
    return occlusionBuffer;
}
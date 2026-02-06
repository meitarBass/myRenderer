#include "VisualTests.h"
#include <iostream>
#include <limits>
#include <cmath>
#include "../src/Core/Rasterizer.h"
#include "../src/Shaders/PhongShader.h"
#include "../src/Shaders/DepthShader.h"
#include "../src/Math/Matrix.h"

void VisualTests::runVisualSuite(const std::vector<ModelInstance>& scene, int width, int height) {
    std::cout << "--- Starting Visual Suite ---" << std::endl;

    constexpr int shadowW = 2048;
    constexpr int shadowH = 2048;

    Vec3f lightPos = Vec3f(1, 1, 1).normalize() * 3.0f;
    Vec3f center(0, 0, 0);
    Vec3f up(0, 1, 0);
    Vec3f eye(0, 0.5, 3);

    float angle = 30.0f;
    auto ModelMatrix = Matrix4f4::rotationY(angle);

    // Setup Light Matrices
    Matrix4f4 lightView = Matrix4f4::lookat(lightPos, center, up);
    Matrix4f4 lightProj = Matrix4f4::projection(3.0f);
    Matrix4f4 lightViewport = Matrix4f4::viewport(0, 0, shadowW, shadowH);

    Matrix4f4 lightMVP = lightViewport * lightProj * lightView * ModelMatrix;
    Matrix4f4 lightProjView = lightProj * lightView;

    // --- STEP 1: Shadow Pass ---
    std::cout << "1. Running Shadow Pass..." << std::endl;
    std::vector<float> shadowMap = runShadowPass(scene, lightProj * lightView, shadowW, shadowH);

    // --- STEP 2: Color Pass ---
    std::cout << "2. Running Color Pass..." << std::endl;
    Uniforms baseUniforms;
    baseUniforms.model = ModelMatrix;
    baseUniforms.modelView = Matrix4f4::lookat(eye, center, up) * ModelMatrix;
    baseUniforms.projection = Matrix4f4::projection(3);
    baseUniforms.viewport = Matrix4f4::viewport(width / 8, height / 8, width * 3/4, height * 3/4);
    baseUniforms.lightDir = (lightPos - center).normalize();
    baseUniforms.cameraPos = eye;
    baseUniforms.normalMatrix = baseUniforms.model.inverseTranspose3x3();

    RenderBuffers buffers = runColorPass(scene, width, height, baseUniforms, shadowMap, lightProjView);

    // --- STEP 3: SSAO & Post Processing ---
    std::cout << "3. Applying SSAO..." << std::endl;
    applySSAO(buffers.framebuffer, buffers.zbuffer, buffers.normalBuffer);

    // Save
    buffers.framebuffer.write_tga_file("visual_test_output.tga");
    std::cout << "Done! Saved visual_test_output.tga" << std::endl;
}

std::vector<float> VisualTests::runShadowPass(const std::vector<ModelInstance>& scene,
                                              const Matrix4f4& lightProjView, int w, int h) {
    TGAImage shadowImage(w, h, TGAImage::RGB);
    std::vector<float> shadowBuffer(w * h, -std::numeric_limits<float>::max());

    Matrix4f4 lightViewport = Matrix4f4::viewport(0, 0, w, h);

    for (const auto& object : scene) {
        Uniforms depthUniforms;
        depthUniforms.projection = Matrix4f4::identity();
        depthUniforms.viewport = lightViewport;

        auto ModelMatrix = Matrix4f4::rotationY(30.0f);
        depthUniforms.modelView = lightProjView * ModelMatrix;

        DepthShader depthShader(depthUniforms);
        RenderContext ctx = { object.model, shadowImage, shadowBuffer };
        drawModel(ctx, depthShader);
    }
    return shadowBuffer;
}

VisualTests::RenderBuffers VisualTests::runColorPass(const std::vector<ModelInstance>& scene,
                                                     int width, int height,
                                                     const Uniforms& baseUniforms,
                                                     const std::vector<float>& shadowMap,
                                                     const Matrix4f4& lightProjView) {
    RenderBuffers buffers;
    buffers.framebuffer = TGAImage(width, height, TGAImage::RGB);
    buffers.zbuffer.assign(width * height, -std::numeric_limits<float>::max());
    buffers.normalBuffer.assign(width * height, Vec3f(0,0,0));

    for (const auto& object : scene) {
        Uniforms uniforms = baseUniforms;

        uniforms.lightProjView = lightProjView;
        uniforms.shadowWidth = 2048;
        uniforms.shadowHeight = 2048;
        uniforms.shadowMap = &shadowMap;

        PhongShader shader(object.diffuse, object.normal, object.specular, uniforms, object.useAlphaTest);

        RenderContext ctx = { object.model, buffers.framebuffer, buffers.zbuffer, &buffers.normalBuffer };
        drawModel(ctx, shader);
    }
    return buffers;
}

void VisualTests::applySSAO(TGAImage& framebuffer,
                            const std::vector<float>& zbuffer,
                            const std::vector<Vec3f>& normals) {
    int width = framebuffer.width();
    int height = framebuffer.height();

    auto ssaoMap = computeSSAO(zbuffer, normals, width, height);

    for (int x = 0; x < width; x++) {
        for (int y = 0; y < height; y++) {
            TGAColor c = framebuffer.get(x, y);
            float intensity = ssaoMap[x + y * width];
            for (int i=0; i<3; i++) {
                c[i] = static_cast<unsigned char>(c[i] * intensity);
            }
            framebuffer.set(x, y, c);
        }
    }
}

std::vector<float> VisualTests::computeSSAO(const std::vector<float>& zbuffer,
                                            const std::vector<Vec3f>& normalBuffer,
                                            int width, int height) {
    std::vector<float> occlusionBuffer(width * height, 0.0f);
    std::cout << "Computing SSAO..." << std::endl;

    constexpr int kernelSize = 10;
    constexpr float strength = 2.0f;
    constexpr int pixelSamples = 16;
    constexpr float bias = 5.0f;

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            int idx = x + y * width;
            float currentZ = zbuffer[idx];
            if (currentZ < -10000.0f) continue;

            float occlusion = 0.0f;
            int samples = 0;

            for (int i = 0; i < pixelSamples; i++) {
                float r = randf() * kernelSize;
                float angle = randf() * 2.0f * 3.14159f;

                int sampleX = x + static_cast<int>(r * cos(angle));
                int sampleY = y + static_cast<int>(r * sin(angle));

                if (sampleX >= 0 && sampleX < width && sampleY >= 0 && sampleY < height) {
                    int sampleIdx = sampleX + sampleY * width;
                    float sampleZ = zbuffer[sampleIdx];

                    if (sampleZ > currentZ + bias) {
                        if (std::abs(currentZ - sampleZ) < 50.0f) {
                            occlusion += 1.0f;
                        }
                    }
                }
                samples++;
            }
            occlusion = (occlusion / samples) * strength;
            if (occlusion > 1.0f) occlusion = 1.0f;
            occlusionBuffer[idx] = 1.0f - occlusion;
        }
    }
    return occlusionBuffer;
}
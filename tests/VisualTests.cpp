#include "VisualTests.h"
#include <iostream>
#include <limits>

#include "../src/Core/Rasterizer.h"
#include "../src/Shaders/PhongShader.h"
#include "../src/Shaders/DepthShader.h"
#include "../src/Math/Matrix.h"

void VisualTests::runVisualSuite(const std::vector<ModelInstance>& scene, int width, int height) {
    std::cout << "--- Starting Visual Suite (Rotating Model) ---" << std::endl;

    constexpr int shadowW = 2048;
    constexpr int shadowH = 2048;

    Vec3f lightPos(1, 1, 1);
    lightPos = lightPos.normalize() * 3.0f;

    const Vec3f eye(0, 0.5, 3);
    const Vec3f center(0, 0, 0);
    const Vec3f up(0, 1, 0);

    float angles[] = {0.0f, 30.0f, 90.0f, 180.0f, 270.0f};
    std::string names[] = {"0", "30", "90", "180", "270"};

    auto Projection = Matrix4f4::projection(3);
    auto View       = Matrix4f4::viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);

    for (int i = 0; i < 5; i++) {
        std::cout << "Processing Angle: " << names[i] << std::endl;

        auto ModelMatrix = Matrix4f4::rotationY(angles[i]);

        // --- Shadow Pass ---
        Matrix4f4 lightViewport = Matrix4f4::viewport(0, 0, shadowW, shadowH);
        Matrix4f4 lightProj = Matrix4f4::projection(3.0f);
        Matrix4f4 lightView = Matrix4f4::lookat(lightPos, center, up);

        TGAImage shadowImage(shadowW, shadowH, TGAImage::RGB);
        std::vector<float> shadowBuffer(shadowW * shadowH, -std::numeric_limits<float>::max());

        Uniforms depthUniforms;
        depthUniforms.projection = lightProj;
        depthUniforms.viewport = lightViewport;
        depthUniforms.modelView = lightView * ModelMatrix;

        DepthShader depthShader(depthUniforms);

        for (auto& object : scene) {
             RenderContext ctx = {
                .model = object.model,
                .framebuffer = shadowImage,
                .zbuffer = shadowBuffer
            };
            drawModel(ctx, depthShader);
        }

        // --- Render Pass ---
        TGAImage framebuffer(width, height, TGAImage::RGB);
        std::vector<float> zbuffer(width * height, -std::numeric_limits<float>::max());

        Uniforms uniforms;
        uniforms.model = ModelMatrix;
        uniforms.modelView = Matrix4f4::lookat(eye, center, up) * ModelMatrix;
        uniforms.projection = Projection;
        uniforms.viewport = View;
        uniforms.lightDir = (lightPos - center).normalize();
        uniforms.cameraPos = eye;
        uniforms.normalMatrix = uniforms.model.inverseTranspose3x3();

        uniforms.lightProjView = lightProj * lightView;
        uniforms.shadowWidth = shadowW;
        uniforms.shadowHeight = shadowH;
        uniforms.shadowMap = &shadowBuffer;

        std::vector<Vec3f> normalBuffer(width * height, Vec3f(0,0,0));

        for (auto object : scene) {
            RenderContext ctx = {
                .model = object.model,
                .framebuffer = framebuffer,
                .zbuffer = zbuffer,
                .normalBuffer = &normalBuffer
            };

            auto shader = PhongShader(object.diffuse, object.normal, object.specular, uniforms, object.useAlphaTest);
            drawModel(ctx, shader);
        }

        // --- SSAO Pass ---
        std::vector<float> ssaoMap = computeSSAO(zbuffer, normalBuffer, width, height);

        for (int x = 0; x < width; x++) {
            for (int y = 0 ; y < height; y++) {
                int index = x + y * width;
                TGAColor c = framebuffer.get(x, y);
                float intensity = ssaoMap[index];

                for (int j = 0; j < 3; j++) {
                    c[j] = static_cast<unsigned char>(std::min(255.0f, static_cast<float>(c[j]) * intensity));
                }
                framebuffer.set(x, y, c);
            }
        }

        std::string filename = "visual_test_" + names[i] + ".tga";
        framebuffer.write_tga_file(filename.c_str());
    }
    std::cout << "--- Visual Suite Done! ---" << std::endl;
}

std::vector<float> VisualTests::computeSSAO(const std::vector<float>& zbuffer, const std::vector<Vec3f>& normalBuffer, int width, int height) {
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
                const float r = randf() * kernelSize;
                const float angle = randf() * 2.0f * GraphicsUtils::PI;

                const int sampleX = x + static_cast<int>(r * cos(angle));
                const int sampleY = y + static_cast<int>(r * sin(angle));

                if (sampleX >= 0 && sampleX < width && sampleY >= 0 && sampleY < height) {
                    const int sampleIdx = sampleX + sampleY * width;
                    const float sampleZ = zbuffer[sampleIdx];

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
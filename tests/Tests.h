#ifndef RENDERER_TESTS_H
#define RENDERER_TESTS_H

#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include "../src/Math/Matrix.h"
#include "../src/Math/Vec.h"
#include "../src/Math/Geometry.h"
#include "../src/Core/Rasterizer.h"

class RendererTests {
public:
    static void runAll() {
        std::cout << "--- Starting Unit Tests ---" << std::endl;
        testVectorMath();
        testMatrixIdentity();
        testMatrixShear();
        testBarycentric();
        std::cout << "--- Unit Tests Passed! ---" << std::endl;
    }

    static void runVisualSuite(const ModelLoader &model, const TGAImage& texture, int width, int height) {
        std::cout << "--- Starting Visual Suite (Rotating Model) ---" << std::endl;

        Vec3f eye(0, 0, 3);
        Vec3f center(0, 0, 0);
        Vec3f up(0, 1, 0);
        Vec3f lightDir = Vec3f(1,1,1).normalize();

        auto Projection = Matrix4f4::projection(7);
        auto View       = Matrix4f4::viewport(width / 8, height / 8, width * 3 / 4, height * 3 / 4);

        float angles[] = {0.0f, 90.0f, 180.0f, 270.0f};
        std::string names[] = {"0", "90", "180", "270"};

        for (int i = 0; i < 4; i++) {
            TGAImage framebuffer(width, height, TGAImage::RGB);

            for (int x = 0; x < width; x++) {
                for (int y = 0; y < height; y++) {
                    // צובע בלבן: R=255, G=255, B=255, A=255
                    framebuffer.set(x, y, {255, 255, 255, 255});
                }
            }

            std::vector<float> zbuffer(width * height, -std::numeric_limits<float>::max());

            auto Model = Matrix4f4::rotationY(angles[i]);
            auto LookAt = Matrix4f4::lookat(eye, center, up);
            auto totalMat = View * Projection * LookAt * Model;

            // שימוש ב-RenderContext החדש שבנינו
            RenderContext ctx = {
                .model = model,
                .framebuffer = framebuffer,
                .zbuffer = zbuffer,
            };

            Uniforms uniforms;
            uniforms.modelView = LookAt * Model;
            uniforms.projection = Projection;
            uniforms.viewport = View;
            uniforms.lightDir = lightDir;
            uniforms.cameraPos = eye;

            auto shader = PhongShader(texture, uniforms);

            std::cout << "  Rendering angle: " << names[i] << " degrees..." << std::endl;

            drawModel(ctx, shader);

            std::string filename = "test_output_" + names[i] + ".tga";
            framebuffer.write_tga_file(filename.c_str());
        }
        std::cout << "--- Visual Suite Done! ---" << std::endl;
    }

private:
    static void testVectorMath() {
        Vec3f v1(1, 0, 0), v2(0, 1, 0);
        assert(dotProduct(v1, v2) == 0);
        assert(cross(v1, v2)[2] == 1);
        std::cout << "  [OK] Vector Math" << std::endl;
    }

    static void testMatrixIdentity() {
        auto identity = Matrix4f4::identity();
        Vec4f v(1, 2, 3, 1);
        Vec4f res = identity * v;
        assert(std::abs(res[0] - 1.f) < 1e-5);
        std::cout << "  [OK] Matrix Identity" << std::endl;
    }

    static void testMatrixShear() {
        auto s = Matrix4f4::shear(0.5f, 0, 0, 0, 0, 0);
        Vec4f v(0, 1, 0, 1);
        Vec4f res = s * v;
        assert(std::abs(res[0] - 0.5f) < 1e-5);
        std::cout << "  [OK] Matrix Shear" << std::endl;
    }

    static void testBarycentric() {
        Vec2f A(0, 0);
        Vec2f B(10, 0);
        Vec2f C(0, 10);

        Vec2f P(0, 0);

        Point3 bc = barycentric(A, B, C, P);

        assert(std::abs(bc.x() - 1.0f) < 1e-5);

        Vec2f center(3.333f, 3.333f);
        bc = barycentric(A, B, C, center);
        assert(std::abs(bc.x() - 0.3333f) < 1e-2);

        std::cout << "  [OK] Barycentric" << std::endl;
    }
};

#endif
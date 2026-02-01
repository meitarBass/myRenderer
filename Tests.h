#ifndef RENDERER_TESTS_H
#define RENDERER_TESTS_H

#include <cassert>
#include <iostream>
#include <string>
#include <vector>
#include "Geometry/Matrix.h"
#include "Geometry/Vec.h"
#include "Geometry/Geometry.h"
#include "Rasterizer.h"

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

        auto ModelView  = Matrix4f4::lookat(eye, center, up);
        auto Projection = Matrix4f4::projection(7);
        auto View       = Matrix4f4::viewport(width/8, height/8, width*3/4, height*3/4);

        float angles[] = {0.0f, GraphicsUtils::PI/2, GraphicsUtils::PI, 3*GraphicsUtils::PI/2};
        std::string names[] = {"0", "90", "180", "270"};

        for (int i = 0; i < 4; i++) {
            TGAImage framebuffer(width, height, TGAImage::RGB);
            auto zbuffer = std::vector<float>(width * height, -std::numeric_limits<float>::max());

            auto Rotation = Matrix4f4::rotationY(angles[i]);
            auto Shear = Matrix4f4::shear(0.1f, 0, 0, 0, 0, 0);

            auto modelMat = Rotation * Shear;

            auto mat = View * Projection * ModelView * modelMat;

            std::cout << "  Rendering angle: " << names[i] << " degrees..." << std::endl;

            drawModel(model, framebuffer, texture, zbuffer, mat, modelMat, eye);

            std::string filename = "test_output_" + names[i] + ".tga";
            framebuffer.write_tga_file(filename.c_str());
        }
        std::cout << "--- Visual Suite Done! Check your folder for 4 images ---" << std::endl;
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
        Point3 pts[3] = {{0,0,0}, {1,0,0}, {0,1,0}};
        Vec2f uv[3] = {};
        float w[3] = {1,1,1};
        Triangle tri(pts, uv, w);
        Vec3f bc = barycentric(tri, Point3(0,0,0));
        assert(std::abs(bc[0] - 1.0f) < 1e-5);
        std::cout << "  [OK] Barycentric" << std::endl;
    }
};

#endif
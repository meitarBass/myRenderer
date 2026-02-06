#include "RendererUnitTests.h"
#include <iostream>
#include <cassert>
#include "../Math/Vec.h"
#include "../Math/Matrix.h"
#include "../Core/Rasterizer.h"

void RendererUnitTests::runAll() {
    std::cout << "--- Starting Unit Tests ---" << std::endl;
    RendererUnitTests::testVectorMath();
    RendererUnitTests::testMatrixIdentity();
    RendererUnitTests::testMatrixShear();
    RendererUnitTests::testBarycentric();
    std::cout << "--- Unit Tests Passed! ---" << std::endl;
}


void RendererUnitTests::testVectorMath() {
    Vec3f v1(1, 0, 0), v2(0, 1, 0);
    assert(dotProduct(v1, v2) == 0);
    assert(cross(v1, v2)[2] == 1);
    std::cout << "  [OK] Vector Math" << std::endl;
}

void RendererUnitTests::testMatrixIdentity() {
    auto identity = Matrix4f4::identity();
    Vec4f v(1, 2, 3, 1);
    Vec4f res = identity * v;
    assert(std::abs(res[0] - 1.f) < GraphicsUtils::EPSILON);
    std::cout << "  [OK] Matrix Identity" << std::endl;
}

void RendererUnitTests::testMatrixShear() {
    auto s = Matrix4f4::shear(0.5f, 0, 0, 0, 0, 0);
    Vec4f v(0, 1, 0, 1);
    Vec4f res = s * v;
    assert(std::abs(res[0] - 0.5f) < GraphicsUtils::EPSILON);
    std::cout << "  [OK] Matrix Shear" << std::endl;
}

void RendererUnitTests::testBarycentric() {
    Vec2f A(0, 0);
    Vec2f B(10, 0);
    Vec2f C(0, 10);

    Vec2f P(0, 0);

    Point3 bc = barycentric(A, B, C, P);

    assert(std::abs(bc.x() - 1.0f) < GraphicsUtils::EPSILON);

    Vec2f center(3.333f, 3.333f);
    bc = barycentric(A, B, C, center);
    assert(std::abs(bc.x() - 0.3333f) < 1e-2);

    std::cout << "  [OK] Barycentric" << std::endl;
}
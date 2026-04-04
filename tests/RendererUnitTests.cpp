#include "RendererUnitTests.h"
#include <iostream>
#include <cassert>
#include <cmath>
#include "../Math/Vec.h"
#include "../Math/Matrix.h"
#include "../Core/Rasterizer.h"

void RendererUnitTests::runAll() {
    std::cout << "--- Starting Core Math Unit Tests ---" << std::endl;

    testVectorAdvanced();
    testMatrixIdentity();
    testMatrixTransforms();
    testMatrixRotations();
    testCameraMatrices();
    testMatrixShear();
    testBarycentric();

    std::cout << "--- All Unit Tests Passed Successfully! ---" << std::endl;
}

void RendererUnitTests::testVectorAdvanced() {
    Vec3f v1(1.0f, 2.0f, 3.0f);
    Vec3f v2(4.0f, 5.0f, 6.0f);
    Vec3f sum = v1 + v2;
    assert(std::abs(sum.x() - 5.0f) < GraphicsUtils::EPSILON);
    assert(std::abs(sum.y() - 7.0f) < GraphicsUtils::EPSILON);

    Vec3f v3(3.0f, 4.0f, 0.0f);
    assert(std::abs(v3.length() - 5.0f) < GraphicsUtils::EPSILON);

    Vec3f norm = Vec3f(10.0f, 0.0f, 0.0f).normalize();
    assert(std::abs(norm.x() - 1.0f) < GraphicsUtils::EPSILON);
    assert(std::abs(norm.length() - 1.0f) < GraphicsUtils::EPSILON);

    Vec3f xAxis(1, 0, 0);
    Vec3f yAxis(0, 1, 0);
    assert(std::abs(dotProduct(xAxis, yAxis) - 0.0f) < GraphicsUtils::EPSILON);
    assert(std::abs(cross(xAxis, yAxis).z() - 1.0f) < GraphicsUtils::EPSILON);

    std::cout << "  [OK] Vector Advanced Math" << std::endl;
}

void RendererUnitTests::testMatrixIdentity() {
    auto identity = Matrix4f4::identity();
    Vec4f v(1.5f, -2.5f, 3.0f, 1.0f);
    Vec4f res = identity * v;

    assert(std::abs(res[0] - 1.5f) < GraphicsUtils::EPSILON);
    assert(std::abs(res[1] + 2.5f) < GraphicsUtils::EPSILON);
    assert(std::abs(res[3] - 1.0f) < GraphicsUtils::EPSILON);

    std::cout << "  [OK] Matrix Identity" << std::endl;
}

void RendererUnitTests::testMatrixTransforms() {
    auto tMat = Matrix4f4::translation(Vec3f(2.0f, 3.0f, 4.0f));
    Vec4f p(0.0f, 0.0f, 0.0f, 1.0f);
    Vec4f tp = tMat * p;
    assert(std::abs(tp.x() - 2.0f) < GraphicsUtils::EPSILON);
    assert(std::abs(tp.y() - 3.0f) < GraphicsUtils::EPSILON);
    assert(std::abs(tp.z() - 4.0f) < GraphicsUtils::EPSILON);

    auto sMat = Matrix4f4::scale(2.0f, 0.5f, 1.0f);
    Vec4f p2(10.0f, 10.0f, 10.0f, 1.0f);
    Vec4f sp = sMat * p2;
    assert(std::abs(sp.x() - 20.0f) < GraphicsUtils::EPSILON);
    assert(std::abs(sp.y() - 5.0f) < GraphicsUtils::EPSILON);
    assert(std::abs(sp.z() - 10.0f) < GraphicsUtils::EPSILON);

    std::cout << "  [OK] Matrix Transforms (Translation, Scale)" << std::endl;
}

void RendererUnitTests::testMatrixRotations() {
    auto rotZ = Matrix4f4::rotationZ(90.0f);
    Vec4f vX(1.0f, 0.0f, 0.0f, 1.0f);
    Vec4f rotRes = rotZ * vX;

    assert(std::abs(rotRes.x() - 0.0f) < GraphicsUtils::EPSILON);
    assert(std::abs(rotRes.y() - 1.0f) < GraphicsUtils::EPSILON);

    std::cout << "  [OK] Matrix Rotations" << std::endl;
}

void RendererUnitTests::testCameraMatrices() {
    float fov = 90.0f;
    float aspect = 1.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    auto proj = Matrix4f4::perspective(fov, aspect, nearPlane, farPlane);

    assert(std::abs(proj[0][0] - 1.0f) < GraphicsUtils::EPSILON);
    assert(std::abs(proj[1][1] - 1.0f) < GraphicsUtils::EPSILON);
    assert(std::abs(proj[2][3] - (-1.0f)) < GraphicsUtils::EPSILON);

    Vec3f eye(0, 0, 5);
    Vec3f center(0, 0, 0);
    Vec3f up(0, 1, 0);
    auto view = Matrix4f4::lookat(eye, center, up);

    Vec4f origin(0, 0, 0, 1);
    Vec4f viewOrigin = view * origin;
    assert(std::abs(viewOrigin.z() - (-5.0f)) < GraphicsUtils::EPSILON);

    std::cout << "  [OK] Camera Matrices (Perspective, LookAt)" << std::endl;
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
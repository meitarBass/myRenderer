#ifndef RENDERER_TESTS_H
#define RENDERER_TESTS_H


class RendererUnitTests {
public:
    static void runAll();

private:
    static void testVectorAdvanced();
    static void testMatrixIdentity();
    static void testMatrixTransforms();
    static void testMatrixRotations();
    static void testCameraMatrices(); // Perspective + LookAt
    static void testMatrixShear();
    static void testBarycentric();
};

#endif
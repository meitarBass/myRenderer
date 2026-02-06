#ifndef RENDERER_TESTS_H
#define RENDERER_TESTS_H


class RendererUnitTests {
public:
    static void runAll();

private:

    static void testVectorMath();
    static void testMatrixIdentity();
    static void testMatrixShear();
    static void testBarycentric();
};

#endif
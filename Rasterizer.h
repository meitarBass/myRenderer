#ifndef RENDERER_RASTERIZER_H
#define RENDERER_RASTERIZER_H

#include "Geometry/Geometry.h"
#include "Helpers/tgaimage.h"
#include "ModelLoader.h"

struct RenderContext {
    const ModelLoader &model;
    const TGAImage &diffuseMap;
    const TGAImage *normalMap = nullptr;

    TGAImage& framebuffer;
    std::vector<float>& zbuffer;

    Matrix4f4 totalMat;
    Matrix4f4 modelMat;
    Vec3f eye;
    Vec3f lightDir;
};


Point3 barycentric(Triangle tri, Point3 P);
BBox computeTriangleBBox(const Triangle &tri);
void drawTriangle(const Triangle &tri, RenderContext &ctx, const float intensity[3]);
void drawModel(RenderContext &ctx);





void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color);
void fillTriangle(const Triangle& tri, TGAImage &framebuffer, TGAColor color);


#endif //RENDERER_RASTERIZER_H

#ifndef RENDERER_RASTERIZER_H
#define RENDERER_RASTERIZER_H

#include "../Math/Geometry.h"
#include "../IO/tgaimage.h"
#include "../IO/ModelLoader.h"
#include "IShader.h"

struct RenderContext {
    const ModelLoader &model;
    TGAImage& framebuffer;
    std::vector<float>& zbuffer;

    std::vector<Vec3f>* normalBuffer = nullptr;
};


Point3 barycentric(const Vec2f& A, const Vec2f& B, const Vec2f& C, const Vec2f& P);
BBox computeTriangleBBox(const Vec3f pts[3]);
void drawTriangle(const Varyings varyings[3], IShader &shader, const RenderContext &ctx);
void drawModel(const RenderContext &ctx, IShader& shader);
std::pair<Vec3f, Vec3f> calculateTriangleBasis(const Vec3f pts[3], const Vec2f uvs[3]);

#endif //RENDERER_RASTERIZER_H

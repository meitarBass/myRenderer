#ifndef RENDERER_RASTERIZER_H
#define RENDERER_RASTERIZER_H

#include "Geometry/Geometry.h"
#include "Helpers/tgaimage.h"
#include "ModelLoader.h"


Point3 barycentric(Triangle tri, Point3 P);
BBox computeTriangleBBox(const Triangle &tri);
void drawTriangle(const Triangle &tri, std::vector<float>& zbuffer,
                  TGAImage &image, const TGAImage& texture, const float intensity[3]);

void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color);
void fillTriangle(const Triangle& tri, TGAImage &framebuffer, TGAColor color);

void drawModel(const ModelLoader &model, TGAImage &framebuffer, const TGAImage& texture,
               std::vector<float>& zbuffer, const Matrix4f4 &mat, Vec3f eye);

inline void loadFaces(ModelLoader &model);


#endif //RENDERER_RASTERIZER_H

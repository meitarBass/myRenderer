#ifndef RENDERER_RASTERIZER_H
#define RENDERER_RASTERIZER_H

#include "Geometry/Geometry.h"
#include "Helpers/tgaimage.h"
#include <algorithm>


Point3 barycentric(Triangle tri, Point3 P);
BBox computeTriangleBBox(const Triangle &tri);
void drawTriangle(const Triangle &tri, std::vector<float>& zbuffer,
                  TGAImage &image, const float intensity[3]);

void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color);
void fillTriangle(const Triangle& tri, TGAImage &framebuffer, TGAColor color);

#endif //RENDERER_RASTERIZER_H

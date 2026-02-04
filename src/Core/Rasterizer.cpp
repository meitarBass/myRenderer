#include "Rasterizer.h"

Point3 barycentric(const Vec2f& A, const Vec2f& B, const Vec2f& C, const Vec2f& P) {
    const Vec2f v0 = B - A;
    const Vec2f v1 = C - A;
    const Vec2f v2 = P - A;

    float denom = determinant2D(v0, v1);
    if (std::abs(denom) < GraphicsUtils::EPSILON) return { -1, 1, 1 };

    float beta = determinant2D(v2, v1) / denom;
    float gamma = determinant2D(v0, v2) / denom;
    float alpha = 1.0f - beta - gamma;

    return { alpha, beta, gamma };
}

BBox computeTriangleBBox(const Vec3f pts[3]) {
    Vec2f minVal(std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec2f maxVal(-std::numeric_limits<float>::max(), -std::numeric_limits<float>::max());

    for (int i = 0; i < 3; i++) {
        minVal.x() = std::min(minVal.x(), pts[i].x());
        minVal.y() = std::min(minVal.y(), pts[i].y());
        maxVal.x() = std::max(maxVal.x(), pts[i].x());
        maxVal.y() = std::max(maxVal.y(), pts[i].y());
    }
    return { Point3(minVal.x(), minVal.y(), 0), Point3(maxVal.x(), maxVal.y(), 0) };
}

void drawTriangle(const Varyings varyings[3], IShader &shader, TGAImage &framebuffer, std::vector<float> &zbuffer) {
    Vec3f pts[3] = { varyings[0].screenPos, varyings[1].screenPos, varyings[2].screenPos };
    BBox bbox = computeTriangleBBox(pts);

    const int width = framebuffer.width();
    const int height = framebuffer.height();

    const int minX = std::max(0, (int)bbox._boxMin.x());
    const int maxX = std::min(width - 1, (int)bbox._boxMax.x());
    const int minY = std::max(0, (int)bbox._boxMin.y());
    const int maxY = std::min(height - 1, (int)bbox._boxMax.y());

    for (int x = minX; x <= maxX; x++) {
        for (int y = minY; y <= maxY; y++) {
            Vec3f bc = barycentric(Vec2f(pts[0].x(), pts[0].y()),
                                   Vec2f(pts[1].x(), pts[1].y()),
                                   Vec2f(pts[2].x(), pts[2].y()),
                                   Vec2f(x, y));

            if (bc.x() < 0 || bc.y() < 0 || bc.z() < 0) continue;

            const float z = pts[0].z() * bc.x() + pts[1].z() * bc.y() + pts[2].z() * bc.z();
            const int index = x + y * width;

            if (zbuffer[index] < z) {
                Varyings pixelVaryings = IShader::interpolate(varyings[0], varyings[1], varyings[2], bc);

                TGAColor color;
                bool discard = shader.fragment(pixelVaryings, color);

                if (!discard) {
                    zbuffer[index] = z;
                    framebuffer.set(x, y, color);
                }
            }
        }
    }
}

void drawModel(RenderContext &ctx, IShader& shader) {
    const auto &faces = ctx.model.getFaces();

    for (const auto &face : faces) {
        Varyings triangleVaryings[3];
        for (int i = 0 ; i < 3; i++) {
            triangleVaryings[i] = shader.vertex(face.pts[i], face.normals[i], face.uv[i]);
        }
        drawTriangle(triangleVaryings, shader, ctx.framebuffer, ctx.zbuffer);
    }
}
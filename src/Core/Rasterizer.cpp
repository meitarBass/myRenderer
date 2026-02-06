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

void drawTriangle(const Varyings varyings[3], IShader &shader, const RenderContext &ctx) {
    Vec3f pts[3] = { varyings[0].screenPos, varyings[1].screenPos, varyings[2].screenPos };
    BBox bbox = computeTriangleBBox(pts);

    const int width = ctx.framebuffer.width();
    const int height = ctx.framebuffer.height();

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

            if (ctx.zbuffer[index] < z) {
                Varyings pixelVaryings = IShader::interpolate(varyings[0], varyings[1], varyings[2], bc);

                TGAColor color;
                bool discard = shader.fragment(pixelVaryings, color);

                if (!discard) {
                    ctx.zbuffer[index] = z;
                    ctx.framebuffer.set(x, y, color);

                    if (ctx.normalBuffer) {
                        (*ctx.normalBuffer)[index] = shader.outNormal;
                    }
                }
            }
        }
    }
}

void drawModel(const RenderContext &ctx, IShader& shader) {
    for (const auto &faces = ctx.model.getFaces(); const auto &face : faces) {
        const auto basis = calculateTriangleBasis(face.pts, face.uv);
        Vec3f tangent = basis.first;
        Vec3f bitangent = basis.second;

        Varyings triangleVaryings[3];
        for (int i = 0 ; i < 3; i++) {
            triangleVaryings[i] = shader.vertex(face.pts[i], face.normals[i], face.uv[i], tangent, bitangent);
        }
        drawTriangle(triangleVaryings, shader, ctx);
    }
}

std::pair<Vec3f, Vec3f> calculateTriangleBasis(const Vec3f pts[3], const Vec2f uvs[3]) {
    Vec3f edge1 = pts[1] - pts[0];
    Vec3f edge2 = pts[2] - pts[0];

    Vec2f deltaUV1 = uvs[1] - uvs[0];
    Vec2f deltaUV2 = uvs[2] - uvs[0];

    const float f = 1.0f / (deltaUV1.x() * deltaUV2.y() - deltaUV2.x() * deltaUV1.y());

    Vec3f tangent, bitangent;

    tangent.x() = f * (deltaUV2.y() * edge1.x() - deltaUV1.y() * edge2.x());
    tangent.y() = f * (deltaUV2.y() * edge1.y() - deltaUV1.y() * edge2.y());
    tangent.z() = f * (deltaUV2.y() * edge1.z() - deltaUV1.y() * edge2.z());

    bitangent.x() = f * (-deltaUV2.x() * edge1.x() + deltaUV1.x() * edge2.x());
    bitangent.y() = f * (-deltaUV2.x() * edge1.y() + deltaUV1.x() * edge2.y());
    bitangent.z() = f * (-deltaUV2.x() * edge1.z() + deltaUV1.x() * edge2.z());

    return {tangent.normalize(), bitangent.normalize()};
}
#include "Rasterizer.h"
#include <thread>
#include <vector>
#include <atomic>
#include <algorithm>

constexpr int TILE_SIZE = 32;

struct Tile {
    std::vector<int> triangleIndices;
};

struct ProcessedTriangle {
    Varyings varyings[3];
};

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

void drawTriangleClipped(const Varyings varyings[3], IShader &shader, const RenderContext &ctx,
                         int tileMinX, int tileMinY, int tileMaxX, int tileMaxY) {

    Vec3f pts[3] = { varyings[0].screenPos, varyings[1].screenPos, varyings[2].screenPos };
    BBox bbox = computeTriangleBBox(pts);

    const int minX = std::max(tileMinX, (int)bbox._boxMin.x());
    const int maxX = std::min(tileMaxX, (int)bbox._boxMax.x());
    const int minY = std::max(tileMinY, (int)bbox._boxMin.y());
    const int maxY = std::min(tileMaxY, (int)bbox._boxMax.y());

    if (minX > maxX || minY > maxY) return;

    const int width = ctx.framebuffer.width();

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

void drawModel(const RenderContext &ctx, IShader& shader) {
    const int width = ctx.framebuffer.width();
    const int height = ctx.framebuffer.height();

    const int numTilesX = (width + TILE_SIZE - 1) / TILE_SIZE;
    const int numTilesY = (height + TILE_SIZE - 1) / TILE_SIZE;

    std::vector<Tile> tiles(numTilesX * numTilesY);

    const auto &faces = ctx.model.getFaces();
    std::vector<ProcessedTriangle> processedTriangles(faces.size());

    for (size_t i = 0; i < faces.size(); ++i) {
        const auto &face = faces[i];
        const auto basis = calculateTriangleBasis(face.pts, face.uv);

        for (int j = 0; j < 3; j++) {
            processedTriangles[i].varyings[j] = shader.vertex(face.pts[j], face.normals[j], face.uv[j], basis.first, basis.second);
        }

        Vec3f screenPts[3] = {
            processedTriangles[i].varyings[0].screenPos,
            processedTriangles[i].varyings[1].screenPos,
            processedTriangles[i].varyings[2].screenPos
        };
        BBox bbox = computeTriangleBBox(screenPts);

        int minTx = std::max(0, (int)(bbox._boxMin.x() / TILE_SIZE));
        int maxTx = std::min(numTilesX - 1, (int)(bbox._boxMax.x() / TILE_SIZE));
        int minTy = std::max(0, (int)(bbox._boxMin.y() / TILE_SIZE));
        int maxTy = std::min(numTilesY - 1, (int)(bbox._boxMax.y() / TILE_SIZE));

        for (int ty = minTy; ty <= maxTy; ++ty) {
            for (int tx = minTx; tx <= maxTx; ++tx) {
                tiles[ty * numTilesX + tx].triangleIndices.push_back(i);
            }
        }
    }

    std::atomic<int> nextTileIndex{0};
    int totalTiles = tiles.size();

    unsigned int numThreads = std::thread::hardware_concurrency();
    if (numThreads == 0) numThreads = 4;

    std::vector<std::thread> workers;

    for (unsigned int t = 0; t < numThreads; ++t) {
        workers.emplace_back([&]() {
            int tileIdx;
            while ((tileIdx = nextTileIndex.fetch_add(1)) < totalTiles) {
                const auto& tile = tiles[tileIdx];
                if (tile.triangleIndices.empty()) continue;

                int tx = tileIdx % numTilesX;
                int ty = tileIdx / numTilesX;
                int minX = tx * TILE_SIZE;
                int minY = ty * TILE_SIZE;
                int maxX = std::min(minX + TILE_SIZE - 1, width - 1);
                int maxY = std::min(minY + TILE_SIZE - 1, height - 1);

                for (size_t k = 0; k < tile.triangleIndices.size(); ++k) {
                    int triIdx = tile.triangleIndices[k];
                    drawTriangleClipped(processedTriangles[triIdx].varyings, shader, ctx, minX, minY, maxX, maxY);
                }
            }
        });
    }

    for (auto& worker : workers) {
        worker.join();
    }
}
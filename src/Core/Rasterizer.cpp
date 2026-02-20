#include "Rasterizer.h"
#include <thread>
#include <vector>
#include <atomic>
#include <algorithm>
#include "../Utils/ThreadPool.h"

constexpr int TILE_SIZE = 32;

struct Tile {
    std::vector<int> triangleIndices;
};

struct ProcessedTriangle {
    Varyings varyings[3];
};

Point3 barycentric(const Vec2f& A, const Vec2f& B, const Vec2f& C, const Vec2f& P)
{
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

BBox computeTriangleBBox(const Vec3f pts[3])
{
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

void drawTriangleClipped(const Varyings varyings[3], IShader &shader, const RenderContext &ctx,
                         const int tileMinX, const int tileMinY, const int tileMaxX, const int tileMaxY)
{
    Vec3f pts[3] = { varyings[0].screenPos, varyings[1].screenPos, varyings[2].screenPos };
    BBox bbox = computeTriangleBBox(pts);

    const int minX = std::max(tileMinX, (int)bbox._boxMin.x());
    const int maxX = std::min(tileMaxX, (int)bbox._boxMax.x());
    const int minY = std::max(tileMinY, (int)bbox._boxMin.y());
    const int maxY = std::min(tileMaxY, (int)bbox._boxMax.y());

    if (minX > maxX || minY > maxY) return;

    for (int y = minY; y <= maxY; y++) {
        for (int x = minX; x <= maxX; x++) {
            Vec3f bc = barycentric(Vec2f(pts[0].x(), pts[0].y()),
                                   Vec2f(pts[1].x(), pts[1].y()),
                                   Vec2f(pts[2].x(), pts[2].y()),
                                   Vec2f(x, y));

            if (bc.x() < 0 || bc.y() < 0 || bc.z() < 0) continue;

            float z = pts[0].z() * bc.x() + pts[1].z() * bc.y() + pts[2].z() * bc.z();
            int index = x + y * ctx.width;

            if (ctx.zbuffer[index] < z) {
                TGAColor color;
                Varyings pixelVaryings = IShader::interpolate(varyings[0], varyings[1], varyings[2], bc);
                if (!shader.fragment(pixelVaryings, color)) {
                    ctx.zbuffer[index] = z;
                    if (ctx.framebuffer) ctx.framebuffer->set(x, y, color);
                    if (ctx.normalBuffer) (*ctx.normalBuffer)[index] = varyings->normalForBuffer;
                }
            }
        }
    }
}

std::pair<Vec3f, Vec3f> calculateTriangleBasis(const Vec3f pts[3], const Vec2f uvs[3])
{
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

inline std::vector<ProcessedTriangle> preProcessVertices(const ModelLoader& model, IShader& shader)
{
    const auto& faces = model.getFaces();
    std::vector<ProcessedTriangle> processed;
    processed.reserve(faces.size());

    for (const auto& face: faces) {
        auto [tangent, bitangent] = calculateTriangleBasis(face.pts, face.uv);

        ProcessedTriangle pt;

        for (int j = 0; j < 3; j++) {
            pt.varyings[j] =
                shader.vertex(face.pts[j], face.normals[j], face.uv[j], tangent, bitangent);
        }

        const Vec3f& p0 = pt.varyings[0].screenPos;
        const Vec3f& p1 = pt.varyings[1].screenPos;
        const Vec3f& p2 = pt.varyings[2].screenPos;

        const float signedArea = (p1.x() - p0.x()) * (p2.y() - p0.y()) - (p1.y() - p0.y()) * (p2.x() - p0.x());

        if (signedArea > 0.0f) {
            processed.push_back(pt);
        }
    }
    // std::cout << "Original faces: " << faces.size() << " | Rendered faces: " << processed.size() << std::endl;
    return processed;
}

inline std::vector<Tile> binTrianglesToTiles(const std::vector<ProcessedTriangle>& triangles,
                                             int width,
                                             int height,
                                             const int numTilesX,
                                             const int numTilesY)
{
    std::vector<Tile> tiles(numTilesX * numTilesY);

    for (size_t i = 0; i < triangles.size(); ++i) {
        Vec3f screenPts[3] = { triangles[i].varyings[0].screenPos,
                               triangles[i].varyings[1].screenPos,
                               triangles[i].varyings[2].screenPos };
        BBox bbox = computeTriangleBBox(screenPts);

        const int minTx = std::max(0, (int)(bbox._boxMin.x() / TILE_SIZE));
        const int maxTx = std::min(numTilesX - 1, (int)(bbox._boxMax.x() / TILE_SIZE));
        const int minTy = std::max(0, (int)(bbox._boxMin.y() / TILE_SIZE));
        const int maxTy = std::min(numTilesY - 1, (int)(bbox._boxMax.y() / TILE_SIZE));

        for (int ty = minTy; ty <= maxTy; ++ty) {
            for (int tx = minTx; tx <= maxTx; ++tx) {
                tiles[ty * numTilesX + tx].triangleIndices.push_back(i);
            }
        }
    }
    return tiles;
}

inline void tileWorker(std::atomic<int>& nextTileIndex,
                       const int totalTiles,
                       const std::vector<Tile>& tiles,
                       const std::vector<ProcessedTriangle>& processedTriangles,
                       IShader& shader,
                       const RenderContext& ctx,
                       const int numTilesX)
{
    int tileIdx;
    while ((tileIdx = nextTileIndex.fetch_add(1)) < totalTiles) {
        const auto& tile = tiles[tileIdx];
        if (tile.triangleIndices.empty()) continue;

        const int tx = tileIdx % numTilesX;
        const int ty = tileIdx / numTilesX;
        const int minX = tx * TILE_SIZE;
        const int minY = ty * TILE_SIZE;
        const int maxX = std::min(minX + TILE_SIZE - 1, ctx.width - 1);
        const int maxY = std::min(minY + TILE_SIZE - 1, ctx.height - 1);

        for (const int triIdx : tile.triangleIndices) {
            drawTriangleClipped(processedTriangles[triIdx].varyings, shader, ctx,
                         minX, minY, maxX, maxY);
        }
    }
}

void drawModel(const RenderContext &ctx, IShader& shader)
{
    const int numTilesX = (ctx.width + TILE_SIZE - 1) / TILE_SIZE;
    const int numTilesY = (ctx.height + TILE_SIZE - 1) / TILE_SIZE;

    const auto processedTriangles = preProcessVertices(ctx.model, shader);
    const auto tiles = binTrianglesToTiles(processedTriangles,
                                                      ctx.width,
                                                      ctx.height,
                                                      numTilesX,
                                                      numTilesY);

    std::atomic<int> nextTileIndex{0};
    const unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());

    for (unsigned int t = 0; t < numThreads; ++t) {
        ThreadPool::instance().enqueue([&, t]() {
            tileWorker(std::ref(nextTileIndex),
                     static_cast<int>(tiles.size()),
                     std::cref(tiles),
          std::cref(processedTriangles),
                 std::ref(shader),
                      std::cref(ctx), numTilesX);
        });
    }

    ThreadPool::instance().waitFinished();
}
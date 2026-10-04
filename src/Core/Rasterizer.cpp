#include "Rasterizer.h"
#include "../Renderer/Scene.h"

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
                pixelVaryings.barycentric = bc;

                if (!shader.fragment(pixelVaryings, color)) {
                    const bool isTranslucentPixel = shader.usesBlending() && color.bgra[3] < 255;
                    if (isTranslucentPixel) {
                        if (ctx.blendDepthBuffer) {
                            if ((*ctx.blendDepthBuffer)[index] >= z) {
                                continue;
                            }
                            (*ctx.blendDepthBuffer)[index] = z;
                        }

                        if (ctx.colorBuffer) {
                            const float alpha = color.bgra[3] / 255.0f;
                            const int colorIdx = index * 3;
                            const unsigned char srcRGB[3] = { color.bgra[2], color.bgra[1], color.bgra[0] };
                            for (int c = 0; c < 3; ++c) {
                                const float dst = (*ctx.colorBuffer)[colorIdx + c];
                                const float blended = srcRGB[c] * alpha + dst * (1.0f - alpha);
                                (*ctx.colorBuffer)[colorIdx + c] = static_cast<unsigned char>(std::min(255.0f, blended));
                            }
                        }
                    } else {
                        ctx.zbuffer[index] = z;
                        if (ctx.colorBuffer) {
                            int colorIdx = index * 3;
                            (* ctx.colorBuffer)[colorIdx] = color.bgra[2];
                            (* ctx.colorBuffer)[colorIdx + 1] = color.bgra[1];
                            (* ctx.colorBuffer)[colorIdx + 2] = color.bgra[0];
                        }
                        if (ctx.normalBuffer) (*ctx.normalBuffer)[index] = varyings->normalForBuffer;
                        if (ctx.distanceBuffer) (*ctx.distanceBuffer)[index] = pixelVaryings.distanceForBuffer;
                        if (ctx.opacityBuffer) (*ctx.opacityBuffer)[index] = pixelVaryings.opacityForBuffer;
                    }
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

Vec3f calculateFaceNormal(const Vec3f& p0, const Vec3f &p1, const Vec3f& p2)
{
    return cross(p1 - p0, p2 - p0).normalize();
}

struct RawVertex {
    Vec3f pos;
    Vec3f normal;
    Vec2f uv;
};

inline RawVertex lerpRawVertex(const RawVertex& a, const RawVertex& b, const float t)
{
    return { a.pos + (b.pos - a.pos) * t,
             a.normal + (b.normal - a.normal) * t,
             a.uv + (b.uv - a.uv) * t };
}

// Sutherland-Hodgman Algorithm
template <typename InsideFn>
inline std::vector<RawVertex> clipPolygonAgainstPlane(const std::vector<RawVertex>& input,
                                                       const Matrix4f4& projView,
                                                       InsideFn insideFn)
{
    // A polygon with fewer than 3 vertices is degenerate/invalid
    if (input.size() < 3) return {};
    std::vector<RawVertex> output;

    // Iterate over each edge of the polygon by connecting the previous and current vertices
    for (size_t i = 0; i < input.size(); ++i) {
        const RawVertex& current = input[i];
        const RawVertex& prev = input[(i + input.size() - 1) % input.size()];

        // Compute the signed distance of both vertices from the clipping plane in Clip Space
        const float currentDist = insideFn(projView * Vec4f(current.pos));
        const float prevDist = insideFn(projView * Vec4f(prev.pos));

        // Determine if each vertex lies inside the frustum boundary (positive distance or zero = inside)
        const bool currentInside = currentDist >= 0.0f;
        const bool prevInside = prevDist >= 0.0f;

        // Case A: The edge crosses the clipping plane boundary (one vertex inside, one outside)
        if (currentInside != prevInside) {
            const float t = prevDist / (prevDist - currentDist);

            // Generate a new vertex at the intersection point and add it to the clipped polygon
            output.push_back(lerpRawVertex(prev, current, t));
        }

        // Case B: If the current vertex is inside, it survives the clip and stays in the polygon
        if (currentInside) {
            output.push_back(current);
        }
    }
    return output;
}

inline std::vector<RawVertex> clipTriangleToFrustum(const RawVertex verts[3], const Matrix4f4& projView)
{
    std::vector<RawVertex> polygon = { verts[0], verts[1], verts[2] };

    polygon = clipPolygonAgainstPlane(polygon, projView, [](const Vec4f& c) { return c.w() - GraphicsUtils::EPSILON; });
    polygon = clipPolygonAgainstPlane(polygon, projView, [](const Vec4f& c) { return c.w() - c.x(); }); // right
    polygon = clipPolygonAgainstPlane(polygon, projView, [](const Vec4f& c) { return c.w() + c.x(); }); // left
    polygon = clipPolygonAgainstPlane(polygon, projView, [](const Vec4f& c) { return c.w() - c.y(); }); // top
    polygon = clipPolygonAgainstPlane(polygon, projView, [](const Vec4f& c) { return c.w() + c.y(); }); // bottom

    return polygon;
}

inline std::vector<ProcessedTriangle> preProcessVertices(const ModelLoader& model, IShader& shader)
{
    const auto& faces = model.getFaces();
    std::vector<ProcessedTriangle> processed;
    processed.reserve(faces.size());

    const Matrix4f4 projView = shader.uniforms.projection * shader.uniforms.modelView;
    const bool useFlatNormal = shader.usesFlatShading();

    for (const auto& face: faces) {
        auto [tangent, bitangent] = calculateTriangleBasis(face.pts, face.uv);

        const RawVertex rawVerts[3] = {
            { face.pts[0], face.normals[0], face.uv[0] },
            { face.pts[1], face.normals[1], face.uv[1] },
            { face.pts[2], face.normals[2], face.uv[2] }
        };

        const auto clipped = clipTriangleToFrustum(rawVerts, projView);
        if (clipped.size() < 3) continue; // fully outside the view volume


        for (size_t t = 1; t + 1 < clipped.size(); ++t) {
            const RawVertex triVerts[3] = { clipped[0], clipped[t], clipped[t + 1] };

            // Flat shading uses one normal for the whole face instead of
            // each vertex's own smoothed normal, so lighting doesn't vary
            // across the triangle, faceted rather than smooth.
            const Vec3f flatNormal = useFlatNormal
                ? calculateFaceNormal(triVerts[0].pos, triVerts[1].pos, triVerts[2].pos)
                : Vec3f(0, 0, 0);

            ProcessedTriangle pt;
            for (int j = 0; j < 3; j++) {
                const Vec3f& normalToUse = useFlatNormal ? flatNormal : triVerts[j].normal;
                pt.varyings[j] = shader.vertex(triVerts[j].pos, normalToUse, triVerts[j].uv, tangent, bitangent);
            }

            const Vec3f& p0 = pt.varyings[0].screenPos;
            const Vec3f& p1 = pt.varyings[1].screenPos;
            const Vec3f& p2 = pt.varyings[2].screenPos;

            const float signedArea = (p1.x() - p0.x()) * (p2.y() - p0.y()) - (p1.y() - p0.y()) * (p2.x() - p0.x());

            if (signedArea > 0.0f) {
                processed.push_back(pt);
            }
        }
    }
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

    if (shader.isFaceNormalDrawingEnabled()) {
        drawFaceNormals(ctx, shader);
    }

    if (shader.isVertexNormalDrawingEnabled()) {
        drawVertexNormals(ctx, shader);
    }

    if (shader.isBBoxDrawingEnabled()) {
        drawBoundingBox(ctx, shader);
    }

    if (ctx.sceneRef) {
        drawCameraIcons(ctx, *ctx.sceneRef, shader);
    }
}


void drawLine(int x0, int y0, int x1, int y1, const RenderContext& ctx, const TGAColor& color) {
    bool steep = std::abs(y1 - y0) > std::abs(x1 - x0);
    if (steep) {
        std::swap(x0, y0);
        std::swap(x1, y1);
    }
    if (x0 > x1) {
        std::swap(x0, x1);
        std::swap(y0, y1);
    }
    int dx = x1 - x0;
    int dy = std::abs(y1 - y0);
    int error = dx / 2;
    int ystep = (y0 < y1) ? 1 : -1;
    int y = y0;

    for (int x = x0; x <= x1; x++) {
        int px = steep ? y : x;
        int py = steep ? x : y;

        if (px >= 0 && px < ctx.width && py >= 0 && py < ctx.height) {
            int index = (px + py * ctx.width) * 3;
            if (ctx.colorBuffer) {
                (*ctx.colorBuffer)[index]     = color.bgra[2];
                (*ctx.colorBuffer)[index + 1] = color.bgra[1];
                (*ctx.colorBuffer)[index + 2] = color.bgra[0];
            }
        }
        error -= dy;
        if (error < 0) {
            y += ystep;
            error += dx;
        }
    }
}

void drawFaceNormals(const RenderContext& ctx, IShader& shader) {
    const auto& faces = ctx.model.getFaces();
    float targetPixelLength = 15.0f;
    TGAColor normalColor = {0, 0, 255, 255};

    for (const auto& face : faces) {
        Vec4f w0 = shader.uniforms.model * Vec4f(face.pts[0]);
        Vec4f w1 = shader.uniforms.model * Vec4f(face.pts[1]);
        Vec4f w2 = shader.uniforms.model * Vec4f(face.pts[2]);

        Vec3f wp0(w0.x(), w0.y(), w0.z());
        Vec3f wp1(w1.x(), w1.y(), w1.z());
        Vec3f wp2(w2.x(), w2.y(), w2.z());

        Vec3f worldNormal = cross(wp1 - wp0, wp2 - wp0).normalize();
        Vec3f worldCenter = (wp0 + wp1 + wp2) / 3.0f;

        Vec3f viewDir = (shader.uniforms.cameraPos - worldCenter).normalize();

        if (dotProduct(worldNormal, viewDir) <= 0.0f) {
            continue;
        }

        Vec4f clipCenter = shader.uniforms.projection * shader.uniforms.modelView * shader.uniforms.model.inverse4x4() * Vec4f(worldCenter);
        if (clipCenter.w() <= 0) continue;

        Vec4f ndcCenter = clipCenter / clipCenter.w();
        Vec4f screenCenter = shader.uniforms.viewport * ndcCenter;

        Vec4f clipNormalTip = shader.uniforms.projection * shader.uniforms.modelView * shader.uniforms.model.inverse4x4() * Vec4f(worldCenter + worldNormal * 0.01f);
        if (clipNormalTip.w() <= 0) continue;

        Vec4f ndcNormalTip = clipNormalTip / clipNormalTip.w();
        Vec4f screenNormalTip = shader.uniforms.viewport * ndcNormalTip;

        Vec2f screenDir(screenNormalTip.x() - screenCenter.x(), screenNormalTip.y() - screenCenter.y());
        screenDir = screenDir.normalize();

        int endX = static_cast<int>(screenCenter.x() + screenDir.x() * targetPixelLength);
        int endY = static_cast<int>(screenCenter.y() + screenDir.y() * targetPixelLength);

        drawLine(static_cast<int>(screenCenter.x()), static_cast<int>(screenCenter.y()),
                 endX, endY, ctx, normalColor);
    }
}


void drawVertexNormals(const RenderContext& ctx, IShader& shader) {
    const auto& faces = ctx.model.getFaces();
    constexpr float targetPixelLength = 15.0f;
    constexpr TGAColor normalColor = {255, 0, 0, 255}; // red, to distinguish from face normals (blue)

    // modelView is already view * model, so this takes a local-space point
    // straight to clip space, no need to detour through world space at all.
    const Matrix4f4 projView = shader.uniforms.projection * shader.uniforms.modelView;

    for (const auto& face : faces) {
        for (int i = 0; i < 3; ++i) {
            const Vec4f clipPos = projView * Vec4f(face.pts[i]);
            if (clipPos.w() <= 0) continue;
            Vec4f screenPos = shader.uniforms.viewport * (clipPos / clipPos.w());

            const Vec4f clipTip = projView * Vec4f(face.pts[i] + face.normals[i] * 0.01f);
            if (clipTip.w() <= 0) continue;
            Vec4f screenTip = shader.uniforms.viewport * (clipTip / clipTip.w());

            Vec2f screenDir(screenTip.x() - screenPos.x(), screenTip.y() - screenPos.y());
            screenDir = screenDir.normalize();

            int endX = static_cast<int>(screenPos.x() + screenDir.x() * targetPixelLength);
            int endY = static_cast<int>(screenPos.y() + screenDir.y() * targetPixelLength);

            drawLine(static_cast<int>(screenPos.x()), static_cast<int>(screenPos.y()),
                     endX, endY, ctx, normalColor);
        }
    }
}


void drawBoundingBox(const RenderContext& ctx, IShader& shader) {
    Vec3f lMin(std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max());
    Vec3f lMax(std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest());

    for (const auto& v : ctx.model.getVertices()) {
        for (int i = 0; i < 3; ++i) {
            lMin[i] = std::min(lMin[i], v[i]);
            lMax[i] = std::max(lMax[i], v[i]);
        }
    }

    Vec3f corners[8] = {
        {lMin.x(), lMin.y(), lMin.z()}, {lMin.x(), lMin.y(), lMax.z()},
        {lMin.x(), lMax.y(), lMin.z()}, {lMin.x(), lMax.y(), lMax.z()},
        {lMax.x(), lMin.y(), lMin.z()}, {lMax.x(), lMin.y(), lMax.z()},
        {lMax.x(), lMax.y(), lMin.z()}, {lMax.x(), lMax.y(), lMax.z()}
    };

    Vec2f screenCorners[8];
    bool valid[8];

    for (int i = 0; i < 8; ++i) {
        Vec4f worldPos = shader.uniforms.model * Vec4f(corners[i]);
        Vec4f clipPos = shader.uniforms.projection * shader.uniforms.modelView * shader.uniforms.model.inverse4x4() * worldPos;

        if (clipPos.w() <= 0) {
            valid[i] = false;
            continue;
        }

        Vec4f ndc = clipPos / clipPos.w();
        Vec4f screen = shader.uniforms.viewport * ndc;
        screenCorners[i] = Vec2f(screen.x(), screen.y());
        valid[i] = true;
    }

    int edges[12][2] = {
        {0, 1}, {1, 5}, {5, 4}, {4, 0}, // Bottom face
        {2, 3}, {3, 7}, {7, 6}, {6, 2}, // Top face
        {0, 2}, {1, 3}, {4, 6}, {5, 7}  // Vertical pillars
    };

    TGAColor boxColor = {255, 0, 0, 255};

    for (int i = 0; i < 12; ++i) {
        int u = edges[i][0];
        int v = edges[i][1];
        if (valid[u] && valid[v]) {
            drawLine(static_cast<int>(screenCorners[u].x()), static_cast<int>(screenCorners[u].y()),
                     static_cast<int>(screenCorners[v].x()), static_cast<int>(screenCorners[v].y()),
                     ctx, boxColor);
        }
    }
}

void drawCameraIcons(const RenderContext& ctx, const Scene& scene, IShader& shader) {
    if (!scene.showCameraIcons) return;

    int size = 8;
    TGAColor iconColor = {255, 255, 0, 255};

    for (int i = 0; i < scene.cameras.size(); ++i) {
        if (i == scene.activeCameraIndex) continue;

        Vec4f worldPos = Vec4f(Vec3f(scene.cameras[i].pos));
        Vec4f clipPos = shader.uniforms.projection * shader.uniforms.modelView * shader.uniforms.model.inverse4x4() * worldPos;

        if (clipPos.w() <= 0) continue;

        Vec4f ndc = clipPos / clipPos.w();
        Vec4f screen = shader.uniforms.viewport * ndc;

        int cx = static_cast<int>(screen.x());
        int cy = static_cast<int>(screen.y());

        drawLine(cx - size, cy, cx + size, cy, ctx, iconColor);
        drawLine(cx, cy - size, cx, cy + size, ctx, iconColor);
    }
}
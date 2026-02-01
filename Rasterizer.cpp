#include "Rasterizer.h"

Point3 barycentric(Triangle tri, Point3 P) {
    Vec2f v0 = Vec2f(tri.pts[1].x() - tri.pts[0].x(), tri.pts[1].y() - tri.pts[0].y());
    Vec2f v1 = Vec2f(tri.pts[2].x() - tri.pts[0].x(), tri.pts[2].y() - tri.pts[0].y());
    Vec2f v2 = Vec2f(P.x() - tri.pts[0].x(), P.y() - tri.pts[0].y());

    float denom = determinant2D(v0, v1);
    if (std::abs(denom) < GraphicsUtils::EPSILON) // Degenerate triangle
        return { -1, 1, 1 };

    float beta = determinant2D(v2, v1) / denom;
    float gamma = determinant2D(v0, v2) / denom;
    float alpha = 1 - beta - gamma;

    return { alpha, beta, gamma };
}

BBox computeTriangleBBox(const Triangle &tri) {
    auto min_x = std::min(tri.pts[0].x(), std::min(tri.pts[1].x(), tri.pts[2].x()));
    auto max_x = std::max(tri.pts[0].x(), std::max(tri.pts[1].x(), tri.pts[2].x()));

    auto min_y = std::min(tri.pts[0].y(), std::min(tri.pts[1].y(), tri.pts[2].y()));
    auto max_y = std::max(tri.pts[0].y(), std::max(tri.pts[1].y(), tri.pts[2].y()));

    auto min_z = std::min(tri.pts[0].z(), std::min(tri.pts[1].z(), tri.pts[2].z()));
    auto max_z = std::max(tri.pts[0].z(), std::max(tri.pts[1].z(), tri.pts[2].z()));

    return {{min_x, min_y, min_z}, {max_x, max_y, max_z}};
}


void drawTriangle(const Triangle &tri, std::vector<float>& zbuffer,
                  TGAImage &image, const TGAImage& texture, const float intensity[3]) {
    BBox bbox = computeTriangleBBox(tri);

    int xStart = std::max(0, (int) bbox._boxMin.x());
    int xEnd = std::min(image.width() - 1, (int) bbox._boxMax.x());

    int yStart = std::max(0, (int) bbox._boxMin.y());
    int yEnd = std::min(image.height() - 1, (int) bbox._boxMax.y());

    #pragma omp parallel for
    for (int x = xStart; x <= xEnd; x++) {
        for (int y = yStart; y <= yEnd; y++) {
            Vec3f xyVec { float(x), float(y), 0.0f };
            Vec3f bc_screen = barycentric(tri, xyVec);
            if (bc_screen.x() < 0 || bc_screen.y() < 0 || bc_screen.z() < 0) continue;

            // Compute z value in screen coordinates using the barycentric coords.
            float z = 0, intensityP = 0;
            Vec2f uvAtPointP{0, 0};
            for (int i = 0; i < 3; i++) {
                z += tri.pts[i].z() * bc_screen[i];
                intensityP += intensity[i] * bc_screen[i];

                uvAtPointP.x() += tri.uv[i].x() * bc_screen[i];
                uvAtPointP.y() += tri.uv[i].y() * bc_screen[i];
            }

            intensityP = std::min(1.f, std::max(0.f, intensityP));

            if (zbuffer[x + y * image.width()] < z) {
                // Get the texture
                Vec2f texP = {uvAtPointP.x() * texture.width(), uvAtPointP.y() * texture.height()};
                TGAColor color = texture.get(texP.x(), texP.y());

                TGAColor finalColor;
                finalColor[0] = color[0] * intensityP; // Blue
                finalColor[1] = color[1] * intensityP; // Green
                finalColor[2] = color[2] * intensityP; // Red
                finalColor[3] = 255;                   // Alpha

                zbuffer[x + y * image.width()] = z;
                image.set(x, y, finalColor);
            }
        }
    }
}


void line(int ax, int ay, int bx, int by, TGAImage &framebuffer, TGAColor color) {
    bool steep = std::abs(ax - bx) < std::abs(ay - by);
    if (steep) { // if the line is steep, we transpose the image
        std::swap(ax, ay);
        std::swap(bx, by);
    }
    if (ax > bx) { // make it left−to−right
        std::swap(ax, bx);
        std::swap(ay, by);
    }

    int dx = std::abs(bx - ax);
    int dy = std::abs(by - ay);
    int error = 0;
    int y = ay;
    for (int x = ax; x <= bx; x++) {
        if (steep) {
            framebuffer.set(y, x,
                            color); // De-transpose: put Y in the X slot and X in the Y slot
        } else {
            framebuffer.set(x, y, color); // Standard draw
        }
        error += dy;
        if (error * 2 >= dx) {
            y += (by > ay ? 1 : -1);
            error -= dx;
        }
    }
}

void fillTriangle(const Triangle& tri, TGAImage &framebuffer, TGAColor color) {
    Point3 v0 = tri.pts[0];
    Point3 v1 = tri.pts[1];
    Point3 v2 = tri.pts[2];

    if (v0.y() > v1.y()) std::swap(v0, v1);
    if (v0.y() > v2.y()) std::swap(v0, v2);
    if (v1.y() > v2.y()) std::swap(v1, v2);

    int total_height = v2.y() - v0.y();
    if (total_height == 0) return;

    for (int i = 0; i < 2; i++) {
        bool upper = (i != 0);
        int segment_height = upper ? v2.y() - v1.y() : v1.y() - v0.y();
        if (segment_height == 0 && !upper) continue;
        if (segment_height == 0 && upper) break;

        for (int y = (upper ? v1.y() : v0.y()); y <= (upper ? v2.y() : v1.y()); y++) {
            int x1 = v0.x() + (v2.x() - v0.x()) * (float) (y - v0.y()) / total_height;
            int x2 = upper ? v1.x() +
                             (v2.x() - v1.x()) * (float) (y - v1.y()) / segment_height
                           : v0.x() + (v1.x() - v0.x()) * (float) (y - v0.y()) /
                                    segment_height;

            if (x1 > x2) std::swap(x1, x2);

            for (int x = std::max(0, x1);
                 x <= std::min(framebuffer.width() - 1, x2); x++) {
                framebuffer.set(x, y, color);
            }
        }
    }
}

void drawModel(const ModelLoader &model, TGAImage &framebuffer, const TGAImage& texture,
               std::vector<float>& zbuffer, const Matrix4f4 &mat, Vec3f eye) {
    const auto &faces = model.getFaces();
    Vec3f lightDir = (eye - Vec3f(0, 0, 0)).normalize();

    for (const auto &face : faces) {
        Point3 worldCoords[3], screenCoords[3];
        Vec2f uv[3];
        float verticesIntensity[3];

        for (int i = 0; i < 3; i++) {
            worldCoords[i] = face.pts[i];
            uv[i] = face.uv[i];
            Vec4f vTransformed = mat * Vec4f(worldCoords[i]);

            screenCoords[i] = Vec3f(
                    vTransformed[0] / vTransformed[3],
                    vTransformed[1] / vTransformed[3],
                    vTransformed[2] / vTransformed[3]
            );

            Vec3f n = face.normals[i];
            verticesIntensity[i] = std::max(0.f, dotProduct(n, lightDir));
        }

        Vec3f viewDir = (eye - worldCoords[0]).normalize();
        Vec3f nFace = cross(worldCoords[1] - worldCoords[0], worldCoords[2] - worldCoords[0]).normalize();

        if (dotProduct(nFace, viewDir) > 0) {
            drawTriangle(Triangle(screenCoords, uv), zbuffer, framebuffer, texture ,verticesIntensity);
        }
    }
}
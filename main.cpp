#include "ModelLoader.h"
#include "Constants.h"
#include "Geometry/Geometry.h"
#include "Rasterizer.h"

#include <cmath>
#include <algorithm>
#include <limits>

void drawModel(const ModelLoader &model, TGAImage &framebuffer,
               std::vector<float>& zbuffer, const Matrix4f4 &mat, Vec3f eye) {
    const auto &vertices = model.getVertices();
    const auto &verticesNormals = model.getVerticesNormals();
    const auto &faces = model.getFaces();

    Vec3f lightDir = (eye - Vec3f(0, 0, 0)).normalize();

    for (const auto &face : faces) {
        Point3 worldCoords[3];
        Point3 screenCoords[3];
        float verticesIntensity[3];

        for (int i = 0; i < 3; i++) {
            worldCoords[i] = vertices[i == 0 ? face.a : (i == 1 ? face.b : face.c)];
            Vec4f vTransformed = mat * Vec4f(worldCoords[i]);

            screenCoords[i] = Vec3f(
                    vTransformed[0] / vTransformed[3],
                    vTransformed[1] / vTransformed[3],
                    vTransformed[2] / vTransformed[3]
            );

            Vec3f n = verticesNormals[i == 0 ? face.na : (i == 1 ? face.nb : face.nc)];
            verticesIntensity[i] = std::max(0.f, dotProduct(n, lightDir));
        }

        Vec3f viewDir = (eye - worldCoords[0]).normalize();
        Vec3f nFace = cross(worldCoords[1] - worldCoords[0], worldCoords[2] - worldCoords[0]).normalize();

        if (dotProduct(nFace, viewDir) > 0) {
            drawTriangle(Triangle(screenCoords), zbuffer, framebuffer, verticesIntensity);
        }
    }
}

int main(int argc, char **argv) {
    constexpr float width = 800.0f;
    constexpr float height = 800.0f;

    Vec3f eye(0, 0, 10);             // Where the viewer is
    Vec3f center(0, 0, 0);           // Where the viewer is watching
    Vec3f up(0, 1, 0);               // The new up

    auto ModelView  = Matrix4f4::lookat(eye, center, up);
    auto Projection = Matrix4f4::projection(7);
    auto View       = Matrix4f4::viewport(width/8, height/8, width*3/4, height*3/4);

    auto mat = View * Projection * ModelView;

    TGAImage framebuffer(width, height, TGAImage::RGB);

    auto zbuffer = std::vector<float>(width * height);
    for (int i = 0; i < width * height; i++) {
        zbuffer[i] = -std::numeric_limits<float>::max();
    }

    ModelLoader mdl("../Models/obj/african_head/african_head.obj");
    drawModel(mdl, framebuffer, zbuffer, mat, eye);

    framebuffer.write_tga_file("output.tga");
    return 0;
}
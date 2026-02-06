#ifndef RENDERER_GEOMETRY_H
#define RENDERER_GEOMETRY_H

#include <iostream>
#include "Vec.h"

struct BBox {
    Point3 _boxMin{}, _boxMax{};

    BBox(const Point3 &Min, const Point3 &Max) : _boxMin(Min), _boxMax(Max) {}
};

struct Face {
    int vertexIndices[3] = {};
    int normalIndices[3] = {};
    int textureIndices[3] = {};

    Point3 pts[3] = {};
    Point3 normals[3] = {};
    Vec2f  uv[3] = {};

    Face() = default;
    Face(const int vIndices[3], const int nIndices[3], const int tIndices[3]) {
        for (int i = 0; i < 3; i++) {
            vertexIndices[i] = vIndices[i];
            normalIndices[i] = nIndices[i];
            textureIndices[i] = tIndices[i];
        }
    }

    void updateFace(const std::vector<Point3>& modelVertices,
                    const std::vector<Point3>& modelNormals,
                    const std::vector<Vec2f>& modelTexture) {
        for(int i = 0; i < 3; i++) {
            pts[i] = modelVertices[vertexIndices[i]];
            normals[i] = modelNormals[normalIndices[i]];
            uv[i] = modelTexture[textureIndices[i]];
        }
    }
};

#endif //RENDERER_GEOMETRY_H

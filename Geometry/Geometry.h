#ifndef RENDERER_GEOMETRY_H
#define RENDERER_GEOMETRY_H

#include <iostream>
#include "Geometry/Vec.h"
#include "Geometry/Matrix.h"

struct BBox {
    Point3 _boxMin{}, _boxMax{};

    BBox(Point3 Min, Point3 Max) : _boxMin(Min), _boxMax(Max) {}
};


struct Triangle {
    Point3 pts[3] = {};

    explicit Triangle(const Point3 other[3]) {
        for (int i = 0 ; i < 3 ; i++) {
            pts[i] = other[i];
        }
    }
};

struct Face {
    int a, b, c;
    int na, nb, nc;

    Face() = default;
    Face(const int verticesIndices[3], const int normalIndices[3]) :
            a(verticesIndices[0]), b(verticesIndices[1]), c(verticesIndices[2]),
            na(normalIndices[0]), nb(normalIndices[1]), nc(normalIndices[2])
    {}
};

// Matrix

#endif //RENDERER_GEOMETRY_H

#ifndef RENDERER_MODELLOADER_H
#define RENDERER_MODELLOADER_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "Geometry/Geometry.h"

namespace fs = std::filesystem;

class ModelLoader {

public:
    explicit ModelLoader(const std::string& fileName) {
        loadFile(fileName);
    }

    const std::vector<Face>& getFaces() const { return faces; }
    const std::vector<Vec3f>& getVertices() const { return vertices; }
    const std::vector<Vec3f>& getVerticesNormals() const { return normals; }

private:
    void loadFile(const std::string fileName);

    std::vector<Face> faces;
    std::vector<Point3> vertices;
    std::vector<Vec3f> normals;
};

#endif //RENDERER_MODELLOADER_H

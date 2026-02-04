#ifndef RENDERER_MODELLOADER_H
#define RENDERER_MODELLOADER_H

#include <fstream>
#include <iostream>
#include <sstream>
#include <string>

#include "../Math/Geometry.h"

namespace fs = std::filesystem;

class ModelLoader {

public:
    explicit ModelLoader(const std::string& fileName) {
        loadFile(fileName);
        for (auto &face: faces) {
            face.updateFace(vertices, normals, textures);
        }
    }

    const std::vector<Face>& getFaces() const { return faces; }
    std::vector<Face>& getFaces() { return faces; }
    const std::vector<Vec3f>& getVertices() const { return vertices; }
    const std::vector<Vec3f>& getVerticesNormals() const { return normals; }
    const std::vector<Vec2f>& getVerticesTexture() const { return textures; }

private:
    void loadFile(const std::string &fileName);

    std::vector<Face> faces;
    std::vector<Point3> vertices;
    std::vector<Vec3f> normals;
    std::vector<Vec2f> textures;
};

#endif //RENDERER_MODELLOADER_H

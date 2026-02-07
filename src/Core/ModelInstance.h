#ifndef RENDERER_MODELINSTANCE_H
#define RENDERER_MODELINSTANCE_H

#include "Matrix.h"
#include "../IO/tgaimage.h"
#include "../IO/ModelLoader.h"

struct ModelInstance {
    ModelLoader model;
    TGAImage diffuse;
    TGAImage normal;
    TGAImage specular;
    bool useAlphaTest;

    Vec3f position = {0, 0, 0};
    Vec3f rotation = {0, 0, 0}; // Euler angles in degrees
    Vec3f scale = {1, 1, 1};

    ModelInstance(const std::string& modelRoot,
                  const std::string &objPath,
                  const std::string &diffPath,
                  const std::string &nmPath,
                  const std::string &specPath,
                  const bool useAlphaTest)
        : model(modelRoot + objPath), useAlphaTest(useAlphaTest) {

        diffuse.read_tga_file(modelRoot + diffPath);
        normal.read_tga_file(modelRoot + nmPath);
        specular.read_tga_file(modelRoot + specPath);

        diffuse.flip_vertically();
        normal.flip_vertically();
        specular.flip_vertically();
    }

    Matrix4f4 getModelMatrix() const {
        const Matrix4f4 T = Matrix4f4::translation(position);
        const Matrix4f4 S = Matrix4f4::scale(scale.x(), scale.y(), scale.z());

        // Rotation Order: Z -> Y -> X
        const Matrix4f4 Rx = Matrix4f4::rotationX(rotation.x());
        const Matrix4f4 Ry = Matrix4f4::rotationY(rotation.y());
        const Matrix4f4 Rz = Matrix4f4::rotationZ(rotation.z());

        return T * (Rx * Ry * Rz) * S;
    }
};

#endif //RENDERER_MODELINSTANCE_H
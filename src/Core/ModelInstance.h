#ifndef RENDERER_MODELINSTANCE_H
#define RENDERER_MODELINSTANCE_H

#include "Matrix.h"
#include "../IO/tgaimage.h"
#include "../IO/ModelLoader.h"

struct AABB {
    Vec3f min;
    Vec3f max;

    AABB() {
        min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        max = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
    }
};

struct ModelInstance {
    ModelLoader model;
    TGAImage diffuse;
    TGAImage normal;
    TGAImage specular;
    bool useAlphaTest;
    bool isDeletable = true;
    AABB bbox;

    Vec3f position = {0, 0, 0};
    Vec3f rotation = {0, 0, 0}; // Euler angles in degrees
    Vec3f scale = {1, 1, 1};

    ModelInstance(const std::string& modelRoot,
                  const std::string &objPath,
                  const std::string &diffPath,
                  const std::string &nmPath,
                  const std::string &specPath,
                  const bool useAlphaTest)
        : model(modelRoot + objPath), useAlphaTest(useAlphaTest), bbox() {

        diffuse.read_tga_file(modelRoot + diffPath);
        normal.read_tga_file(modelRoot + nmPath);
        specular.read_tga_file(modelRoot + specPath);

        diffuse.flip_vertically();
        normal.flip_vertically();
        specular.flip_vertically();
    }

    [[nodiscard]] Matrix4f4 getModelMatrix() const {
        const Matrix4f4 T = Matrix4f4::translation(position);
        const Matrix4f4 S = Matrix4f4::scale(scale.x(), scale.y(), scale.z());

        // Rotation Order: Z -> Y -> X
        const Matrix4f4 Rx = Matrix4f4::rotationX(rotation.x());
        const Matrix4f4 Ry = Matrix4f4::rotationY(rotation.y());
        const Matrix4f4 Rz = Matrix4f4::rotationZ(rotation.z());

        return T * (Rx * Ry * Rz) * S;
    }

    void updateBBox() {
        for (const auto& v: model.getVertices()) {
            for (int i = 0 ; i < 3; ++i) {
                bbox.min[i] = std::min(bbox.min[i], v[i]);
                bbox.max[i] = std::max(bbox.max[i], v[i]);
            }
        }
    }

    [[nodiscard]] AABB getWorldAABB() const {
        const Matrix4f4 modelMat = getModelMatrix();

        Vec3f localCorners[8] = {
            {bbox.min.x(), bbox.min.y(), bbox.min.z()}, {bbox.min.x(), bbox.min.y(), bbox.max.z()},
            {bbox.min.x(), bbox.max.y(), bbox.min.z()}, {bbox.min.x(), bbox.max.y(), bbox.max.z()},
            {bbox.max.x(), bbox.min.y(), bbox.min.z()}, {bbox.max.x(), bbox.min.y(), bbox.max.z()},
            {bbox.max.x(), bbox.max.y(), bbox.min.z()}, {bbox.max.x(), bbox.max.y(), bbox.max.z()}
        };

        auto worldAABB = AABB();

        for (auto corner : localCorners) {
            auto homogeneousCorner = Vec4f(corner);
            Vec4f transformed = modelMat * homogeneousCorner;

            auto worldCorner = Vec3f(transformed);
            for (int j = 0; j < 3; ++j) {
                worldAABB.min[j] = std::min(worldAABB.min[j], worldCorner[j]);
                worldAABB.max[j] = std::max(worldAABB.max[j], worldCorner[j]);
            }
        }

        return worldAABB;
    }

    [[nodiscard]] static float RayBoxInterSection(Vec3f rayOrigin, Vec3f rayDir, Vec3f min, Vec3f max) {
        float tNear = std::numeric_limits<float>::lowest();
        float tFar = std::numeric_limits<float>::max();

        for (int i = 0; i < 3; i++) {
            float invDir = 1.0f / rayDir[i];
            float t1 = (min[i] - rayOrigin[i]) * invDir;
            float t2 = (max[i] - rayOrigin[i]) * invDir;

            float tStart = std::min(t1, t2);
            float tEnd = std::max(t1, t2);

            tNear = std::max(tNear, tStart);
            tFar = std::min(tFar, tEnd);

            if (tNear > tFar) return -1.0f;
        }

        return (tFar >= 0) ? tNear : -1.0f;
    }
};

#endif //RENDERER_MODELINSTANCE_H
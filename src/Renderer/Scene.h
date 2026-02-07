#ifndef RENDERER_SCENE_H
#define RENDERER_SCENE_H

#include <vector>
#include "../Core/Camera.h"
#include "../Core/ModelInstance.h"

struct Scene {
    std::vector<ModelInstance> models;
    Camera camera;

    Vec3f lightDir;
    Vec3f lightColor = { 1, 1, 1};
    Vec3f lightPos = { 1, 1, 1 };

    Scene(const Camera& cam, const Vec3f& lightDir, const Vec3f& lightPos)
        : camera(cam), lightDir(lightDir), lightPos(lightPos) {}

    void addModel(const ModelInstance& model) {
        models.push_back(model);
    }
};

#endif //RENDERER_SCENE_H
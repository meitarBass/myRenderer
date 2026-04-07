#ifndef RENDERER_SCENE_H
#define RENDERER_SCENE_H

#include <vector>
#include "../Core/Camera.h"
#include "../Core/ModelInstance.h"


struct Scene {
    std::vector<ModelInstance> models;

    std::vector<Camera> cameras;
    int activeCameraIndex = 0;

    Vec3f lightDir;
    Vec3f lightColor = { 1, 1, 1};
    Vec3f lightPos = { 1, 1, 1 };

    bool useShadows = true;
    bool useSSAO = true;

    Scene(const Camera& cam, const Vec3f& lightDir, const Vec3f& lightPos)
        : cameras(), lightDir(lightDir), lightPos(lightPos) {
        cameras.push_back(cam);
    }

    void addModel(ModelInstance& model) {
        model.updateBBox();
        models.push_back(model);
    }

    Camera& getActiveCamera() { return cameras[activeCameraIndex]; }
    const Camera& getActiveCamera() const { return cameras[activeCameraIndex]; }
};

#endif //RENDERER_SCENE_H
#ifndef RENDERER_SCENE_H
#define RENDERER_SCENE_H

#include <vector>
#include "../Core/Camera.h"
#include "../Core/ModelInstance.h"
#include "../Core/Light.h"


struct Scene {
    std::vector<ModelInstance> models;

    std::vector<Camera> cameras;
    int activeCameraIndex = 0;
    int activeModelIndex = -1; // -1 means no model is currently selected

    // Every light in the scene. Index 0 is the one that casts the shadow.
    std::vector<Light> lights = { Light{} };

    // Ambient light, scene-wide, applies to every model regardless of its
    // own material.
    Vec3f ambientLight = { 0.3f, 0.3f, 0.3f };

    bool useShadows = true;
    bool useSSAO = true;
    bool useFrustumCulling = true;

    // Fades distant surfaces toward fogColor based on distance from the
    // camera, see PhongShader::fragment(). fogStart is where fog begins to
    // show up, fogEnd is where it's fully opaque, matching a default scene
    // scale of a few units across.
    bool useFog = false;
    Vec3f fogColor = { 0.6f, 0.6f, 0.7f };
    float fogStart = 5.0f;
    float fogEnd = 15.0f;

    // Renders at a higher internal resolution and downsamples before
    // display, smoothing jagged edges at the cost of extra render time.
    bool useSupersampling = false;

    // Blurs the entire final image with a gaussian filter, a simple soften
    // pass independent of bloom below.
    bool useFullScreenBlur = false;

    // Bloom extracts pixels brighter than bloomThreshold, blurs just those,
    // and adds the glow back onto the original image, scaled by
    // bloomIntensity. Gives bright areas (emissive materials, strong
    // highlights) a soft halo instead of a hard cutoff.
    bool useBloom = false;
    float bloomThreshold = 0.7f;
    float bloomIntensity = 0.8f;

    bool useSkybox = false;
    Vec3f skyHorizonColor = { 0.8f, 0.85f, 0.9f };
    Vec3f skyZenithColor = { 0.25f, 0.45f, 0.75f };

    // Depth of field: pixels within focusRange of focusDistance stay sharp,
    // pixels dofBlurDistance past that range (nearer or farther) are fully
    // blurred, everything in between blends smoothly.
    bool useDepthOfField = false;
    float focusDistance = 6.0f;
    float focusRange = 1.5f;
    float dofBlurDistance = 4.0f;


    bool useOpenGLRenderPath = false;

    bool showCameraIcons = true;

    // How far a drag control moves per pixel dragged in the UI. Rotation
    // fields scale this up since they're in degrees, not scene units.
    float transformStepSize = 0.05f;

    explicit Scene(const Camera& cam) : cameras() {
        cameras.push_back(cam);
    }

    void addModel(ModelInstance& model) {
        model.updateBBox();
        models.push_back(model);
    }

    Camera& getActiveCamera() { return cameras[activeCameraIndex]; }
    const Camera& getActiveCamera() const { return cameras[activeCameraIndex]; }

    [[nodiscard]] bool hasActiveModel() const {
        return activeModelIndex >= 0 && activeModelIndex < static_cast<int>(models.size());
    }

    ModelInstance* getActiveModel() { return hasActiveModel() ? &models[activeModelIndex] : nullptr; }
    [[nodiscard]] const ModelInstance* getActiveModel() const { return hasActiveModel() ? &models[activeModelIndex] : nullptr; }
};

#endif //RENDERER_SCENE_H
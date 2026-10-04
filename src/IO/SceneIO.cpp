#include "SceneIO.h"
#include "../../external/json/json.hpp"
#include <fstream>
#include <algorithm>

using json = nlohmann::json;

namespace {
    json vec3ToJson(const Vec3f& v) { return json::array({v.x(), v.y(), v.z()}); }
    Vec3f jsonToVec3(const json& j) {
        return Vec3f(j.at(0).get<float>(), j.at(1).get<float>(), j.at(2).get<float>());
    }
}

namespace SceneIO {

bool saveScene(const std::string& path, const Scene& scene) {
    json j;

    j["ambientLight"] = vec3ToJson(scene.ambientLight);
    j["lights"] = json::array();
    for (const auto& light : scene.lights) {
        json jl;
        jl["type"] = light.type == LightType::Point ? "point" : "directional";
        jl["direction"] = vec3ToJson(light.direction);
        jl["position"] = vec3ToJson(light.position);
        jl["color"] = vec3ToJson(light.color);
        jl["intensity"] = light.intensity;
        j["lights"].push_back(jl);
    }

    json& st = j["settings"];
    st["useShadows"] = scene.useShadows;
    st["useSSAO"] = scene.useSSAO;
    st["useFrustumCulling"] = scene.useFrustumCulling;
    st["useFog"] = scene.useFog;
    st["fogColor"] = vec3ToJson(scene.fogColor);
    st["fogStart"] = scene.fogStart;
    st["fogEnd"] = scene.fogEnd;
    st["useSupersampling"] = scene.useSupersampling;
    st["useFullScreenBlur"] = scene.useFullScreenBlur;
    st["useBloom"] = scene.useBloom;
    st["bloomThreshold"] = scene.bloomThreshold;
    st["bloomIntensity"] = scene.bloomIntensity;
    st["useSkybox"] = scene.useSkybox;
    st["skyHorizonColor"] = vec3ToJson(scene.skyHorizonColor);
    st["skyZenithColor"] = vec3ToJson(scene.skyZenithColor);
    st["useDepthOfField"] = scene.useDepthOfField;
    st["focusDistance"] = scene.focusDistance;
    st["focusRange"] = scene.focusRange;
    st["dofBlurDistance"] = scene.dofBlurDistance;
    st["transformStepSize"] = scene.transformStepSize;

    j["activeCameraIndex"] = scene.activeCameraIndex;
    j["cameras"] = json::array();
    for (const auto& cam : scene.cameras) {
        json jc;
        jc["pos"] = vec3ToJson(cam.pos);
        jc["lookAt"] = vec3ToJson(cam.lookAt);
        jc["up"] = vec3ToJson(cam.up);
        jc["focalLength"] = cam.focalLength;
        jc["yaw"] = cam.yaw;
        jc["pitch"] = cam.pitch;
        j["cameras"].push_back(jc);
    }

    j["models"] = json::array();
    for (const auto& model : scene.models) {
        json jm;
        jm["modelRoot"] = model.resource->modelRoot;
        jm["objPath"] = model.resource->objPath;
        jm["diffPath"] = model.resource->diffPath;
        jm["nmPath"] = model.resource->nmPath;
        jm["specPath"] = model.resource->specPath;

        jm["position"] = vec3ToJson(model.position);
        jm["rotation"] = vec3ToJson(model.rotation);
        jm["scale"] = vec3ToJson(model.scale);

        jm["useAlphaTest"] = model.useAlphaTest;
        jm["isDeletable"] = model.isDeletable;
        jm["useDiffuse"] = model.useDiffuse;
        jm["useSpecularMap"] = model.useSpecularMap;
        jm["useNormalMap"] = model.useNormalMap;
        jm["useWireframe"] = model.useWireframe;
        jm["fillColor"] = model.fillColor;
        jm["useBlending"] = model.useBlending;
        jm["opacity"] = model.material.opacity;

        j["models"].push_back(jm);
    }

    std::ofstream out(path);
    if (!out) return false;
    out << j.dump(2);
    return static_cast<bool>(out);
}

bool loadScene(const std::string& path, Scene& scene,
               std::map<std::string, std::shared_ptr<ModelResource>>& resourceCache,
               std::string* errorOut) {
    std::ifstream in(path);
    if (!in) {
        if (errorOut) *errorOut = "Could not open " + path;
        return false;
    }

    json j;
    try {
        in >> j;
    } catch (const std::exception& e) {
        if (errorOut) *errorOut = std::string("Invalid JSON: ") + e.what();
        return false;
    }

    try {
        std::vector<Camera> newCameras;
        for (const auto& jc : j.at("cameras")) {
            Camera cam(jsonToVec3(jc.at("pos")), jsonToVec3(jc.at("lookAt")),
                       jsonToVec3(jc.at("up")), jc.at("focalLength").get<float>());
            cam.yaw = jc.value("yaw", -90.0);
            cam.pitch = jc.value("pitch", 0.0);
            newCameras.push_back(cam);
        }
        if (newCameras.empty()) {
            if (errorOut) *errorOut = "Scene file has no cameras";
            return false;
        }

        std::vector<ModelInstance> newModels;
        newModels.reserve(j.at("models").size());
        for (const auto& jm : j.at("models")) {
            const std::string modelRoot = jm.at("modelRoot").get<std::string>();
            const std::string objPath = jm.at("objPath").get<std::string>();
            const std::string diffPath = jm.at("diffPath").get<std::string>();
            const std::string nmPath = jm.at("nmPath").get<std::string>();
            const std::string specPath = jm.at("specPath").get<std::string>();

            const std::string resourceKey = modelRoot + objPath;
            auto it = resourceCache.find(resourceKey);
            if (it == resourceCache.end()) {
                it = resourceCache.emplace(
                    resourceKey,
                    std::make_shared<ModelResource>(modelRoot, objPath, diffPath, nmPath, specPath)
                ).first;
            }

            ModelInstance model(it->second, jm.value("useAlphaTest", false));
            model.position = jsonToVec3(jm.at("position"));
            model.rotation = jsonToVec3(jm.at("rotation"));
            model.scale = jsonToVec3(jm.at("scale"));
            model.isDeletable = jm.value("isDeletable", true);
            model.useDiffuse = jm.value("useDiffuse", true);
            model.useSpecularMap = jm.value("useSpecularMap", true);
            model.useNormalMap = jm.value("useNormalMap", true);
            model.useWireframe = jm.value("useWireframe", false);
            model.fillColor = jm.value("fillColor", true);
            model.useBlending = jm.value("useBlending", false);
            model.material.opacity = jm.value("opacity", 1.0f);
            model.updateBBox();
            newModels.push_back(std::move(model));
        }

        // Lights and scene settings, parsed into locals first so a malformed
        // file still can't leave the live scene half-updated.
        std::vector<Light> newLights;
        for (const auto& jl : j.at("lights")) {
            Light light;
            light.type = jl.value("type", std::string("directional")) == "point" ? LightType::Point : LightType::Directional;
            light.direction = jsonToVec3(jl.at("direction"));
            light.position = jsonToVec3(jl.at("position"));
            light.color = jsonToVec3(jl.at("color"));
            light.intensity = jl.value("intensity", 1.0f);
            newLights.push_back(light);
        }
        const Vec3f newAmbient = jsonToVec3(j.at("ambientLight"));
        const json& st = j.at("settings");
        // Defaults for any setting missing from the file: the current scene's values.
        struct {
            bool useShadows, useSSAO, useFrustumCulling, useFog, useSupersampling, useFullScreenBlur,
                 useBloom, useSkybox, useDepthOfField;
            Vec3f fogColor, skyHorizonColor, skyZenithColor;
            float fogStart, fogEnd, bloomThreshold, bloomIntensity, focusDistance, focusRange,
                  dofBlurDistance, transformStepSize;
        } settings = {
            scene.useShadows, scene.useSSAO, scene.useFrustumCulling, scene.useFog, scene.useSupersampling,
            scene.useFullScreenBlur, scene.useBloom, scene.useSkybox, scene.useDepthOfField,
            scene.fogColor, scene.skyHorizonColor, scene.skyZenithColor,
            scene.fogStart, scene.fogEnd, scene.bloomThreshold, scene.bloomIntensity, scene.focusDistance,
            scene.focusRange, scene.dofBlurDistance, scene.transformStepSize
        };
        settings.useShadows = st.value("useShadows", settings.useShadows);
        settings.useSSAO = st.value("useSSAO", settings.useSSAO);
        settings.useFrustumCulling = st.value("useFrustumCulling", settings.useFrustumCulling);
        settings.useFog = st.value("useFog", settings.useFog);
        if (st.contains("fogColor")) settings.fogColor = jsonToVec3(st.at("fogColor"));
        settings.fogStart = st.value("fogStart", settings.fogStart);
        settings.fogEnd = st.value("fogEnd", settings.fogEnd);
        settings.useSupersampling = st.value("useSupersampling", settings.useSupersampling);
        settings.useFullScreenBlur = st.value("useFullScreenBlur", settings.useFullScreenBlur);
        settings.useBloom = st.value("useBloom", settings.useBloom);
        settings.bloomThreshold = st.value("bloomThreshold", settings.bloomThreshold);
        settings.bloomIntensity = st.value("bloomIntensity", settings.bloomIntensity);
        settings.useSkybox = st.value("useSkybox", settings.useSkybox);
        if (st.contains("skyHorizonColor")) settings.skyHorizonColor = jsonToVec3(st.at("skyHorizonColor"));
        if (st.contains("skyZenithColor")) settings.skyZenithColor = jsonToVec3(st.at("skyZenithColor"));
        settings.useDepthOfField = st.value("useDepthOfField", settings.useDepthOfField);
        settings.focusDistance = st.value("focusDistance", settings.focusDistance);
        settings.focusRange = st.value("focusRange", settings.focusRange);
        settings.dofBlurDistance = st.value("dofBlurDistance", settings.dofBlurDistance);
        settings.transformStepSize = st.value("transformStepSize", settings.transformStepSize);

        // Everything parsed successfully -- now actually replace the scene.
        scene.cameras = std::move(newCameras);
        const int savedIndex = j.value("activeCameraIndex", 0);
        scene.activeCameraIndex = std::clamp(savedIndex, 0, static_cast<int>(scene.cameras.size()) - 1);
        scene.lights = std::move(newLights);
        scene.ambientLight = newAmbient;
        scene.useShadows = settings.useShadows;
        scene.useSSAO = settings.useSSAO;
        scene.useFrustumCulling = settings.useFrustumCulling;
        scene.useFog = settings.useFog;
        scene.fogColor = settings.fogColor;
        scene.fogStart = settings.fogStart;
        scene.fogEnd = settings.fogEnd;
        scene.useSupersampling = settings.useSupersampling;
        scene.useFullScreenBlur = settings.useFullScreenBlur;
        scene.useBloom = settings.useBloom;
        scene.bloomThreshold = settings.bloomThreshold;
        scene.bloomIntensity = settings.bloomIntensity;
        scene.useSkybox = settings.useSkybox;
        scene.skyHorizonColor = settings.skyHorizonColor;
        scene.skyZenithColor = settings.skyZenithColor;
        scene.useDepthOfField = settings.useDepthOfField;
        scene.focusDistance = settings.focusDistance;
        scene.focusRange = settings.focusRange;
        scene.dofBlurDistance = settings.dofBlurDistance;
        scene.transformStepSize = settings.transformStepSize;
        scene.models = std::move(newModels);
    } catch (const std::exception& e) {
        if (errorOut) *errorOut = std::string("Malformed scene file: ") + e.what();
        return false;
    }

    return true;
}

}

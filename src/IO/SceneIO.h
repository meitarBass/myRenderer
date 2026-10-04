#ifndef RENDERER_SCENEIO_H
#define RENDERER_SCENEIO_H

#include <string>
#include <map>
#include <memory>
#include "../Renderer/Scene.h"
#include "../Core/ModelInstance.h"

namespace SceneIO {
    bool saveScene(const std::string& path, const Scene& scene);
    bool loadScene(const std::string& path, Scene& scene,
                   std::map<std::string, std::shared_ptr<ModelResource>>& resourceCache,
                   std::string* errorOut = nullptr);
}

#endif //RENDERER_SCENEIO_H

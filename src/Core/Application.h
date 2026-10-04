#ifndef RENDERER_APPLICATION_H
#define RENDERER_APPLICATION_H

#include "ModelInstance.h"
#include "BVH.h"
#include "../Renderer/Renderer.h"
#include "../Shaders/ScreenShader.h"
#include "../IO/SceneIO.h"

#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <utility>
#include <map>
#include <array>


struct PickRay {
    Vec3f origin;
    Vec3f direction;
};

struct GLModelBuffers {
    unsigned int vao = 0;
    unsigned int vbo = 0;
    int vertexCount = 0;
    unsigned int diffuseTex = 0;
    unsigned int specularTex = 0;
    unsigned int normalTex = 0;
    bool hasDiffuseTex = false;
    bool hasSpecularTex = false;
    bool hasNormalTex = false;
};

struct Application {
    static constexpr int SUPERSAMPLE_FACTOR = 2;

    Application(const int w, const int h, const char* name)
        : width(w), height(h), appName(name),
          rb (RenderBuffers{width, height, width, height}),
          ssRb (RenderBuffers{width * SUPERSAMPLE_FACTOR, height * SUPERSAMPLE_FACTOR, width, height}),
          scene({{0, 1, 6}, {0, 0, 0}, {0, 1, 0}, 3.0f})
    {}

    bool init();
    void run();

    GLFWwindow* window;
    void resizeBuffers(int newWidth, int newHeight);


private:
    int width, height;
    const char *appName;

    std::map<std::string, std::shared_ptr<ModelResource>> resourceCache;
    RenderBuffers rb;
    RenderBuffers ssRb;
    Scene scene;

    unsigned int shaderProgram, VAO, texture;

    unsigned int glModelShaderProgram = 0;
    std::map<ModelResource*, GLModelBuffers> glResourceCache;
    unsigned int envCubemap = 0;

    double lastX = 400.0f;
    double lastY = 400.0f;

    bool firstMouse = true;

    // Hold R to rotate the active model by dragging, mirrors the yaw/pitch
    // deltas the camera look already uses below.
    bool firstMouseModelRotate = true;
    double lastModelRotateX = 0.0;
    double lastModelRotateY = 0.0;

    // Hold M to move the active model by dragging. Unlike rotation this
    // isn't a pixel-delta hack, the mouse ray is intersected against a
    // camera-facing plane through the model each frame, so the model
    // tracks the cursor 1:1 in world space regardless of zoom or distance.
    bool firstMouseModelDrag = true;
    Vec3f lastDragHit;
    Vec3f dragPlanePoint;

    // Hold N to scale the active model by dragging vertically.
    bool firstMouseModelScale = true;
    double lastModelScaleY = 0.0;

    // Scene save/load (see SceneIO.h). sceneFilePathBuf is the ImGui text
    // input's backing buffer; sceneIOStatus is the last save/load result,
    // shown next to the buttons.
    std::array<char, 256> sceneFilePathBuf = {"scene.json"};
    std::string sceneIOStatus;

    bool initWindow();
    void setupShaders();
    void setupBuffers();
    void buildScene();

    void setupGLModelShader();
    GLModelBuffers& ensureGLResources(ModelResource& resource);
    static unsigned int uploadGLTexture(TGAImage& img);

    unsigned int ensureEnvironmentCubemap();
    void renderModelsGL();

    static Matrix4f4 buildStandardGLProjection(const Camera& cam, float aspect);

    void processMouseInput(double xPos, double yPos);

    void downsampleSupersampleBuffer();
    static void mouse_callback(GLFWwindow *window, double xPos, double yPos);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    static void framebuffer_size_callback(GLFWwindow* window, int width, int height);
    PickRay screenToWorldRay(double mouseX, double mouseY);
    void addModelToScene(const std::string& folderPath,
                         const std::string& objFile,
                         const std::string& diffFile,
                         const std::string& nmFile,
                         const std::string& specFile);

    void saveSceneToFile(const std::string& path);
    void loadSceneFromFile(const std::string& path);
};

#endif //RENDERER_APPLICATION_H

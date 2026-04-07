#ifndef RENDERER_APPLICATION_H
#define RENDERER_APPLICATION_H

#include "ModelInstance.h"
#include "../Renderer/Renderer.h"
#include "../Shaders/ScreenShader.h"

#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <cmath>
#include <utility>

struct Application {
    Application(const int w, const int h, const char* name)
        : width(w), height(h), appName(name),
          rb (RenderBuffers{width, height, width, height}),
          scene({{0, 2, 6}, {0, 0, 0}, {0, 1, 0}, 3.0f}, (Vec3f(2, 3, 3).normalize() * 5.0f).normalize(), Vec3f(2, 3, 3).normalize() * 5.0f)
    {}

    bool init();
    void run();

    GLFWwindow* window;

private:
    int width, height;
    const char *appName;

    RenderBuffers rb;
    Scene scene;
    unsigned int shaderProgram, VAO, texture;

    double lastX = 400.0f;
    double lastY = 400.0f;
    double yaw = -90.0f;
    double pitch = 0.0f;
    bool firstMouse = true;

    bool initWindow();
    void setupShaders();
    void setupBuffers();
    void buildScene();

    void processMouseInput(double xPos, double yPos);
    static void mouse_callback(GLFWwindow *window, double xPos, double yPos);
    static void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
    Vec3f screenToWorldRay(double mouseX, double mouseY);
};

#endif //RENDERER_APPLICATION_H

#include <iostream>
#include "src/Core/ModelInstance.h"
#include "src/Renderer/Renderer.h"
#include "tests/RendererUnitTests.h"
#define GL_SILENCE_DEPRECATION
#include <glad/glad.h>
#include <GLFW/glfw3.h>

const char *vertexShaderSource = "#version 410 core\n"
    "layout (location = 0) in vec3 aPos;\n"
    "layout (location = 1) in vec2 aTexCoord;\n"
    "out vec2 TexCoord;\n"
    "void main()\n"
    "{\n"
    "   gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);\n"
    "   TexCoord = aTexCoord;\n"
    "}\0";

const char *fragmentShaderSource = "#version 410 core\n"
    "out vec4 FragColor;\n"
    "in vec2 TexCoord;\n"
    "uniform sampler2D screenTexture;\n"
    "void main()\n"
    "{\n"
    "   FragColor = texture(screenTexture, TexCoord);\n"
    "}\0";

int runGLFW() {
    float vertices[] = {
        1.0f,  1.0f, 0.0f, 1.0f, 1.0f,  // top right
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom right
       -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
       -1.0f,  1.0f, 0.0f, 0.0f, 1.0f  // top left
    };

    unsigned int indices[] = {
        0, 1, 3,   // first triangle
        1, 2, 3    // second triangle
    };

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return -1;
    }

    // TODO: Check why we need this
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    GLFWwindow* window = glfwCreateWindow(800, 800, "Black Window", nullptr, nullptr);
    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return -1;
    }

    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    unsigned int VBO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO); // FROM NOW ON WE USE VBO
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    unsigned int EBO;
    glGenBuffers(1, &EBO);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    const unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    const unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    const unsigned int shaderProgram = glCreateProgram();

    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    int success;
    char infoLog[512];

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "VERTEX SHADER ERROR:\n" << infoLog << std::endl;
    }

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "FRAGMENT SHADER ERROR:\n" << infoLog << std::endl;
    }

    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
        std::cout << "PROGRAM LINK ERROR:\n" << infoLog << std::endl;
    }

    glUseProgram(shaderProgram);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) 0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *) (3 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);


    ///
    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 800, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "screenTexture"), 0);
    ///


    constexpr int width = 800;
    constexpr int height = 800;

    const auto cam = Camera({0, 2, 6}, {0,0,0}, {0, 1, 0}, 3.0);
    Vec3f lightPos = Vec3f(2, 3, 3).normalize() * 5.0f;
    const auto lightDir = (lightPos - cam.lookAt).normalize();
    auto rb = RenderBuffers(width, height, width, height);
    auto scene = Scene(cam, lightDir, lightPos);

    const std::string diabloRoot = "../Models/obj/diablo3_pose/";
    const std::string headRoot = "../Models/obj/african_head/";
    constexpr std::string floorRoot = "../Models/obj/";

    auto diabloModel = ModelInstance(diabloRoot, "diablo3_pose.obj", "diablo3_pose_diffuse.tga", "diablo3_pose_nm_tangent.tga", "diablo3_pose_spec.tga", false);
    diabloModel.rotation = {0, 40, 0};
    diabloModel.scale = {0.55, 0.55, 0.55};
    diabloModel.position = {-0.5, -0.42, -1.5};

    scene.addModel(diabloModel);

    diabloModel.rotation = {0, -50, 0};
    diabloModel.scale = {0.45, 0.45, 0.45};
    diabloModel.position = {1.3, -0.53, -2.8};

    scene.addModel(diabloModel);

    diabloModel.rotation = {0, 100, 0};
    diabloModel.scale = {0.35, 0.35, 0.35};
    diabloModel.position = {-1.15, -0.60, -0.1};

    scene.addModel(diabloModel);

    auto head = ModelInstance(headRoot , "african_head.obj", "african_head_diffuse.tga", "african_head_nm_tangent.tga", "african_head_spec.tga", false);
    auto eyes_in = ModelInstance(headRoot , "african_head_eye_inner.obj", "african_head_eye_inner_diffuse.tga", "african_head_eye_inner_nm_tangent.tga", "african_head_eye_inner_spec.tga", false);
    head.rotation = {-20, 10,0 };
    eyes_in.rotation = {-20, 10, 0};

    head.scale = {0.6, 0.6, 0.6};
    eyes_in.scale = {0.6, 0.6, 0.6};

    head.position = {0, -0.8, -1.2};
    eyes_in.position = {0, -0.8, -1.2};

    scene.addModel(head);
    scene.addModel(eyes_in);

    auto floorModel = ModelInstance(floorRoot, "floor.obj", "floor_diffuse.tga", "floor_nm_tangent.tga", "floor_spec.tga", false);
    floorModel.scale = {2.0, 1.0, 2.0};
    floorModel.position = {0.0, 0.0, -2.0};

    scene.addModel(floorModel);

    while (!glfwWindowShouldClose(window)) {
        float speed = 0.1f;
        Vec3f forward = (scene.camera.lookAt - scene.camera.pos).normalize();
        Vec3f right = cross(forward, scene.camera.up).normalize();

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            scene.camera.pos = scene.camera.pos + (forward * speed);
            scene.camera.lookAt = scene.camera.lookAt + (forward * speed);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            scene.camera.pos = scene.camera.pos - (forward * speed);
            scene.camera.lookAt = scene.camera.lookAt - (forward * speed);
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            scene.camera.pos = scene.camera.pos + (right * speed);
            scene.camera.lookAt = scene.camera.lookAt + (right * speed);
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            scene.camera.pos = scene.camera.pos - (right * speed);
            scene.camera.lookAt = scene.camera.lookAt - (right * speed);
        }

        Renderer::render(scene, rb);

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 800, 800, GL_RGB, GL_UNSIGNED_BYTE, rb.colorBuffer.data());

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwDestroyWindow(window);
    glfwTerminate();
    return 0;
}

int main() {
    RendererUnitTests::runAll();

    /// RUNNING GLFW FROM HERE ///
    return runGLFW();
}

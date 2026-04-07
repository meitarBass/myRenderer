#include "Application.h"

#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui_impl_glfw.h"
#include "../external/imgui/imgui_impl_opengl3.h"

#include <filesystem>
namespace fs = std::filesystem;

bool Application::initWindow() {
    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW" << std::endl;
        return false;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);

    window = glfwCreateWindow(width, height, appName, nullptr, nullptr);

    if (!window) {
        std::cerr << "Failed to create GLFW window\n";
        glfwTerminate();
        return false;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();

    ImGui_ImplGlfw_InitForOpenGL(window, false);
    ImGui_ImplGlfw_InstallCallbacks(window);
    ImGui_ImplOpenGL3_Init("#version 410");

    return true;
}

void Application::setupShaders() {
    const unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    const unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    shaderProgram = glCreateProgram();

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
}

void Application::setupBuffers() {
    constexpr float vertices[] = {
        1.0f, 1.0f, 0.0f, 1.0f, 1.0f, // top right
        1.0f, -1.0f, 0.0f, 1.0f, 0.0f, // bottom right
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, // bottom left
        -1.0f, 1.0f, 0.0f, 0.0f, 1.0f // top left
    };

    constexpr unsigned int indices[] = {
        0, 1, 3, // first triangle
        1, 2, 3 // second triangle
    };

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
}

void Application::buildScene() {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, 800, 800, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "screenTexture"), 0);
    ///

    constexpr std::string floorRoot = "../Models/obj/";
    const auto floorRes = std::make_shared<ModelResource>(floorRoot, "floor.obj", "floor_diffuse.tga",
                                             "floor_nm_tangent.tga", "floor_spec.tga");
    ModelInstance floorModel(floorRes, false);

    floorModel.scale = {2.0, 1.0, 2.0};
    floorModel.position = {0.0, 0.0, -2.0};
    floorModel.isDeletable = false;

    scene.addModel(floorModel);


    // glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    glfwSetWindowUserPointer(window, this);

    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);
}


bool Application::init() {
    if (!initWindow()) return false;
    setupBuffers();
    setupShaders();
    buildScene();
    return true;
}

void Application::run() {
    float deltaTime = 0.0f;
    float lastFrame = 0.0f;

    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();

        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        ImGui::SetNextWindowSize(ImVec2(350, 500), ImGuiCond_FirstUseEver);
        ImGui::Begin("Scene Inspector");
        ImGuiIO& io = ImGui::GetIO();
        ImGui::Text("Performance: %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);

        // Lighting
        ImGui::Separator();
        ImGui::Text("Global Light Settings:");
        if (ImGui::SliderFloat3("Light Direction", &scene.lightDir[0], -1.0f, 1.0f)) {
            scene.lightPos = scene.lightDir * 5.0f;
        }
        ImGui::ColorEdit3("Light Color", &scene.lightColor[0]);

        if (ImGui::Button("Reset Camera")) {
            scene.getActiveCamera().reset();
        }

        ImGui::Separator();
        ImGui::Text("Post-Processing & Shadows:");
        ImGui::Checkbox("Enable Shadows", &scene.useShadows);
        ImGui::Checkbox("Enable SSAO", &scene.useSSAO);

        ImGui::Separator();
        ImGui::Text("Models in Scene:");

        for (int i = 0; i < scene.models.size(); ++i) {
            auto& model = scene.models[i];
            std::string label = "Model " + std::to_string(i) + (model.isDeletable ? "" : " (Floor)");

            if (ImGui::CollapsingHeader(label.c_str())) {
                ImGui::PushID(i);

                ImGui::Checkbox("Use Diffuse", &model.useDiffuse);
                ImGui::Checkbox("Use Normal Map", &model.useNormalMap);
                ImGui::Checkbox("Use Specular Map", &model.useSpecularMap);
                ImGui::Checkbox("Use Wireframe", &model.useWireframe);
                ImGui::Checkbox("Fill Color", &model.fillColor);
                ImGui::SliderFloat3("Position", &model.position[0], -5.0f, 5.0f);
                ImGui::SliderFloat3("Rotation", &model.rotation[0], 0.0f, 360.0f);

                float uniformScale = model.scale.x();
                if (ImGui::SliderFloat("Scale", &uniformScale, 0.1f, 3.0f)) {
                    model.scale = {uniformScale, uniformScale, uniformScale};
                }

                if (model.isDeletable && ImGui::Button("Remove")) {
                    scene.models.erase(scene.models.begin() + i);
                }

                ImGui::PopID();
            }
        }
        ImGui::End();

        // Model Loader
        ImGui::Begin("Model Library");
        std::string modelsPath = "../Models/obj/";

        if (fs::exists(modelsPath)) {
            for (const auto& entry : fs::directory_iterator(modelsPath)) {
                if (entry.is_directory()) {
                    std::string folderName = entry.path().filename().string();

                    if (ImGui::Button(("Add " + folderName).c_str())) {
                        addModelToScene(modelsPath + folderName + "/",
                                        folderName + ".obj",
                                        folderName + "_diffuse.tga",
                                        folderName + "_nm_tangent.tga",
                                        folderName + "_spec.tga");
                    }
                }
            }
        } else {
            ImGui::TextColored(ImVec4(1,0,0,1), "Error: Models path not found!");
        }
        ImGui::End();

        // Cameras

        ImGui::Begin("Camera Manager");

        if (ImGui::Button("Add New Camera (Current View)")) {
            Camera newCam = scene.getActiveCamera();
            scene.cameras.push_back(newCam);
            scene.activeCameraIndex = static_cast<int>(scene.cameras.size()) - 1;
        }

        ImGui::Separator();
        ImGui::Text("Cameras List:");

        for (int i = 0; i < scene.cameras.size(); i++) {
            ImGui::PushID(i);

            if (scene.cameras.size() > 1) {
                if (ImGui::Button("Delete")) {
                    if (i == scene.activeCameraIndex && scene.activeCameraIndex > 0) {
                        scene.activeCameraIndex--;
                    } else if (i < scene.activeCameraIndex) {
                        scene.activeCameraIndex--;
                    }
                    scene.cameras.erase(scene.cameras.begin() + i);
                    i--;
                    ImGui::PopID();
                    continue;
                }
                ImGui::SameLine();
            }

            std::string label = "Camera " + std::to_string(i);
            if (ImGui::Selectable(label.c_str(), scene.activeCameraIndex == i)) {
                scene.activeCameraIndex = i;
            }

            ImGui::PopID();
        }

        Camera &currCam = scene.getActiveCamera();
        ImGui::Separator();
        ImGui::Text("Active Camera Settings:");
        ImGui::DragFloat3("Position", &currCam.pos[0], 0.1f);
        ImGui::DragFloat("Focal Length", &currCam.focalLength, 0.01f, 0.1f, 10.0f);

        ImGui::End();

        Vec3f front;
        front.x() = cos(currCam.yaw * M_PI / 180.0f) * cos(currCam.pitch * M_PI / 180.0f);
        front.y() = sin(currCam.pitch * M_PI / 180.0f);
        front.z() = sin(currCam.yaw * M_PI / 180.0f) * cos(currCam.pitch * M_PI / 180.0f);
        Vec3f forward = front.normalize();

        Camera &cam = scene.getActiveCamera();
        float speed = 2.5f * deltaTime;
        Vec3f right = cross(forward, cam.up).normalize();

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            cam.pos = cam.pos + (forward * speed);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            cam.pos = cam.pos - (forward * speed);
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cam.pos = cam.pos + (right * speed);
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cam.pos = cam.pos - (right * speed);
        }

        cam.lookAt = cam.pos + forward;
        Renderer::render(scene, rb);

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);
        // glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 800, 800, GL_RGB, GL_UNSIGNED_BYTE, rb.colorBuffer.data());

        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);

        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    glfwDestroyWindow(window);
    glfwTerminate();
}

void Application::processMouseInput(const double xPos, const double yPos) {
    if (glfwGetKey(window, GLFW_KEY_Q) != GLFW_PRESS) {
        firstMouse = true;
        return;
    }

    if (firstMouse) {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }

    float xOffset = xPos - lastX;
    float yOffset = lastY - yPos;

    lastX = xPos;
    lastY = yPos;

    xOffset *= 0.1f;
    yOffset *= 0.1f;

    Camera &activeCam = scene.getActiveCamera();
    activeCam.yaw += xOffset;
    activeCam.pitch += yOffset;

    if (activeCam.pitch > 89.0f) activeCam.pitch = 89.0f;
    if (activeCam.pitch < -89.0f) activeCam.pitch = -89.0f;
}

void Application::mouse_callback(GLFWwindow *window, const double xPos, const double yPos) {
    ImGui_ImplGlfw_CursorPosCallback(window, xPos, yPos);
    if (ImGui::GetIO().WantCaptureMouse) return;

    auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app) {
        app->processMouseInput(xPos, yPos);
    }
}


void Application::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    ImGui_ImplGlfw_MouseButtonCallback(window, button, action, mods);
    if (ImGui::GetIO().WantCaptureMouse) return;

    double xPos, yPos;
    auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glfwGetCursorPos(window, &xPos, &yPos);
        Vec3f ray = app->screenToWorldRay(xPos, yPos);

        float minT = std::numeric_limits<float>::max();
        int closestModelIndex = -1;
        for (int i = 0 ; i < app->scene.models.size(); ++i) {
            const auto& model = app->scene.models[i];
            const auto bbox = model.getWorldAABB();

            if (model.isDeletable == false) continue;
            const float t = ModelInstance::RayBoxInterSection(app->scene.getActiveCamera().pos, ray, bbox.min, bbox.max);

            if (t >= 0 && t < minT) {
                minT = t;
                closestModelIndex = i;
            }
        }

        if (closestModelIndex != -1) {
            std::cout << "Removing model at index: " << closestModelIndex << std::endl;
            app->scene.models.erase(app->scene.models.begin() + closestModelIndex);
        }

        std::cout << closestModelIndex << std::endl;
        std::cout << ray.x() << " " << ray.y() << " " << ray.z() << std::endl;
    }
}

Vec3f Application::screenToWorldRay(const double mouseX, const double mouseY) {
    // Step 1 - NDC
    const auto xNdc = (2 * mouseX) / width - 1;
    const auto yNdc = 1 - (2 * mouseY) / height;

    // Step 2 - Clip Space
    const Vec4f rayClip = {xNdc, yNdc, -1.0f, 1.0f};

    // Step 3 - Eye Space
    const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    Vec4f rayEye = Matrix4f4::perspective(60.0f, aspectRatio, 0.1f, 100.0f).inverse4x4() * rayClip;

    rayEye = {rayEye.x(), rayEye.y(), -1.0f, 0.0f};

    // Step 4 - World Space
    const Camera& cam = scene.getActiveCamera();
    Vec4f rayWorld = Matrix4f4::lookat(cam.pos, cam.lookAt, cam.up).inverse4x4() * rayEye;

    const Vec3f retVec = {rayWorld.x(), rayWorld.y(), rayWorld.z()};
    return retVec.normalize();
}

void Application::addModelToScene(const std::string& folderPath, const std::string& objFile,
                                  const std::string& diffFile, const std::string& nmFile,
                                  const std::string& specFile) {
    std::string resourceKey = folderPath + objFile;
    if (resourceCache.find(resourceKey) == resourceCache.end()) {
        resourceCache[resourceKey] = std::make_shared<ModelResource>(
            folderPath, objFile, diffFile, nmFile, specFile
        );
    }

    ModelInstance newModel(resourceCache[resourceKey], false);
    newModel.position = { 0, 0, 0 };
    scene.addModel(newModel);
}
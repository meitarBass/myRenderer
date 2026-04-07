#include "Application.h"

#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui_impl_glfw.h"
#include "../external/imgui/imgui_impl_opengl3.h"

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


    const std::string diabloRoot = "../Models/obj/diablo3_pose/";
    const std::string headRoot = "../Models/obj/african_head/";
    constexpr std::string floorRoot = "../Models/obj/";

    auto diabloRes = std::make_shared<ModelResource>(diabloRoot, "diablo3_pose.obj", "diablo3_pose_diffuse.tga",
                                     "diablo3_pose_nm_tangent.tga", "diablo3_pose_spec.tga");
    ModelInstance d1(diabloRes, false);
    d1.rotation = {0, 40, 0};
    d1.scale = {0.55, 0.55, 0.55};
    d1.position = {-0.5, -0.42, -1.5};

    scene.addModel(d1);

    ModelInstance d2(diabloRes, false);
    d2.rotation = {0, -50, 0};
    d2.scale = {0.45, 0.45, 0.45};
    d2.position = {1.3, -0.53, -2.8};

    scene.addModel(d2);

    ModelInstance d3(diabloRes, false);
    d3.rotation = {0, 100, 0};
    d3.scale = {0.35, 0.35, 0.35};
    d3.position = {-1.15, -0.60, -0.1};

    scene.addModel(d3);

    auto headRes = std::make_shared<ModelResource>(
        headRoot, "african_head.obj", "african_head_diffuse.tga",
        "african_head_nm_tangent.tga", "african_head_spec.tga");
    auto eyesInRes = std::make_shared<ModelResource>(
        headRoot, "african_head_eye_inner.obj", "african_head_eye_inner_diffuse.tga",
        "african_head_eye_inner_nm_tangent.tga", "african_head_eye_inner_spec.tga");

    ModelInstance head(headRes, false);
    ModelInstance eyesIn(eyesInRes, false);
    head.rotation = {-20, 10, 0};
    eyesIn.rotation = {-20, 10, 0};

    head.scale = {0.6, 0.6, 0.6};
    eyesIn.scale = {0.6, 0.6, 0.6};

    head.position = {0, -0.6, -1.2};
    eyesIn.position = {0, -0.6, -1.2};

    scene.addModel(head);
    scene.addModel(eyesIn);

    auto floorRes = std::make_shared<ModelResource>(floorRoot, "floor.obj", "floor_diffuse.tga",
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
            scene.camera.pos = {0, 3, 10};
        }

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
                ImGui::Checkbox("Use wireframe", &model.useWireframe);
                ImGui::SliderFloat3("Position", &model.position[0], -5.0f, 5.0f);
                ImGui::SliderFloat3("Rotation", &model.rotation[0], 0.0f, 360.0f);
                ImGui::SliderFloat("Scale", &model.scale[0], 0.1f, 3.0f);

                if (model.isDeletable && ImGui::Button("Remove")) {
                    scene.models.erase(scene.models.begin() + i);
                }

                ImGui::PopID();
            }
        }
        ImGui::End();

        // FPS
        ImGui::SetNextWindowPos(ImVec2(10, 10));
        ImGui::Begin("Stats", nullptr, ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_AlwaysAutoResize | ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing | ImGuiWindowFlags_NoNav);
        ImGui::Text("%.1f FPS", ImGui::GetIO().Framerate);
        ImGui::End();

        Vec3f front;
        front.x() = cos(yaw * M_PI / 180.0f) * cos(pitch * M_PI / 180.0f);
        front.y() = sin(pitch * M_PI / 180.0f);
        front.z() = sin(yaw * M_PI / 180.0f) * cos(pitch * M_PI / 180.0f);

        Vec3f forward = front.normalize();


        float speed = 2.5f * deltaTime;
        Vec3f right = cross(forward, scene.camera.up).normalize();

        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            scene.camera.pos = scene.camera.pos + (forward * speed);
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            scene.camera.pos = scene.camera.pos - (forward * speed);
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            scene.camera.pos = scene.camera.pos + (right * speed);
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            scene.camera.pos = scene.camera.pos - (right * speed);
        }

        scene.camera.lookAt = scene.camera.pos + forward;
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

    yaw += xOffset;
    pitch += yOffset;

    if (pitch > 89.0f) pitch = 89.0f;
    if (pitch < -89.0f) pitch = -89.0f;
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
            const float t = ModelInstance::RayBoxInterSection(app->scene.camera.pos, ray, bbox.min, bbox.max);

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
    Vec4f rayWorld = Matrix4f4::lookat(scene.camera.pos, scene.camera.lookAt, scene.camera.up).inverse4x4() * rayEye;

    const Vec3f retVec = {rayWorld.x(), rayWorld.y(), rayWorld.z()};
    return retVec.normalize();
}

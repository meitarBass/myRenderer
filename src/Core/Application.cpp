#include "Application.h"

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

    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress)) {
        std::cerr << "Failed to initialize GLAD\n";
        return false;
    }
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

    auto diabloModel = ModelInstance(diabloRoot, "diablo3_pose.obj", "diablo3_pose_diffuse.tga",
                                     "diablo3_pose_nm_tangent.tga", "diablo3_pose_spec.tga", false);
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

    auto head = ModelInstance(headRoot, "african_head.obj", "african_head_diffuse.tga", "african_head_nm_tangent.tga",
                              "african_head_spec.tga", false);
    auto eyes_in = ModelInstance(headRoot, "african_head_eye_inner.obj", "african_head_eye_inner_diffuse.tga",
                                 "african_head_eye_inner_nm_tangent.tga", "african_head_eye_inner_spec.tga", false);
    head.rotation = {-20, 10, 0};
    eyes_in.rotation = {-20, 10, 0};

    head.scale = {0.6, 0.6, 0.6};
    eyes_in.scale = {0.6, 0.6, 0.6};

    head.position = {0, -0.6, -1.2};
    eyes_in.position = {0, -0.6, -1.2};

    scene.addModel(head);
    scene.addModel(eyes_in);

    auto floorModel = ModelInstance(floorRoot, "floor.obj", "floor_diffuse.tga", "floor_nm_tangent.tga",
                                    "floor_spec.tga", false);
    floorModel.scale = {2.0, 1.0, 2.0};
    floorModel.position = {0.0, 0.0, -2.0};

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
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

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
}

void Application::processMouseInput(const double xPos, const double yPos) {
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
    auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app) {
        app->processMouseInput(xPos, yPos);
    }
}


void Application::mouse_button_callback(GLFWwindow* window, int button, int action, int mods) {
    double xPos, yPos;
    auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window));
    if (app && button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS) {
        glfwGetCursorPos(window, &xPos, &yPos);
        Vec3f ray = app->screenToWorldRay(xPos, yPos);

        std::cout << ray.x() << " " << ray.y() << " " << ray.z() << std::endl;
    }
}

Vec3f Application::screenToWorldRay(const double mouseX, const double mouseY) {
    // Step 1 - NDC
    const auto xNdc = (2 * mouseX) / width - 1;
    const auto yNdc = 1 - (2 * mouseY) / height;

    // Step 2 - Clip Space
    Vec4f rayClip = {xNdc, yNdc, -1.0f, 1.0f};

    // Step 3 - Eye Space
    const float aspectRatio = static_cast<float>(width) / static_cast<float>(height);
    Vec4f rayEye = Matrix4f4::perspective(60.0f, aspectRatio, 0.1f, 100.0f).inverse4x4() * rayClip;

    rayEye = {rayEye.x(), rayEye.y(), -1.0f, 0.0f};

    // Step 4 - World Space
    Vec4f rayWorld = Matrix4f4::lookat(cam.pos, cam.lookAt, cam.up).inverse4x4() * rayEye;

    const Vec3f retVec = {rayClip.x(), rayClip.y(), rayClip.z()};
    return retVec.normalize();
}

#include "Application.h"

#include "../external/imgui/imgui.h"
#include "../external/imgui/imgui_impl_glfw.h"
#include "../external/imgui/imgui_impl_opengl3.h"
#include "../Shaders/GLPhongShader.h"

#include <algorithm>
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

    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

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

Matrix4f4 Application::buildStandardGLProjection(const Camera& cam, const float aspect) {
    if (cam.projectionType == ProjectionType::Orthographic) {
        const float halfWidth = cam.orthoSize * aspect;
        const float halfHeight = cam.orthoSize;
        Matrix4f4 res;
        res[0][0] = 1.0f / halfWidth;
        res[1][1] = 1.0f / halfHeight;
        res[2][2] = -2.0f / (cam.farPlane - cam.nearPlane);
        res[3][2] = -(cam.farPlane + cam.nearPlane) / (cam.farPlane - cam.nearPlane);
        res[3][3] = 1.0f;
        return res;
    }

    const float fovDegrees = cam.projectionType == ProjectionType::PerspectiveFov
        ? cam.fov
        : 2.0f * std::atan(1.0f / cam.focalLength) * (180.0f / static_cast<float>(M_PI));
    const float angle = fovDegrees * (static_cast<float>(M_PI) / 180.0f);
    const float f = 1.0f / std::tan(angle / 2.0f);

    Matrix4f4 res;
    res[0][0] = f / aspect;
    res[1][1] = f;
    res[2][2] = (cam.farPlane + cam.nearPlane) / (cam.nearPlane - cam.farPlane);
    res[2][3] = -1.0f;
    res[3][2] = (2.0f * cam.farPlane * cam.nearPlane) / (cam.nearPlane - cam.farPlane);
    return res;
}

void Application::setupGLModelShader() {
    const unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &glModelVertexShaderSource, NULL);
    glCompileShader(vertexShader);

    const unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &glModelFragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    glModelShaderProgram = glCreateProgram();
    glAttachShader(glModelShaderProgram, vertexShader);
    glAttachShader(glModelShaderProgram, fragmentShader);
    glLinkProgram(glModelShaderProgram);

    int success;
    char infoLog[512];

    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        std::cout << "GL MODEL VERTEX SHADER ERROR:\n" << infoLog << std::endl;
    }

    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if (!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        std::cout << "GL MODEL FRAGMENT SHADER ERROR:\n" << infoLog << std::endl;
    }

    glGetProgramiv(glModelShaderProgram, GL_LINK_STATUS, &success);
    if (!success) {
        glGetProgramInfoLog(glModelShaderProgram, 512, NULL, infoLog);
        std::cout << "GL MODEL PROGRAM LINK ERROR:\n" << infoLog << std::endl;
    }

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

unsigned int Application::uploadGLTexture(TGAImage& img) {
    unsigned int texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);

    const GLenum srcFormat = img.get(0, 0).bytespp == 4 ? GL_BGRA : GL_BGR;

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.width(), img.height(), 0, srcFormat, GL_UNSIGNED_BYTE, img.buffer());
    glGenerateMipmap(GL_TEXTURE_2D);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    return texID;
}

unsigned int Application::ensureEnvironmentCubemap() {
    if (envCubemap != 0) return envCubemap;

    constexpr int faceSize = 64;
    glGenTextures(1, &envCubemap);
    glBindTexture(GL_TEXTURE_CUBE_MAP, envCubemap);

    // Face direction vectors follow OpenGL's standard cubemap layout, uc/vc
    // range over each face in [-1, 1]. Colored by the same horizon-to-zenith
    // blend Renderer::applySkybox uses for the CPU path's gradient sky.
    for (int face = 0; face < 6; ++face) {
        std::vector<unsigned char> pixels(faceSize * faceSize * 3);
        for (int y = 0; y < faceSize; ++y) {
            for (int x = 0; x < faceSize; ++x) {
                const float uc = (2.0f * (x + 0.5f) / faceSize) - 1.0f;
                const float vc = (2.0f * (y + 0.5f) / faceSize) - 1.0f;

                Vec3f dir;
                switch (face) {
                    case 0: dir = {1.0f, -vc, -uc}; break;
                    case 1: dir = {-1.0f, -vc, uc}; break;
                    case 2: dir = {uc, 1.0f, vc}; break;
                    case 3: dir = {uc, -1.0f, -vc}; break;
                    case 4: dir = {uc, -vc, 1.0f}; break;
                    default: dir = {-uc, -vc, -1.0f}; break;
                }
                dir = dir.normalize();

                const float blend = std::max(0.0f, std::min(1.0f, dir.y() * 0.5f + 0.5f));
                const Vec3f color = scene.skyHorizonColor + (scene.skyZenithColor - scene.skyHorizonColor) * blend;

                const int idx = (y * faceSize + x) * 3;
                pixels[idx]     = static_cast<unsigned char>(std::min(255.0f, color.x() * 255.0f));
                pixels[idx + 1] = static_cast<unsigned char>(std::min(255.0f, color.y() * 255.0f));
                pixels[idx + 2] = static_cast<unsigned char>(std::min(255.0f, color.z() * 255.0f));
            }
        }
        glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + face, 0, GL_RGB, faceSize, faceSize, 0,
                     GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    }

    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    return envCubemap;
}

GLModelBuffers& Application::ensureGLResources(ModelResource& resource) {
    const auto cached = glResourceCache.find(&resource);
    if (cached != glResourceCache.end()) return cached->second;

    GLModelBuffers buffers;
    const auto& faces = resource.model.getFaces();

    constexpr int floatsPerVertex = 17; // pos(3) + normal(3) + uv(2) + tangent(3) + bitangent(3) + faceNormal(3)
    std::vector<float> vertexData;
    vertexData.reserve(faces.size() * 3 * floatsPerVertex);

    for (const auto& face : faces) {
        const auto [tangent, bitangent] = calculateTriangleBasis(face.pts, face.uv);
        const Vec3f faceNormal = calculateFaceNormal(face.pts[0], face.pts[1], face.pts[2]);

        for (int i = 0; i < 3; ++i) {
            const float attribs[floatsPerVertex] = {
                face.pts[i].x(), face.pts[i].y(), face.pts[i].z(),
                face.normals[i].x(), face.normals[i].y(), face.normals[i].z(),
                face.uv[i].x(), face.uv[i].y(),
                tangent.x(), tangent.y(), tangent.z(),
                bitangent.x(), bitangent.y(), bitangent.z(),
                faceNormal.x(), faceNormal.y(), faceNormal.z()
            };
            vertexData.insert(vertexData.end(), std::begin(attribs), std::end(attribs));
        }
    }

    buffers.vertexCount = static_cast<int>(faces.size() * 3);

    glGenVertexArrays(1, &buffers.vao);
    glBindVertexArray(buffers.vao);

    glGenBuffers(1, &buffers.vbo);
    glBindBuffer(GL_ARRAY_BUFFER, buffers.vbo);
    glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

    constexpr GLsizei stride = floatsPerVertex * sizeof(float);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (void*)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (void*)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (void*)(6 * sizeof(float)));
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, stride, (void*)(8 * sizeof(float)));
    glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, stride, (void*)(11 * sizeof(float)));
    glVertexAttribPointer(5, 3, GL_FLOAT, GL_FALSE, stride, (void*)(14 * sizeof(float)));
    for (unsigned int i = 0; i <= 5; ++i) glEnableVertexAttribArray(i);

    if (resource.diffuse.width() > 0) {
        buffers.diffuseTex = uploadGLTexture(resource.diffuse);
        buffers.hasDiffuseTex = true;
    }
    if (resource.specular.width() > 0) {
        buffers.specularTex = uploadGLTexture(resource.specular);
        buffers.hasSpecularTex = true;
    }
    if (resource.normal.width() > 0) {
        buffers.normalTex = uploadGLTexture(resource.normal);
        buffers.hasNormalTex = true;
    }

    glBindVertexArray(0);

    return glResourceCache.emplace(&resource, buffers).first->second;
}

void Application::renderModelsGL() {
    const Camera& cam = scene.getActiveCamera();
    const Matrix4f4 view = Matrix4f4::lookat(cam.pos, cam.lookAt, cam.up);
    const float aspect = height > 0 ? static_cast<float>(width) / static_cast<float>(height) : 1.0f;

    const Matrix4f4 projection = buildStandardGLProjection(cam, aspect);

    glUseProgram(glModelShaderProgram);

    glUniformMatrix4fv(glGetUniformLocation(glModelShaderProgram, "view"), 1, GL_FALSE, &view[0][0]);
    glUniformMatrix4fv(glGetUniformLocation(glModelShaderProgram, "projection"), 1, GL_FALSE, &projection[0][0]);
    glUniform3fv(glGetUniformLocation(glModelShaderProgram, "cameraPos"), 1, &cam.pos[0]);
    glUniform3fv(glGetUniformLocation(glModelShaderProgram, "ambientLight"), 1, &scene.ambientLight[0]);
    glUniform1f(glGetUniformLocation(glModelShaderProgram, "time"), static_cast<float>(glfwGetTime()));

    const int numLights = std::min(static_cast<int>(scene.lights.size()), 8);
    glUniform1i(glGetUniformLocation(glModelShaderProgram, "numLights"), numLights);
    for (int i = 0; i < numLights; ++i) {
        const Light& light = scene.lights[i];
        const std::string prefix = "lights[" + std::to_string(i) + "].";
        glUniform1i(glGetUniformLocation(glModelShaderProgram, (prefix + "type").c_str()), light.type == LightType::Directional ? 0 : 1);
        glUniform3fv(glGetUniformLocation(glModelShaderProgram, (prefix + "direction").c_str()), 1, &light.direction[0]);
        glUniform3fv(glGetUniformLocation(glModelShaderProgram, (prefix + "position").c_str()), 1, &light.position[0]);
        glUniform3fv(glGetUniformLocation(glModelShaderProgram, (prefix + "color").c_str()), 1, &light.color[0]);
        glUniform1f(glGetUniformLocation(glModelShaderProgram, (prefix + "intensity").c_str()), light.intensity);
    }

    for (auto& object : scene.models) {
        GLModelBuffers& buffers = ensureGLResources(*object.resource);

        const Matrix4f4 model = object.getModelMatrix();
        const Matrix3f3 normalMatrix = model.inverseTranspose3x3();

        glUniformMatrix4fv(glGetUniformLocation(glModelShaderProgram, "model"), 1, GL_FALSE, &model[0][0]);
        glUniformMatrix3fv(glGetUniformLocation(glModelShaderProgram, "normalMatrix"), 1, GL_FALSE, &normalMatrix[0][0]);
        glUniform1i(glGetUniformLocation(glModelShaderProgram, "shadingMode"), static_cast<int>(object.shadingMode));
        glUniform1i(glGetUniformLocation(glModelShaderProgram, "uvMode"), static_cast<int>(object.uvMode));
        glUniform1f(glGetUniformLocation(glModelShaderProgram, "uvScale"), object.uvScale);

        glUniform1i(glGetUniformLocation(glModelShaderProgram, "useVertexAnimation"), object.useVertexAnimation ? 1 : 0);
        glUniform1f(glGetUniformLocation(glModelShaderProgram, "vertexAnimAmplitude"), object.vertexAnimAmplitude);
        glUniform1f(glGetUniformLocation(glModelShaderProgram, "vertexAnimSpeed"), object.vertexAnimSpeed);

        glUniform1i(glGetUniformLocation(glModelShaderProgram, "proceduralTexMode"), static_cast<int>(object.proceduralTexMode));
        glUniform1f(glGetUniformLocation(glModelShaderProgram, "proceduralTexScale"), object.proceduralTexScale);

        glUniform3fv(glGetUniformLocation(glModelShaderProgram, "matDiffuse"), 1, &object.material.diffuseColor[0]);
        glUniform3fv(glGetUniformLocation(glModelShaderProgram, "matSpecular"), 1, &object.material.specularColor[0]);
        glUniform3fv(glGetUniformLocation(glModelShaderProgram, "matEmissive"), 1, &object.material.emissive[0]);
        glUniform1i(glGetUniformLocation(glModelShaderProgram, "matUseNonUniform"), object.material.useNonUniformMaterial ? 1 : 0);
        glUniform3fv(glGetUniformLocation(glModelShaderProgram, "matDiffuseSecondary"), 1, &object.material.diffuseColorSecondary[0]);

        const bool useDiffuseTex = buffers.hasDiffuseTex && object.useDiffuse;
        glUniform1i(glGetUniformLocation(glModelShaderProgram, "useDiffuseTex"), useDiffuseTex ? 1 : 0);
        if (useDiffuseTex) {
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, buffers.diffuseTex);
            glUniform1i(glGetUniformLocation(glModelShaderProgram, "diffuseTex"), 0);
        }

        const bool useSpecularTex = buffers.hasSpecularTex && object.useSpecularMap;
        glUniform1i(glGetUniformLocation(glModelShaderProgram, "useSpecularTex"), useSpecularTex ? 1 : 0);
        if (useSpecularTex) {
            glActiveTexture(GL_TEXTURE1);
            glBindTexture(GL_TEXTURE_2D, buffers.specularTex);
            glUniform1i(glGetUniformLocation(glModelShaderProgram, "specularTex"), 1);
        }

        const bool useNormalMapTex = buffers.hasNormalTex && object.useNormalMap;
        glUniform1i(glGetUniformLocation(glModelShaderProgram, "useNormalMapTex"), useNormalMapTex ? 1 : 0);
        if (useNormalMapTex) {
            glActiveTexture(GL_TEXTURE2);
            glBindTexture(GL_TEXTURE_2D, buffers.normalTex);
            glUniform1i(glGetUniformLocation(glModelShaderProgram, "normalMapTex"), 2);
        }

        glActiveTexture(GL_TEXTURE3);
        glUniform1i(glGetUniformLocation(glModelShaderProgram, "envMap"), 3);
        glUniform1i(glGetUniformLocation(glModelShaderProgram, "envMapMode"), static_cast<int>(object.envMapMode));
        if (object.envMapMode != EnvMapMode::Off) {
            glUniform1f(glGetUniformLocation(glModelShaderProgram, "envMapStrength"), object.envMapStrength);
            glUniform1f(glGetUniformLocation(glModelShaderProgram, "envMapIOR"), object.envMapIOR);
            glBindTexture(GL_TEXTURE_CUBE_MAP, ensureEnvironmentCubemap());
        }

        glUniform1i(glGetUniformLocation(glModelShaderProgram, "useToonShading"), object.useToonShading ? 1 : 0);
        glUniform1f(glGetUniformLocation(glModelShaderProgram, "toonLevels"), static_cast<float>(object.toonLevels));

        glUniform1i(glGetUniformLocation(glModelShaderProgram, "colorAnimMode"), static_cast<int>(object.colorAnimMode));
        glUniform1f(glGetUniformLocation(glModelShaderProgram, "colorAnimSpeed"), object.colorAnimSpeed);

        glBindVertexArray(buffers.vao);

        if (object.useSilhouette) {
            glEnable(GL_CULL_FACE);
            glCullFace(GL_FRONT);
            glUniform1i(glGetUniformLocation(glModelShaderProgram, "renderSilhouette"), 1);
            glUniform1f(glGetUniformLocation(glModelShaderProgram, "silhouetteThickness"), object.silhouetteThickness);
            glUniform3fv(glGetUniformLocation(glModelShaderProgram, "silhouetteColor"), 1, &object.silhouetteColor[0]);
            glDrawArrays(GL_TRIANGLES, 0, buffers.vertexCount);
            glDisable(GL_CULL_FACE);
            glUniform1i(glGetUniformLocation(glModelShaderProgram, "renderSilhouette"), 0);
        }

        glDrawArrays(GL_TRIANGLES, 0, buffers.vertexCount);
    }

    glBindVertexArray(0);
}

enum class PrimitiveType { Cube, Pyramid, Tetrahedron };
void getPrimitiveGeometry(const PrimitiveType type,
                          std::vector<Vec3f>& outVertices,
                          std::vector<std::vector<int>>& outFaceIndices) {
    switch (type) {
        case PrimitiveType::Cube:
            outVertices = {
                {-0.5f, -0.5f,  0.5f}, { 0.5f, -0.5f,  0.5f}, { 0.5f,  0.5f,  0.5f}, {-0.5f,  0.5f,  0.5f},
                {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f,  0.5f, -0.5f}, {-0.5f,  0.5f, -0.5f}
            };
            outFaceIndices = {
                {0, 1, 2}, {2, 3, 0}, // Front
                {1, 5, 6}, {6, 2, 1}, // Right
                {7, 6, 5}, {5, 4, 7}, // Back
                {4, 0, 3}, {3, 7, 4}, // Left
                {3, 2, 6}, {6, 7, 3}, // Top
                {4, 5, 1}, {1, 0, 4}  // Bottom
            };
            break;

        case PrimitiveType::Pyramid:
            // Square base (0-3) + apex (4).
            outVertices = {
                {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f}, {-0.5f, -0.5f,  0.5f},
                { 0.0f,  0.5f,  0.0f}
            };
            outFaceIndices = {
                {0, 1, 2}, {2, 3, 0},       // Base
                {1, 0, 4}, {2, 1, 4},       // Sides
                {3, 2, 4}, {0, 3, 4}
            };
            break;

        case PrimitiveType::Tetrahedron:
            // Triangular base (0-2) + apex (3).
            outVertices = {
                {-0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f, -0.5f}, { 0.5f, -0.5f,  0.5f},
                { 0.0f,  0.5f,  0.0f}
            };
            outFaceIndices = {
                {0, 1, 2},                  // Base
                {1, 0, 3}, {2, 1, 3}, {0, 2, 3} // Sides
            };
            break;
    }
}

void addPrimitiveToScene(Scene& scene, const PrimitiveType type, const Vec3f& position) {
    std::vector<Vec3f> vertices;
    std::vector<std::vector<int>> faceIndices;
    getPrimitiveGeometry(type, vertices, faceIndices);

    // Borrows the floor's textures just so ModelResource has real files to
    // load -- the geometry gets replaced entirely below.
    auto resource = std::make_shared<ModelResource>("../Models/obj/", "floor.obj", "floor_diffuse.tga", "floor_nm_tangent.tga", "floor_spec.tga");

    using FaceContainerType = std::remove_const_t<std::remove_reference_t<decltype(resource->model.getFaces())>>;
    using FaceType = typename FaceContainerType::value_type;

    auto& internalFaces = const_cast<FaceContainerType&>(resource->model.getFaces());
    internalFaces.clear();

    for (const auto& idx : faceIndices) {
        FaceType face;
        face.pts[0] = vertices[idx[0]];
        face.pts[1] = vertices[idx[1]];
        face.pts[2] = vertices[idx[2]];

        Vec3f edge1 = face.pts[1] - face.pts[0];
        Vec3f edge2 = face.pts[2] - face.pts[0];
        Vec3f norm = cross(edge1, edge2).normalize();

        face.normals[0] = norm;
        face.normals[1] = norm;
        face.normals[2] = norm;

        face.uv[0] = {0, 0};
        face.uv[1] = {1, 0};
        face.uv[2] = {1, 1};

        internalFaces.push_back(face);
    }

    ModelInstance instance(resource, false);
    instance.position = position;
    instance.scale = {1.0f, 1.0f, 1.0f};
    instance.isDeletable = true;
    instance.useFaceNormalDrawing = true;

    scene.addModel(instance);
}

constexpr float MODEL_ZOOM_STEP = 0.5f;

// Reorients the camera to look at the model's bbox center without moving it.
void focusCameraOnModel(Camera& cam, const ModelInstance& model) {
    const AABB bbox = model.getWorldAABB();
    const Vec3f target = (bbox.min + bbox.max) * 0.5f;

    const Vec3f toTarget = target - cam.pos;
    const float dist = toTarget.length();
    if (dist < GraphicsUtils::EPSILON) return;

    const Vec3f dir = toTarget / dist;

    cam.pitch = std::asin(std::clamp(dir.y(), -1.0f, 1.0f)) * 180.0 / M_PI;
    cam.yaw = std::atan2(dir.z(), dir.x()) * 180.0 / M_PI;
    cam.pitch = std::clamp(cam.pitch, -89.0, 89.0);

    cam.lookAt = target;
}

void zoomCameraTowardModel(Camera& cam, const ModelInstance& model, const float amount) {
    const AABB bbox = model.getWorldAABB();
    const Vec3f target = (bbox.min + bbox.max) * 0.5f;

    const Vec3f toTarget = target - cam.pos;
    const float dist = toTarget.length();
    if (dist < GraphicsUtils::EPSILON) return;

    const Vec3f dir = toTarget / dist;
    const float minDist = (bbox.max - bbox.min).length() * 0.5f + 0.5f;

    const float newDist = std::max(dist - amount, minDist);
    cam.pos = target - dir * newDist;
}

void Application::buildScene() {
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);

    glUseProgram(shaderProgram);
    glUniform1i(glGetUniformLocation(shaderProgram, "screenTexture"), 0);
    ///

    constexpr std::string floorRoot = "../Models/obj/";
    const auto floorRes = std::make_shared<ModelResource>(floorRoot, "floor.obj", "floor_diffuse.tga",
                                             "floor_nm_tangent.tga", "floor_spec.tga");
    ModelInstance floorModel(floorRes, false);

    floorModel.scale = {5.0, 1.0, 5.0};
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
    setupGLModelShader();
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
        ImGui::Text("Lights:");
        ImGui::ColorEdit3("Ambient Light", &scene.ambientLight[0]);

        int lightToRemove = -1;
        for (int i = 0; i < static_cast<int>(scene.lights.size()); ++i) {
            auto& light = scene.lights[i];
            ImGui::PushID(1000 + i);

            std::string lightLabel = "Light " + std::to_string(i) + (i == 0 ? " (casts shadow)" : "");
            if (ImGui::TreeNode(lightLabel.c_str())) {
                int typeIndex = light.type == LightType::Directional ? 0 : 1;
                const char* typeNames[] = { "Directional", "Point" };
                if (ImGui::Combo("Type", &typeIndex, typeNames, 2)) {
                    light.type = typeIndex == 0 ? LightType::Directional : LightType::Point;
                }

                if (light.type == LightType::Directional) {
                    ImGui::SliderFloat3("Direction", &light.direction[0], -1.0f, 1.0f);
                } else {
                    ImGui::DragFloat3("Position", &light.position[0], 0.1f, -20.0f, 20.0f);
                }

                ImGui::ColorEdit3("Color", &light.color[0]);
                ImGui::SliderFloat("Intensity", &light.intensity, 0.0f, 3.0f);

                if (scene.lights.size() > 1 && ImGui::Button("Remove Light")) {
                    lightToRemove = i;
                }

                ImGui::TreePop();
            }

            ImGui::PopID();
        }

        if (lightToRemove >= 0) {
            scene.lights.erase(scene.lights.begin() + lightToRemove);
        }

        if (ImGui::Button("Add Light")) {
            scene.lights.push_back(Light{});
        }

        if (ImGui::Button("Reset Camera")) {
            scene.getActiveCamera().reset();
        }

        ImGui::Separator();
        ImGui::Checkbox("Use OpenGL Render Path", &scene.useOpenGLRenderPath);

        if (!scene.useOpenGLRenderPath) {
            ImGui::Separator();
            ImGui::Text("Post-Processing & Shadows (CPU path):");
            ImGui::Checkbox("Enable Shadows", &scene.useShadows);
            ImGui::Checkbox("Enable SSAO", &scene.useSSAO);
            ImGui::Checkbox("Enable Frustum Culling", &scene.useFrustumCulling);
            {
                // Stats come from whichever buffer was actually rendered into this frame.
                const RenderBuffers& statsRb = scene.useSupersampling ? ssRb : rb;
                ImGui::Text("Rendered %d / %d models (%d culled)",
                            statsRb.lastFrameModelCount - statsRb.lastFrameCulledCount,
                            statsRb.lastFrameModelCount, statsRb.lastFrameCulledCount);
            }
            ImGui::Checkbox("Enable Supersampling (2x)", &scene.useSupersampling);
            ImGui::TextDisabled("Renders at double resolution and downsamples, smoother edges, slower.");

            ImGui::Checkbox("Enable Fog", &scene.useFog);
            if (scene.useFog) {
                ImGui::ColorEdit3("Fog Color", &scene.fogColor[0]);
                ImGui::SliderFloat("Fog Start", &scene.fogStart, 0.0f, 50.0f);
                ImGui::SliderFloat("Fog End", &scene.fogEnd, 0.0f, 50.0f);
            }

            ImGui::Checkbox("Enable Full-Screen Blur", &scene.useFullScreenBlur);

            ImGui::Checkbox("Enable Bloom", &scene.useBloom);
            if (scene.useBloom) {
                ImGui::SliderFloat("Bloom Threshold", &scene.bloomThreshold, 0.0f, 1.0f);
                ImGui::SliderFloat("Bloom Intensity", &scene.bloomIntensity, 0.0f, 2.0f);
            }

            ImGui::Checkbox("Enable Skybox", &scene.useSkybox);
            if (scene.useSkybox) {
                ImGui::ColorEdit3("Sky Horizon Color", &scene.skyHorizonColor[0]);
                ImGui::ColorEdit3("Sky Zenith Color", &scene.skyZenithColor[0]);
            }

            ImGui::Checkbox("Enable Depth of Field", &scene.useDepthOfField);
            if (scene.useDepthOfField) {
                ImGui::SliderFloat("Focus Distance", &scene.focusDistance, 0.0f, 30.0f);
                ImGui::SliderFloat("Focus Range", &scene.focusRange, 0.0f, 10.0f);
                ImGui::SliderFloat("DoF Blur Distance", &scene.dofBlurDistance, 0.1f, 20.0f);
            }
        } else {
            ImGui::TextDisabled("Post-processing (fog/SSAO/bloom/blur/skybox/DoF/shadows) is CPU-path only, hidden while the GL path is active.");
        }

        ImGui::Separator();
        ImGui::Text("Interaction:");
        ImGui::SliderFloat("Transform Step Size", &scene.transformStepSize, 0.005f, 0.5f, "%.3f");
        ImGui::TextDisabled("Controls how far Position/Rotation/Scale move per pixel dragged below.");

        ImGui::Separator();
        ImGui::Text("Scene File:");
        ImGui::InputText("Path", sceneFilePathBuf.data(), sceneFilePathBuf.size());
        if (ImGui::Button("Save Scene")) {
            saveSceneToFile(sceneFilePathBuf.data());
        }
        ImGui::SameLine();
        if (ImGui::Button("Load Scene")) {
            loadSceneFromFile(sceneFilePathBuf.data());
        }
        if (!sceneIOStatus.empty()) {
            ImGui::TextWrapped("%s", sceneIOStatus.c_str());
        }

        ImGui::Separator();
        ImGui::Text("Models in Scene:");

        for (int i = 0; i < scene.models.size(); ++i) {
            auto& model = scene.models[i];
            std::string label = "Model " + std::to_string(i) + (model.isDeletable ? "" : " (Floor)");
            if (i == scene.activeModelIndex) label += " (Selected)";

            ImGui::PushID(i);

            const bool headerOpen = ImGui::CollapsingHeader(label.c_str());
            if (ImGui::IsItemClicked()) {
                scene.activeModelIndex = i;
            }

            if (headerOpen) {
                int shadingIndex = static_cast<int>(model.shadingMode);
                const char* shadingNames[] = { "Flat", "Gouraud", "Phong" };
                if (ImGui::Combo("Shading Mode", &shadingIndex, shadingNames, 3)) {
                    model.shadingMode = static_cast<ShadingMode>(shadingIndex);
                }

                // GL-path-only: the CPU shader (PhongShader.cpp) never reads
                // any of these, so they only show while the GL render path
                // is active, matching the post-processing gating above.
                if (scene.useOpenGLRenderPath) {
                    int uvModeIndex = static_cast<int>(model.uvMode);
                    const char* uvModeNames[] = { "Model UVs", "Planar Projection", "Spherical Projection" };
                    if (ImGui::Combo("UV Source", &uvModeIndex, uvModeNames, 3)) {
                        model.uvMode = static_cast<UVGenerationMode>(uvModeIndex);
                    }
                    if (model.uvMode != UVGenerationMode::UseOwnUVs) {
                        ImGui::SliderFloat("UV Scale", &model.uvScale, 0.05f, 5.0f);
                    }

                    int envModeIndex = static_cast<int>(model.envMapMode);
                    const char* envModeNames[] = { "Off", "Reflect", "Refract" };
                    if (ImGui::Combo("Environment Map", &envModeIndex, envModeNames, 3)) {
                        model.envMapMode = static_cast<EnvMapMode>(envModeIndex);
                    }
                    if (model.envMapMode != EnvMapMode::Off) {
                        ImGui::SliderFloat("Env Map Strength", &model.envMapStrength, 0.0f, 1.0f);
                    }
                    if (model.envMapMode == EnvMapMode::Refract) {
                        ImGui::SliderFloat("Index of Refraction", &model.envMapIOR, 1.0f, 2.5f);
                    }

                    ImGui::Checkbox("Toon Shading", &model.useToonShading);
                    if (model.useToonShading) {
                        ImGui::SliderInt("Toon Levels", &model.toonLevels, 2, 8);
                    }
                    ImGui::Checkbox("Silhouette Outline", &model.useSilhouette);
                    if (model.useSilhouette) {
                        ImGui::SliderFloat("Silhouette Thickness", &model.silhouetteThickness, 0.0f, 0.1f, "%.3f");
                        ImGui::ColorEdit3("Silhouette Color", &model.silhouetteColor[0]);
                    }

                    int colorAnimIndex = static_cast<int>(model.colorAnimMode);
                    const char* colorAnimNames[] = { "Off", "Hue Cycle", "Pulse" };
                    if (ImGui::Combo("Color Animation", &colorAnimIndex, colorAnimNames, 3)) {
                        model.colorAnimMode = static_cast<ColorAnimMode>(colorAnimIndex);
                    }
                    if (model.colorAnimMode != ColorAnimMode::None) {
                        ImGui::SliderFloat("Color Anim Speed", &model.colorAnimSpeed, 0.1f, 5.0f);
                    }

                    ImGui::Checkbox("Vertex Animation", &model.useVertexAnimation);
                    if (model.useVertexAnimation) {
                        ImGui::SliderFloat("Vertex Anim Amplitude", &model.vertexAnimAmplitude, 0.0f, 0.3f, "%.3f");
                        ImGui::SliderFloat("Vertex Anim Speed", &model.vertexAnimSpeed, 0.1f, 5.0f);
                    }

                    int proceduralTexIndex = static_cast<int>(model.proceduralTexMode);
                    const char* proceduralTexNames[] = { "Off", "Marble", "Wood" };
                    if (ImGui::Combo("Procedural Texture", &proceduralTexIndex, proceduralTexNames, 3)) {
                        model.proceduralTexMode = static_cast<ProceduralTextureMode>(proceduralTexIndex);
                    }
                    if (model.proceduralTexMode != ProceduralTextureMode::Off) {
                        ImGui::SliderFloat("Procedural Tex Scale", &model.proceduralTexScale, 0.1f, 5.0f);
                    }
                }

                ImGui::Checkbox("Use Diffuse", &model.useDiffuse);
                ImGui::Checkbox("Use Normal Map", &model.useNormalMap);
                ImGui::Checkbox("Use Specular Map", &model.useSpecularMap);

                // CPU-path-only debug draws: renderModelsGL() never looks at
                // any of these, only Renderer.cpp does.
                if (!scene.useOpenGLRenderPath) {
                    ImGui::Checkbox("Use Wireframe", &model.useWireframe);
                    ImGui::Checkbox("Fill Color", &model.fillColor);
                    ImGui::Checkbox("Vertex Normals", &model.useVertexNormalDrawing);
                    ImGui::Checkbox("Face Normals", &model.useFaceNormalDrawing);
                    ImGui::Checkbox("Bounding Box", &model.useBBoxDrawing);
                }

                // Drag speed = the adjustable step size above. Rotation is
                // scaled up since it's in degrees, not scene units.
                const float posStep = scene.transformStepSize;
                const float rotStep = scene.transformStepSize * 20.0f;

                ImGui::Text("World Frame (placement):");
                ImGui::DragFloat3("Position", &model.position[0], posStep, -5.0f, 5.0f);
                ImGui::DragFloat3("Rotation", &model.rotation[0], rotStep, 0.0f, 360.0f);

                float uniformScale = model.scale.x();
                if (ImGui::DragFloat("Scale", &uniformScale, posStep, 0.1f, 3.0f)) {
                    model.scale = {uniformScale, uniformScale, uniformScale};
                }

                if (ImGui::TreeNode("Model Frame (local, applied before placement)")) {
                    ImGui::DragFloat3("Local Position", &model.modelFramePosition[0], posStep, -5.0f, 5.0f);
                    ImGui::DragFloat3("Local Rotation", &model.modelFrameRotation[0], rotStep, 0.0f, 360.0f);

                    float uniformModelScale = model.modelFrameScale.x();
                    if (ImGui::DragFloat("Local Scale", &uniformModelScale, posStep, 0.1f, 3.0f)) {
                        model.modelFrameScale = {uniformModelScale, uniformModelScale, uniformModelScale};
                    }

                    if (ImGui::Button("Reset Model Frame")) {
                        model.modelFramePosition = {0, 0, 0};
                        model.modelFrameRotation = {0, 0, 0};
                        model.modelFrameScale = {1, 1, 1};
                    }

                    ImGui::TreePop();
                }

                if (ImGui::TreeNode("Material")) {
                    ImGui::ColorEdit3("Diffuse Color", &model.material.diffuseColor[0]);
                    ImGui::ColorEdit3("Specular Color", &model.material.specularColor[0]);
                    ImGui::ColorEdit3("Emissive", &model.material.emissive[0]);

                    ImGui::Checkbox("Non-Uniform (UV Gradient)", &model.material.useNonUniformMaterial);
                    if (model.material.useNonUniformMaterial) {
                        ImGui::ColorEdit3("Secondary Diffuse Color", &model.material.diffuseColorSecondary[0]);
                        ImGui::TextDisabled("Blends Diffuse Color -> Secondary across the mesh's UV.");
                    }

                    ImGui::Checkbox("Use Blending (Transparency)", &model.useBlending);
                    if (model.useBlending) {
                        ImGui::SliderFloat("Opacity", &model.material.opacity, 0.0f, 1.0f);
                        ImGui::TextDisabled("CPU render path only for now. Drawn after opaque models, sorted back-to-front by camera distance.");
                    }

                    ImGui::TreePop();
                }

                if (model.isDeletable && ImGui::Button("Remove")) {
                    scene.models.erase(scene.models.begin() + i);

                    if (scene.activeModelIndex == i) {
                        scene.activeModelIndex = -1;
                    } else if (scene.activeModelIndex > i) {
                        scene.activeModelIndex--;
                    }
                }
            }

            ImGui::PopID();
        }
        ImGui::End();

        // Model Loader
        ImGui::Begin("Model Library");

        ImGui::Text("Primitives:");
        constexpr float PRIMITIVE_SPAWN_DISTANCE = 3.0f;
        const Camera& spawnCam = scene.getActiveCamera();
        const Vec3f spawnPos = spawnCam.pos + (spawnCam.lookAt - spawnCam.pos).normalize() * PRIMITIVE_SPAWN_DISTANCE;

        if (ImGui::Button("Add Cube")) {
            addPrimitiveToScene(scene, PrimitiveType::Cube, spawnPos);
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Pyramid")) {
            addPrimitiveToScene(scene, PrimitiveType::Pyramid, spawnPos);
        }
        ImGui::SameLine();
        if (ImGui::Button("Add Tetrahedron")) {
            addPrimitiveToScene(scene, PrimitiveType::Tetrahedron, spawnPos);
        }

        ImGui::Separator();
        ImGui::Text("Models:");
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
        ImGui::Checkbox("Show Other Cameras Positions", &scene.showCameraIcons);

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
        ImGui::SliderFloat("Movement Speed", &currCam.moveSpeed, 0.1f, 10.0f, "%.1f");

        ImGui::Separator();
        ImGui::Text("Projection:");

        static const char* projectionTypeNames[] = { "Perspective (Simple)", "Perspective (FOV)", "Orthographic" };
        int projectionTypeIdx = static_cast<int>(currCam.projectionType);
        if (ImGui::Combo("Type", &projectionTypeIdx, projectionTypeNames, IM_COUNTOF(projectionTypeNames))) {
            currCam.projectionType = static_cast<ProjectionType>(projectionTypeIdx);
        }

        switch (currCam.projectionType) {
            case ProjectionType::PerspectiveSimple:
                ImGui::DragFloat("Focal Length", &currCam.focalLength, 0.01f, 0.1f, 10.0f);
                break;
            case ProjectionType::PerspectiveFov:
                ImGui::SliderFloat("Field of View", &currCam.fov, 10.0f, 120.0f, "%.0f deg");
                ImGui::DragFloat("Near Plane", &currCam.nearPlane, 0.01f, 0.001f, currCam.farPlane - 0.01f);
                ImGui::DragFloat("Far Plane", &currCam.farPlane, 0.5f, currCam.nearPlane + 0.01f, 1000.0f);
                break;
            case ProjectionType::Orthographic:
                ImGui::DragFloat("View Size", &currCam.orthoSize, 0.05f, 0.1f, 20.0f);
                ImGui::DragFloat("Near Plane", &currCam.nearPlane, 0.01f, 0.001f, currCam.farPlane - 0.01f);
                ImGui::DragFloat("Far Plane", &currCam.farPlane, 0.5f, currCam.nearPlane + 0.01f, 1000.0f);
                break;
        }

        ImGui::Separator();
        if (scene.hasActiveModel()) {
            if (ImGui::Button("Focus On Selected Model")) {
                focusCameraOnModel(currCam, *scene.getActiveModel());
            }
            if (ImGui::Button("Zoom In")) {
                zoomCameraTowardModel(currCam, *scene.getActiveModel(), MODEL_ZOOM_STEP);
            }
            ImGui::SameLine();
            if (ImGui::Button("Zoom Out")) {
                zoomCameraTowardModel(currCam, *scene.getActiveModel(), -MODEL_ZOOM_STEP);
            }
        } else {
            ImGui::TextDisabled("Select a model to focus/zoom on it.");
        }

        ImGui::End();

        Vec3f front;
        front.x() = cos(currCam.yaw * M_PI / 180.0f) * cos(currCam.pitch * M_PI / 180.0f);
        front.y() = sin(currCam.pitch * M_PI / 180.0f);
        front.z() = sin(currCam.yaw * M_PI / 180.0f) * cos(currCam.pitch * M_PI / 180.0f);
        Vec3f forward = front.normalize();

        Camera &cam = scene.getActiveCamera();
        float speed = cam.moveSpeed * deltaTime;
        Vec3f right = cross(forward, cam.up).normalize();

        // W/S dolly toward/away from the selected model, same as the Zoom
        // In/Out buttons but continuous. No selection = plain fly forward/back, as before.
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) {
            if (scene.hasActiveModel()) {
                zoomCameraTowardModel(cam, *scene.getActiveModel(), speed);
            } else {
                cam.pos = cam.pos + (forward * speed);
            }
        }
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) {
            if (scene.hasActiveModel()) {
                zoomCameraTowardModel(cam, *scene.getActiveModel(), -speed);
            } else {
                cam.pos = cam.pos - (forward * speed);
            }
        }

        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) {
            cam.pos = cam.pos + (right * speed);
        }

        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) {
            cam.pos = cam.pos - (right * speed);
        }

        cam.lookAt = cam.pos + forward;

        int fbWidth, fbHeight;
        glfwGetFramebufferSize(window, &fbWidth, &fbHeight);
        glViewport(0, 0, fbWidth, fbHeight);

        if (scene.useOpenGLRenderPath) {
            // GL render path: real GLSL draw straight to the default framebuffer,
            // needs its own depth test/clear since the CPU path below never uses one.
            glEnable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            renderModelsGL();
        } else {
            glDisable(GL_DEPTH_TEST);

            if (scene.useSupersampling) {
                Renderer::render(scene, ssRb);
                downsampleSupersampleBuffer();
            } else {
                Renderer::render(scene, rb);
            }

            glClear(GL_COLOR_BUFFER_BIT);

            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, rb.width, rb.height, GL_RGB, GL_UNSIGNED_BYTE, rb.colorBuffer.data());

            glUseProgram(shaderProgram);
            glBindVertexArray(VAO);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        }

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
    // Hold R to rotate the active model by dragging, same idea as holding Q
    // to look around with the camera below, just aimed at the model instead.
    if (glfwGetKey(window, GLFW_KEY_R) == GLFW_PRESS && scene.hasActiveModel()) {
        firstMouse = true;
        firstMouseModelDrag = true;
        firstMouseModelScale = true;

        if (firstMouseModelRotate) {
            lastModelRotateX = xPos;
            lastModelRotateY = yPos;
            firstMouseModelRotate = false;
        }

        const auto xOffset = xPos - lastModelRotateX;
        const auto yOffset = yPos - lastModelRotateY;
        lastModelRotateX = xPos;
        lastModelRotateY = yPos;

        ModelInstance* model = scene.getActiveModel();
        model->rotation.y() += static_cast<float>(xOffset) * 0.3f;
        model->rotation.x() += static_cast<float>(yOffset) * 0.3f;
        return;
    }
    firstMouseModelRotate = true;

    // Hold M to move the active model by dragging. The mouse ray is
    // intersected against a camera-facing plane through the model, so the
    // model follows the cursor 1:1 in world space instead of using an
    // arbitrary pixels-to-units scale that would feel wrong at different
    // zoom levels.
    if (glfwGetKey(window, GLFW_KEY_M) == GLFW_PRESS && scene.hasActiveModel()) {
        firstMouse = true;
        firstMouseModelScale = true;

        ModelInstance* model = scene.getActiveModel();
        const PickRay ray = screenToWorldRay(xPos, yPos);
        const Camera& cam = scene.getActiveCamera();
        const Vec3f planeNormal = (cam.lookAt - cam.pos).normalize();

        if (firstMouseModelDrag) {
            dragPlanePoint = model->position;
        }

        const float denom = dotProduct(planeNormal, ray.direction);
        if (std::abs(denom) > 1e-5f) {
            const float t = dotProduct(planeNormal, dragPlanePoint - ray.origin) / denom;
            const Vec3f hit = ray.origin + ray.direction * t;

            if (!firstMouseModelDrag) {
                model->position = model->position + (hit - lastDragHit);
            }
            lastDragHit = hit;
            firstMouseModelDrag = false;
        }
        return;
    }
    firstMouseModelDrag = true;

    // Hold N to scale the active model by dragging vertically, up grows it,
    // down shrinks it. Clamped so you can't drag it down to zero or negative.
    if (glfwGetKey(window, GLFW_KEY_N) == GLFW_PRESS && scene.hasActiveModel()) {
        firstMouse = true;

        if (firstMouseModelScale) {
            lastModelScaleY = yPos;
            firstMouseModelScale = false;
        }

        const auto yOffset = lastModelScaleY - yPos;
        lastModelScaleY = yPos;

        ModelInstance* model = scene.getActiveModel();
        const float newScale = std::max(0.1f, model->scale.x() + static_cast<float>(yOffset) * 0.01f);
        model->scale = {newScale, newScale, newScale};
        return;
    }
    firstMouseModelScale = true;

    if (glfwGetKey(window, GLFW_KEY_Q) != GLFW_PRESS) {
        firstMouse = true;
        return;
    }

    if (firstMouse) {
        lastX = xPos;
        lastY = yPos;
        firstMouse = false;
    }

    auto xOffset = xPos - lastX;
    auto yOffset = lastY - yPos;

    lastX = xPos;
    lastY = yPos;

    xOffset *= 0.1f;
    yOffset *= 0.1f;

    Camera &activeCam = scene.getActiveCamera();
    activeCam.yaw += xOffset;
    activeCam.pitch += yOffset;

    // Avoiding Gimbal lock
    if (activeCam.pitch > 89.0f) activeCam.pitch = 89.0f;
    if (activeCam.pitch < -89.0f) activeCam.pitch = -89.0f;
}

void Application::mouse_callback(GLFWwindow *window, const double xPos, const double yPos) {
    ImGui_ImplGlfw_CursorPosCallback(window, xPos, yPos);
    if (ImGui::GetIO().WantCaptureMouse) return;

    if (auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window))) {
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
        const PickRay ray = app->screenToWorldRay(xPos, yPos);

        // BVH over this frame's model AABBs, instead of the linear scan this
        // used to be.
        std::vector<AABB> worldBoxes;
        std::vector<int> pickableIndices;
        worldBoxes.reserve(app->scene.models.size());
        pickableIndices.reserve(app->scene.models.size());
        for (int i = 0; i < app->scene.models.size(); ++i) {
            const auto& model = app->scene.models[i];
            worldBoxes.push_back(model.getWorldAABB());
            if (model.isDeletable) pickableIndices.push_back(i);
        }

        BVH pickingBVH;
        pickingBVH.build(worldBoxes, pickableIndices);

        // Clicking a model selects it as the active model rather than deleting it.
        // Clicking empty space deselects.
        app->scene.activeModelIndex = pickingBVH.closestHit(ray.origin, ray.direction);
    }
}

PickRay Application::screenToWorldRay(const double mouseX, const double mouseY) {
    int winWidth = width, winHeight = height;
    glfwGetWindowSize(window, &winWidth, &winHeight);
    if (winWidth <= 0) winWidth = 1;
    if (winHeight <= 0) winHeight = 1;

    const auto xNdc = (2 * mouseX) / winWidth - 1;
    const auto yNdc = 1 - (2 * mouseY) / winHeight;

    const Camera& cam = scene.getActiveCamera();
    const float aspectRatio = static_cast<float>(winWidth) / static_cast<float>(winHeight);
    const Matrix4f4 projection = buildCameraProjection(cam, aspectRatio);
    const Matrix4f4 view = Matrix4f4::lookat(cam.pos, cam.lookAt, cam.up);
    const Matrix4f4 invProjection = projection.inverse4x4();
    const Matrix4f4 invView = view.inverse4x4();

    // Near plane = NDC z +1 in this renderer's convention (see Matrix4f4::perspective()).
    const Vec4f rayClipNear = {xNdc, yNdc, 1.0f, 1.0f};

    if (cam.projectionType == ProjectionType::Orthographic) {
        const Vec4f eyeNear = invProjection * rayClipNear;
        const Vec4f worldNear = invView * Vec4f(eyeNear.x(), eyeNear.y(), eyeNear.z(), 1.0f);
        const Vec3f origin(worldNear.x(), worldNear.y(), worldNear.z());
        const Vec3f direction = (cam.lookAt - cam.pos).normalize();
        return { origin, direction };
    }

    const bool isSimple = cam.projectionType == ProjectionType::PerspectiveSimple;
    const float copZ = isSimple ? cam.focalLength : 0.0f;

    Vec4f rayEye = invProjection * rayClipNear;
    rayEye = {rayEye.x(), rayEye.y(), -copZ - (isSimple ? 0.0f : 1.0f), 0.0f};

    const Vec4f rayWorld = invView * rayEye;
    const Vec3f direction = Vec3f(rayWorld.x(), rayWorld.y(), rayWorld.z()).normalize();

    if (isSimple) {
        const Vec4f copEye = {0.0f, 0.0f, copZ, 1.0f};
        const Vec4f originWorld = invView * copEye;
        return { Vec3f(originWorld.x(), originWorld.y(), originWorld.z()), direction };
    }

    return { cam.pos, direction };
}

void Application::framebuffer_size_callback(GLFWwindow* window, const int width, const int height) {
    if (auto *app = static_cast<Application *>(glfwGetWindowUserPointer(window))) {
        app->resizeBuffers(width, height);
    }
}

void Application::resizeBuffers(const int newWidth, const int newHeight) {
    if (newWidth <= 0 || newHeight <= 0) return;

    width = newWidth;
    height = newHeight;

    rb.resize(width, height);
    ssRb.resize(width * SUPERSAMPLE_FACTOR, height * SUPERSAMPLE_FACTOR);

    glBindTexture(GL_TEXTURE_2D, texture);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, NULL);
}

void Application::downsampleSupersampleBuffer() {
    constexpr int factor = SUPERSAMPLE_FACTOR;
    constexpr int sampleCount = factor * factor;

    for (int y = 0; y < rb.height; ++y) {
        for (int x = 0; x < rb.width; ++x) {
            int rSum = 0, gSum = 0, bSum = 0;

            for (int sy = 0; sy < factor; ++sy) {
                for (int sx = 0; sx < factor; ++sx) {
                    const int srcX = x * factor + sx;
                    const int srcY = y * factor + sy;
                    const int srcIdx = (srcX + srcY * ssRb.width) * 3;

                    rSum += ssRb.colorBuffer[srcIdx];
                    gSum += ssRb.colorBuffer[srcIdx + 1];
                    bSum += ssRb.colorBuffer[srcIdx + 2];
                }
            }

            const int dstIdx = (x + y * rb.width) * 3;
            rb.colorBuffer[dstIdx]     = static_cast<unsigned char>(rSum / sampleCount);
            rb.colorBuffer[dstIdx + 1] = static_cast<unsigned char>(gSum / sampleCount);
            rb.colorBuffer[dstIdx + 2] = static_cast<unsigned char>(bSum / sampleCount);
        }
    }
}

void Application::addModelToScene(const std::string& folderPath, const std::string& objFile,
                                  const std::string& diffFile, const std::string& nmFile,
                                  const std::string& specFile) {
    const std::string resourceKey = folderPath + objFile;
    if (resourceCache.find(resourceKey) == resourceCache.end()) {
        resourceCache[resourceKey] = std::make_shared<ModelResource>(
            folderPath, objFile, diffFile, nmFile, specFile
        );
    }

    ModelInstance newModel(resourceCache[resourceKey], false);
    newModel.position = { 0, 0, 0 };
    scene.addModel(newModel);
}

void Application::saveSceneToFile(const std::string& path) {
    if (SceneIO::saveScene(path, scene)) {
        sceneIOStatus = "Saved to " + path;
    } else {
        sceneIOStatus = "Failed to save to " + path;
    }
}

void Application::loadSceneFromFile(const std::string& path) {
    std::string error;
    if (SceneIO::loadScene(path, scene, resourceCache, &error)) {
        sceneIOStatus = "Loaded " + path;
    } else {
        sceneIOStatus = "Failed to load " + path + (error.empty() ? "" : (": " + error));
    }
}
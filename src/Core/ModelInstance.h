#ifndef RENDERER_MODELINSTANCE_H
#define RENDERER_MODELINSTANCE_H

#include "Matrix.h"
#include "../IO/tgaimage.h"
#include "../IO/ModelLoader.h"

struct AABB {
    Vec3f min;
    Vec3f max;

    AABB() {
        min = {std::numeric_limits<float>::max(), std::numeric_limits<float>::max(), std::numeric_limits<float>::max()};
        max = {std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest(), std::numeric_limits<float>::lowest()};
    }
};

struct ModelResource {
    ModelLoader model;
    TGAImage diffuse;
    TGAImage normal;
    TGAImage specular;
    AABB localBBox;

    // Source paths this resource was built from, kept around so a scene can
    // be saved to / reloaded from disk (see SceneIO.h) without having to
    // guess back what a ModelInstance was pointing at.
    std::string modelRoot;
    std::string objPath;
    std::string diffPath;
    std::string nmPath;
    std::string specPath;

    // Assumption - all the model files are in the same directory.
    ModelResource(const std::string& modelRoot,
                  const std::string &objPath,
                  const std::string &diffPath,
                  const std::string &nmPath,
                  const std::string &specPath)
        : model(modelRoot + objPath),
          modelRoot(modelRoot), objPath(objPath), diffPath(diffPath),
          nmPath(nmPath), specPath(specPath) {

        diffuse.read_tga_file(modelRoot + diffPath);
        normal.read_tga_file(modelRoot + nmPath);
        specular.read_tga_file(modelRoot + specPath);

        diffuse.flip_vertically();
        normal.flip_vertically();
        specular.flip_vertically();

        for (const auto& v : model.getVertices()) {
            for (int i = 0; i < 3; ++i) {
                localBBox.min[i] = std::min(localBBox.min[i], v[i]);
                localBBox.max[i] = std::max(localBBox.max[i], v[i]);
            }
        }
    }
};

struct Material {
    Vec3f emissive = {0, 0, 0};          // Self-illumination (ignores lighting/shadows)
    Vec3f diffuseColor = {1, 1, 1};      // Base tint or flat color (multiplies texture)
    Vec3f specularColor = {1, 1, 1};     // Specular highlight color

    float opacity = 1.0f; // 1 = fully opaque; only used when useBlending is on

    bool useNonUniformMaterial = false;
    Vec3f diffuseColorSecondary = {1.0f, 0.5f, 0.0f};
};

enum class ShadingMode {
    Flat,       // One normal per face (faceted look)
    Gouraud,    // Lighting computed per vertex, interpolated
    Phong       // Normal interpolated, lighting computed per pixel
};

enum class UVGenerationMode {
    UseOwnUVs,  // Model's loaded texture coordinates
    Planar,     // Planar projection-based generator
    Spherical   // Spherical projection-based generator
};

enum class EnvMapMode {
    Off,
    Reflect,
    Refract
};

enum class ColorAnimMode {
    None,
    HueCycle,
    Pulse
};

enum class ProceduralTextureMode {
    Off,
    Marble,
    Wood
};


struct ModelInstance {
    std::shared_ptr<ModelResource> resource;
    Material material;
    ShadingMode shadingMode = ShadingMode::Phong;
    UVGenerationMode uvMode = UVGenerationMode::UseOwnUVs;
    float uvScale = 1.0f;
    EnvMapMode envMapMode = EnvMapMode::Off;
    float envMapStrength = 0.5f;
    float envMapIOR = 1.5f;

    bool useToonShading = false;
    int toonLevels = 4;
    bool useSilhouette = false;
    float silhouetteThickness = 0.02f;
    Vec3f silhouetteColor = {0.0f, 0.0f, 0.0f};

    ColorAnimMode colorAnimMode = ColorAnimMode::None;
    float colorAnimSpeed = 1.0f;

    bool useVertexAnimation = false;
    float vertexAnimAmplitude = 0.05f;
    float vertexAnimSpeed = 1.0f;

    ProceduralTextureMode proceduralTexMode = ProceduralTextureMode::Off;
    float proceduralTexScale = 1.0f;

    bool useAlphaTest;

    bool useBlending = false; // alpha-blends instead of z-testing opaque

    bool isDeletable = true;
    bool useDiffuse = true;
    bool useSpecularMap = true;
    bool useNormalMap = true;
    bool useWireframe = false;
    bool useVertexNormalDrawing = false;
    bool useFaceNormalDrawing = false;
    bool useBBoxDrawing = false;
    bool fillColor = true;

    // World-frame placement: where this instance sits in the scene.
    Vec3f position = {0, 0, 0};
    Vec3f rotation = {0, 0, 0}; // Euler angles in degrees
    Vec3f scale = {1, 1, 1};

    Vec3f modelFramePosition = {0, 0, 0};
    Vec3f modelFrameRotation = {0, 0, 0}; // Euler angles in degrees
    Vec3f modelFrameScale = {1, 1, 1};    // Local orientation/scale fix

    ModelInstance(std::shared_ptr<ModelResource> res, const bool useAlpha)
        : resource(std::move(res)), useAlphaTest(useAlpha) {}

    [[nodiscard]] Matrix4f4 getModelFrameMatrix() const {
        const Matrix4f4 T = Matrix4f4::translation(modelFramePosition);
        const Matrix4f4 S = Matrix4f4::scale(modelFrameScale.x(), modelFrameScale.y(), modelFrameScale.z());

        const Matrix4f4 Rx = Matrix4f4::rotationX(modelFrameRotation.x());
        const Matrix4f4 Ry = Matrix4f4::rotationY(modelFrameRotation.y());
        const Matrix4f4 Rz = Matrix4f4::rotationZ(modelFrameRotation.z());

        return T * (Rx * Ry * Rz) * S;
    }

    [[nodiscard]] Matrix4f4 getModelMatrix() const {
        const Matrix4f4 T = Matrix4f4::translation(position);
        const Matrix4f4 S = Matrix4f4::scale(scale.x(), scale.y(), scale.z());

        // Rotation Order: Z -> Y -> X
        const Matrix4f4 Rx = Matrix4f4::rotationX(rotation.x());
        const Matrix4f4 Ry = Matrix4f4::rotationY(rotation.y());
        const Matrix4f4 Rz = Matrix4f4::rotationZ(rotation.z());

        const Matrix4f4 worldTransform = T * (Rx * Ry * Rz) * S;

        // Model frame first (fix the geometry's own orientation), then place it in the world.
        return worldTransform * getModelFrameMatrix();
    }

    void updateBBox() const {
        for (const auto& v: resource->model.getVertices()) {
            for (int i = 0 ; i < 3; ++i) {
                resource->localBBox.min[i] = std::min(resource->localBBox.min[i], v[i]);
                resource->localBBox.max[i] = std::max(resource->localBBox.max[i], v[i]);
            }
        }
    }

    [[nodiscard]] AABB getWorldAABB() const {
        const Matrix4f4 modelMat = getModelMatrix();
        const auto& localBBox = resource->localBBox;

        Vec3f localCorners[8] = {
            {localBBox.min.x(), localBBox.min.y(), localBBox.min.z()},
            {localBBox.min.x(), localBBox.min.y(), localBBox.max.z()},
            {localBBox.min.x(), localBBox.max.y(), localBBox.min.z()},
            {localBBox.min.x(), localBBox.max.y(), localBBox.max.z()},
            {localBBox.max.x(), localBBox.min.y(), localBBox.min.z()},
            {localBBox.max.x(), localBBox.min.y(), localBBox.max.z()},
            {localBBox.max.x(), localBBox.max.y(), localBBox.min.z()},
            {localBBox.max.x(), localBBox.max.y(), localBBox.max.z()}
        };

        auto worldAABB = AABB();
        for (auto corner : localCorners) {
            Vec4f transformed = modelMat * Vec4f(corner);
            Vec3f worldCorner = Vec3f(transformed);
            for (int j = 0; j < 3; ++j) {
                worldAABB.min[j] = std::min(worldAABB.min[j], worldCorner[j]);
                worldAABB.max[j] = std::max(worldAABB.max[j], worldCorner[j]);
            }
        }
        return worldAABB;
    }

    [[nodiscard]] static float RayBoxInterSection(Vec3f rayOrigin, Vec3f rayDir, Vec3f min, Vec3f max) {
        float tNear = std::numeric_limits<float>::lowest();
        float tFar = std::numeric_limits<float>::max();

        for (int i = 0; i < 3; i++) {
            if (std::abs(rayDir[i]) < GraphicsUtils::EPSILON) {
                // Ray is parallel to the slab. If origin is not within the slab, no hit.
                if (rayOrigin[i] < min[i] || rayOrigin[i] > max[i]) return -1.0f;
                continue;
            }

            float invDir = 1.0f / rayDir[i];
            float t1 = (min[i] - rayOrigin[i]) * invDir;
            float t2 = (max[i] - rayOrigin[i]) * invDir;

            float tStart = std::min(t1, t2);
            float tEnd = std::max(t1, t2);

            tNear = std::max(tNear, tStart);
            tFar = std::min(tFar, tEnd);

            if (tNear > tFar) return -1.0f;
        }

        if (tFar < 0) return -1.0f;
        return std::max(tNear, 0.0f);
    }
};

#endif //RENDERER_MODELINSTANCE_H
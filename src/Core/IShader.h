#ifndef RENDERER_SHADER_H
#define RENDERER_SHADER_H

#include "../Math/Vec.h"
#include "../Math/Matrix.h"
#include "../IO/tgaimage.h"

struct Varyings {
    Vec3f screenPos;
    Vec2f uv;
    Vec3f normal;
    Vec3f worldPos;
    Vec3f tangent;
    Vec3f bitangent;
    float invW{1.0f};
    Vec3f normalForBuffer;
};

struct Uniforms {
    Matrix4f4 model;
    Matrix4f4 modelView;
    Matrix4f4 projection;
    Matrix4f4 viewport;
    Matrix3f3 normalMatrix;
    Vec3f lightDir;
    Vec3f cameraPos;

    Matrix4f4 lightSpaceMatrix;

    Matrix4f4 lightProjView;
    const std::vector<float>* shadowMap = nullptr;
    int shadowWidth = 0;
    int shadowHeight = 0;
};

class IShader {
public:
    virtual ~IShader() = default;
    Uniforms uniforms;

    virtual Varyings vertex(const Vec3f& localPos,
                            const Vec3f& normal,
                            const Vec2f& uv,
                            const Vec3f& tangent,
                            const Vec3f& bitangent) = 0;

    virtual bool fragment(Varyings& varyings,
                          TGAColor &color) = 0;

    static Varyings interpolate(const Varyings& v1,
                                const Varyings& v2,
                                const Varyings& v3,
                                const Vec3f& barycentric);
};


#endif //RENDERER_SHADER_H
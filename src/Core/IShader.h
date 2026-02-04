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
    float invW{1.0f};
};

struct Uniforms {
    Matrix4f4 model;
    Matrix4f4 modelView;
    Matrix4f4 projection;
    Matrix4f4 viewport;
    Matrix3f3 normalMatrix;
    Vec3f lightDir;
    Vec3f cameraPos;
};

class IShader {
public:
    virtual ~IShader() = default;
    Uniforms uniforms;

    virtual Varyings vertex(const Vec3f& localPos, const Vec3f& normal, const Vec2f& uv) = 0;
    virtual bool fragment(const Varyings& varyings, TGAColor &color) = 0;

    static Varyings interpolate(const Varyings& v1, const Varyings& v2, const Varyings& v3, const Vec3f& barycentric) {
        Varyings res;

        res.screenPos = v1.screenPos * barycentric.x() + v2.screenPos * barycentric.y() + v3.screenPos * barycentric.z();
        res.invW = v1.invW * barycentric.x() + v2.invW * barycentric.y() + v3.invW * barycentric.z();
        res.uv = v1.uv * barycentric.x() + v2.uv * barycentric.y() + v3.uv * barycentric.z();
        res.normal = v1.normal * barycentric.x() + v2.normal * barycentric.y() + v3.normal * barycentric.z();

        res.worldPos = v1.worldPos * barycentric.x() + v2.worldPos * barycentric.y() + v3.worldPos * barycentric.z();

        return res;
    }
};


#endif //RENDERER_SHADER_H
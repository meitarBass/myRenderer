#ifndef RENDERER_DEPTHSHADER_H
#define RENDERER_DEPTHSHADER_H

#include "../Core/IShader.h"

class DepthShader: public IShader {
public:
    DepthShader(const Uniforms& uniforms) {
        this->uniforms = uniforms;
    }

    Varyings vertex(const Vec3f& localPos, const Vec3f& normal, const Vec2f& uv,
                    const Vec3f& tangent, const Vec3f& bitangent) override {
        Varyings out;

        Vec4f clip = uniforms.projection * uniforms.modelView * Vec4f(localPos);
        out.invW = 1.0f / clip.w();

        const Vec4f ndc = clip * out.invW;
        const Vec4f screen = uniforms.viewport * ndc;

        out.screenPos = Vec3f(screen.x(), screen.y(), screen.z());

        return out;
    }


    bool fragment(const Varyings& varyings, TGAColor &color) override  {
        return false;
    }
};

#endif //RENDERER_DEPTHSHADER_H
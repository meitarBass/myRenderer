#ifndef RENDERER_DEPTHSHADER_H
#define RENDERER_DEPTHSHADER_H

#include "../Core/IShader.h"

class DepthShader: public IShader {
public:
    explicit DepthShader(const Uniforms& uniforms);

    Varyings vertex(const Vec3f& localPos,
                    const Vec3f& normal,
                    const Vec2f& uv,
                    const Vec3f& tangent,
                    const Vec3f& bitangent) override;

    bool fragment(Varyings& varyings, TGAColor &color) override;
};

#endif //RENDERER_DEPTHSHADER_H
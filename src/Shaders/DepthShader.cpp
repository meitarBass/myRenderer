
#include "DepthShader.h"

DepthShader::DepthShader(const Uniforms& uniforms)
{
    this->uniforms = uniforms;
}

Varyings DepthShader::vertex(const Vec3f& localPos,
                             const Vec3f& normal,
                             const Vec2f& uv,
                             const Vec3f& tangent,
                             const Vec3f& bitangent)
{
    Varyings out;

    Vec4f clip = uniforms.projection * uniforms.modelView * Vec4f(localPos);
    out.invW = 1.0f / clip.w();

    const Vec4f ndc = clip * out.invW;
    const Vec4f screen = uniforms.viewport * ndc;

    out.screenPos = Vec3f(screen.x(), screen.y(), screen.z());

    return out;
}

bool DepthShader::fragment(Varyings& varyings, TGAColor &color)
{
    return false;
}
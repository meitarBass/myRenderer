#ifndef RENDERER_PHONGSHADER_H
#define RENDERER_PHONGSHADER_H

#include "../Core/IShader.h"

class PhongShader : public IShader {
public:
    PhongShader(const TGAImage &diffuseMap,
                const TGAImage &normalMap,
                const TGAImage &specularMap,
                const Uniforms &uniforms,
                bool useAlphaTest);

    Varyings vertex(const Vec3f &localPos,
                    const Vec3f &normal,
                    const Vec2f &uv,
                    const Vec3f &tangent,
                    const Vec3f &bitangent) override;


    bool fragment(const Varyings &varyings, TGAColor &color) override;

private:
    const TGAImage &diffuseMap;
    const TGAImage &normalMap;
    const TGAImage &specularMap;
    const bool useAlphaTest;
};

#endif //RENDERER_PHONGSHADER_H

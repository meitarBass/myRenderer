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


    /**
     * Calculates the vertex position from the camera perspective
     * in order to later determine the pixel's color.
     * 
     * Please see IShader.h
     */
    Varyings vertex(const Vec3f &localPos,
                    const Vec3f &normal,
                    const Vec2f &uv,
                    const Vec3f &tangent,
                    const Vec3f &bitangent) override;


    /**
     * Determines the pixel's color using lighting, shadow,
     * and the input TGAColor.
     *
     * Please see IShader.h
     */
    bool fragment(Varyings &varyings, TGAColor &color) override;

private:
    const TGAImage &diffuseMap;
    const TGAImage &normalMap;
    const TGAImage &specularMap;
    const bool useAlphaTest;

    float calculateShadowFactor(const Vec3f& worldPos) const;

    Vec3f calculateNormal(const Vec2f& uv,
                          const Vec3f& T,
                          const Vec3f& B,
                          const Vec3f& N) const;

    void calculateLighting(const Vec3f& normal,
                           const Vec3f& worldPos,
                           const Vec2f& uv,
                           float shadowFactor,
                           float& outDiffuse,
                           float& outSpec) const;

    constexpr static int alphaTestLimit = 200;
    constexpr static float bias = 0.005;
    constexpr static float ambient = 0.3f;

};

#endif //RENDERER_PHONGSHADER_H

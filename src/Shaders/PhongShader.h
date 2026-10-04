#ifndef RENDERER_PHONGSHADER_H
#define RENDERER_PHONGSHADER_H

#include "../Core/IShader.h"
#include "../Core/ModelInstance.h"

class PhongShader : public IShader {
public:
    PhongShader(const TGAImage &diffuseMap,
                const TGAImage &normalMap,
                const TGAImage &specularMap,
                const Uniforms &uniforms,
                const Material &material,
                ShadingMode shadingMode,
                bool useAlphaTest,
                bool useBlending,
                bool useDiffuse,
                bool useNormalMap,
                bool useSpecularMap,
                bool useVertexNormalDrawing,
                bool useFaceNormalDrawing,
                bool useBBoxDrawing,
                bool fillColor,
                bool useWireframe);


    bool isVertexNormalDrawingEnabled() const override { return useVertexNormalDrawing; }
    bool isFaceNormalDrawingEnabled() const override { return useFaceNormalDrawing; }
    bool isBBoxDrawingEnabled() const override { return useBBoxDrawing; }
    bool usesFlatShading() const override { return shadingMode == ShadingMode::Flat; }
    bool usesBlending() const override { return useBlending; }

    Varyings vertex(const Vec3f &localPos, const Vec3f &normal, const Vec2f &uv,
                    const Vec3f &tangent, const Vec3f &bitangent) override;
    bool fragment(Varyings &varyings, TGAColor &color) override;

private:
    const TGAImage &diffuseMap;
    const TGAImage &normalMap;
    const TGAImage &specularMap;
    const Material material;
    const ShadingMode shadingMode;
    const bool useAlphaTest;
    const bool useBlending;
    const bool useDiffuse;
    const bool useNormalMap;
    const bool useSpecularMap;
    const bool fillColor;
    const bool useWireframe;
    const bool useVertexNormalDrawing;
    const bool useFaceNormalDrawing;
    const bool useBBoxDrawing;

    [[nodiscard]] float calculateShadowFactor(const Vec3f& worldPos, const Vec3f& normal) const;
    [[nodiscard]] Vec3f calculateNormal(const Vec2f& uv, const Vec3f& T, const Vec3f& B, const Vec3f& N) const;

    // Sums light contributions; outputs diffuse/specular tinted by light color
    void calculateLighting(const Vec3f& normal, const Vec3f& worldPos, const Vec2f& uv, float shadowFactor,
                           Vec3f& outDiffuse, Vec3f& outSpecular) const;

    constexpr static int alphaTestLimit = 200;

    // Slope-scaled shadow bias to prevent shadow acne at grazing angles
    constexpr static float minBias = 0.005f;
    constexpr static float maxBias = 0.03f;

};

#endif //RENDERER_PHONGSHADER_H

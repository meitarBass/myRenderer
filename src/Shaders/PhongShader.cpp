#include "PhongShader.h"

namespace {
    Vec3f directionToLight(const Light& light, const Vec3f& worldPos) {
        return light.type == LightType::Directional
            ? light.direction.normalize()
            : (light.position - worldPos).normalize();
    }
}

PhongShader::PhongShader(const TGAImage &diffuseMap,
                         const TGAImage &normalMap,
                         const TGAImage &specularMap,
                         const Uniforms &uniforms,
                         const Material &material,
                         const ShadingMode shadingMode,
                         const bool useAlphaTest,
                         const bool useBlending,
                         const bool useDiffuse,
                         const bool useNormalMap,
                         const bool useSpecularMap,
                         const bool useVertexNormalDrawing,
                         const bool useFaceNormalDrawing,
                         const bool useBBoxDrawing,
                         const bool fillColor,
                         const bool useWireframe)
    : diffuseMap(diffuseMap), normalMap(normalMap), specularMap(specularMap), material(material),
      shadingMode(shadingMode),
      useAlphaTest(useAlphaTest), useBlending(useBlending), useDiffuse(useDiffuse), useNormalMap(useNormalMap),
      useSpecularMap(useSpecularMap), fillColor(fillColor), useWireframe(useWireframe),
      useVertexNormalDrawing(useVertexNormalDrawing), useFaceNormalDrawing(useFaceNormalDrawing),
      useBBoxDrawing(useBBoxDrawing)
{
    this->uniforms = uniforms;
}

Varyings PhongShader::vertex(const Vec3f &localPos,
                             const Vec3f &normal,
                             const Vec2f &uv,
                             const Vec3f &tangent,
                             const Vec3f &bitangent)
{
    Varyings out;

    Vec4f clip = uniforms.projection * uniforms.modelView * Vec4f(localPos);
    out.invW = 1.0f / clip.w();

    const Vec4f ndc = clip * out.invW;
    const Vec4f screen = uniforms.viewport * ndc;

    out.screenPos = Vec3f(screen.x(), screen.y(), screen.z());

    out.uv = uv * out.invW;
    Vec4f world = uniforms.model * Vec4f(localPos);
    const Vec3f trueWorldPos = Vec3f(world.x(), world.y(), world.z());
    out.worldPos = trueWorldPos * out.invW;

    const Vec3f trueNormal = (uniforms.normalMatrix * normal).normalize();
    out.normal = trueNormal * out.invW;
    out.tangent = (uniforms.normalMatrix * tangent).normalize() * out.invW;
    out.bitangent = (uniforms.normalMatrix * bitangent).normalize() * out.invW;

    // Gouraud shading computes lighting once per vertex, right here, then
    // lets the rasterizer interpolate the resulting color same as any other
    // varying. Phong and Flat both skip this and light per pixel instead
    // (see fragment()), so this stays zero and unused for them.
    if (shadingMode == ShadingMode::Gouraud) {
        const float vertexShadow = calculateShadowFactor(trueWorldPos, trueNormal);
        Vec3f vertexDiffuse, vertexSpecular;
        calculateLighting(trueNormal, trueWorldPos, uv, vertexShadow, vertexDiffuse, vertexSpecular);
        out.vertexDiffuse = vertexDiffuse * out.invW;
        out.vertexSpecular = vertexSpecular * out.invW;
    }

    return out;
}


bool PhongShader::fragment(Varyings &varyings, TGAColor &color)
{
    if (useWireframe) {
        constexpr float thickness = 0.05f;

        if (varyings.barycentric.x() < thickness ||
            varyings.barycentric.y() < thickness ||
            varyings.barycentric.z() < thickness)
        {
            color = {0, 255, 0, 255};
            return false;
        }
    }

    if (fillColor == false) return true;

    const float w = 1.0f / varyings.invW;
    const Vec2f uv = varyings.uv * w;
    const Vec3f worldPos = varyings.worldPos * w;

    Vec3f N = varyings.normal.normalize();

    varyings.normalForBuffer = N;
    varyings.distanceForBuffer = (worldPos - uniforms.cameraPos).length();

    Vec3f diffuseLight, specularLight;
    if (shadingMode == ShadingMode::Gouraud) {
        diffuseLight = varyings.vertexDiffuse * w;
        specularLight = varyings.vertexSpecular * w;
    } else {
        const float shadowFactor = calculateShadowFactor(worldPos, N);

        Vec3f shadingNormal = N;
        if (useNormalMap && normalMap.width() > 0) {
            const Vec3f T = varyings.tangent.normalize();
            const Vec3f B = varyings.bitangent.normalize();
            shadingNormal = calculateNormal(uv, T, B, N);
        }

        calculateLighting(shadingNormal, worldPos, uv, shadowFactor, diffuseLight, specularLight);
    }

    TGAColor texColor;
    if (useDiffuse) {
        texColor = diffuseMap.get(
            static_cast<int>(uv.x() * diffuseMap.width()),
            static_cast<int>(uv.y() * diffuseMap.height())
        );
    } else {
        texColor = {255, 255, 255, 255};
    }

    // Non-uniform material UV gradient blend
    const Vec3f effectiveDiffuseColor = material.useNonUniformMaterial
        ? material.diffuseColor + (material.diffuseColorSecondary - material.diffuseColor) * uv.x()
        : material.diffuseColor;

    // Lighting combination (TGAColor is stored BGR -> channels: 2/1/0 = R/G/B)
    for (int c = 0; c < 3; ++c) {
        const float texChannel = texColor[2 - c] / GraphicsUtils::MAX_COLOR_F;
        const float ambientTerm = uniforms.ambientLight[c] * effectiveDiffuseColor[c] * texChannel;
        const float diffuseTerm = diffuseLight[c] * effectiveDiffuseColor[c] * texChannel;
        const float specularTerm = specularLight[c] * material.specularColor[c];
        const float emissiveTerm = material.emissive[c];

        const float total = ambientTerm + diffuseTerm + specularTerm + emissiveTerm;
        color[2 - c] = static_cast<unsigned char>(std::min(255.0f, total * GraphicsUtils::MAX_COLOR_F));
    }

    const bool diffuseHasAlpha = useDiffuse && texColor.bytespp == TGAImage::RGBA;
    const float texAlpha = diffuseHasAlpha ? texColor[3] / GraphicsUtils::MAX_COLOR_F : 1.0f;
    const float alpha = useBlending
        ? std::max(0.0f, std::min(1.0f, material.opacity * texAlpha))
        : 1.0f;
    color[3] = static_cast<unsigned char>(alpha * GraphicsUtils::MAX_COLOR_F);

    // Distance-based fog applied last
    if (uniforms.useFog) {
        const float distance = (worldPos - uniforms.cameraPos).length();
        const float range = uniforms.fogEnd - uniforms.fogStart;
        const float fogFactor = range > 0.0f
            ? std::max(0.0f, std::min(1.0f, (uniforms.fogEnd - distance) / range))
            : 1.0f;

        for (int c = 0; c < 3; ++c) {
            const float litValue = color[2 - c] / GraphicsUtils::MAX_COLOR_F;
            const float fogged = uniforms.fogColor[c] * (1.0f - fogFactor) + litValue * fogFactor;
            color[2 - c] = static_cast<unsigned char>(std::min(255.0f, fogged * GraphicsUtils::MAX_COLOR_F));
        }
    }

    return useAlphaTest ? texColor[3] < alphaTestLimit : false;
}

float PhongShader::calculateShadowFactor(const Vec3f& worldPos, const Vec3f& normal) const
{
    if (!uniforms.shadowMap || !uniforms.lights || uniforms.lights->empty()) return 1.0f;

    Vec4f lightClip = uniforms.lightProjView * Vec4f(worldPos);
    Vec3f lightNDC = Vec3f(lightClip.x(), lightClip.y(), lightClip.z()) / lightClip.w();

    const float scX = (lightNDC.x() + 1.0f) * 0.5f * uniforms.shadowWidth;
    const float scY = (lightNDC.y() + 1.0f) * 0.5f * uniforms.shadowHeight;
    const float currentDepth = (lightNDC.z() + 1.0f) * 0.5f;

    // The shadow map is only built from light 0, see Renderer.cpp, so the
    // slope-scaled bias below has to match that same light's direction.
    const Vec3f L = directionToLight(uniforms.lights->front(), worldPos);
    const float dotNL = std::max(0.0f, dotProduct(normal, L));
    const float bias = std::max(minBias, maxBias * (1.0f - dotNL));

    float shadowSum = 0.0f;
    int sampleCount = 0;

    for (int yOffset = -1; yOffset <= 1; yOffset++) {
        for (int xOffset = -1; xOffset <= 1; xOffset++) {
            const int sampleX = static_cast<int>(scX) + xOffset;
            const int sampleY = static_cast<int>(scY) + yOffset;

            if (sampleX >= 0 && sampleX < uniforms.shadowWidth &&
                sampleY >= 0 && sampleY < uniforms.shadowHeight) {
                const int idx = sampleX + sampleY * uniforms.shadowWidth;
                const float closestDepth = (*uniforms.shadowMap)[idx];

                if (currentDepth < closestDepth - bias) {
                    // Occluded: contribution scales with the caster's opacity (1 = fully dark, 0 = no shadow).
                    const float casterOpacity = uniforms.shadowCasterOpacity
                        ? (*uniforms.shadowCasterOpacity)[idx]
                        : 1.0f;
                    shadowSum += 1.0f - casterOpacity;
                } else {
                    shadowSum += 1.0f;
                }
                sampleCount++;
            }
        }
    }
    return sampleCount > 0 ? shadowSum / static_cast<float>(sampleCount) : 1.0f;
}

Vec3f PhongShader::calculateNormal(const Vec2f& uv,
                                   const Vec3f& T,
                                   const Vec3f& B,
                                   const Vec3f& N) const
{

    TGAColor nmC = normalMap.get(
        static_cast<int>(uv.x() * normalMap.width()),
        static_cast<int>(uv.y() * normalMap.height())
    );
    Vec3f mapNormal(
        (static_cast<float>(nmC[2]) / GraphicsUtils::MAX_COLOR_F) * 2.0f - 1.0f,
        (static_cast<float>(nmC[1]) / GraphicsUtils::MAX_COLOR_F) * 2.0f - 1.0f,
        (static_cast<float>(nmC[0]) / GraphicsUtils::MAX_COLOR_F) * 2.0f - 1.0f
    );
    return (T * mapNormal.x() + B * mapNormal.y() + N * mapNormal.z()).normalize();
}

void PhongShader::calculateLighting(const Vec3f& normal,
                                    const Vec3f& worldPos,
                                    const Vec2f& uv,
                                    const float shadowFactor,
                                    Vec3f& outDiffuse,
                                    Vec3f& outSpecular) const
{
    outDiffuse = {0, 0, 0};
    outSpecular = {0, 0, 0};

    if (!uniforms.lights) return;

    const Vec3f V = (uniforms.cameraPos - worldPos).normalize();

    TGAColor specData;
    if (useSpecularMap) {
        specData = specularMap.get(
            static_cast<int>(uv.x() * specularMap.width()),
            static_cast<int>(uv.y() * specularMap.height())
        );
    }

    for (size_t i = 0; i < uniforms.lights->size(); ++i) {
        const Light& light = (*uniforms.lights)[i];

        // Only light 0 has a shadow map built for it,
        // so every other light ignores occluders.
        const float lightShadow = (i == 0) ? shadowFactor : 1.0f;

        const Vec3f L = directionToLight(light, worldPos);
        const float dotNL = dotProduct(normal, L);
        const float diffuseAmount = std::max(0.0f, dotNL) * lightShadow * light.intensity;

        for (int c = 0; c < 3; ++c) {
            outDiffuse[c] += diffuseAmount * light.color[c];
        }

        if (useSpecularMap) {
            constexpr float lightFormulaPower = 10.0f;

            Vec3f R = (normal * (2.0f * dotNL)) - L;
            R = R.normalize();
            // specData[0] is Blue, but since specular maps are grayscale all channels match
            const float specAmount = std::pow(std::max(0.0f, dotProduct(R, V)), lightFormulaPower)
                                      * (specData[0] / GraphicsUtils::MAX_COLOR_F) * lightShadow * light.intensity;

            for (int c = 0; c < 3; ++c) {
                outSpecular[c] += specAmount * light.color[c];
            }
        }
    }
}
#include "PhongShader.h"

PhongShader::PhongShader(const TGAImage &diffuseMap,
                         const TGAImage &normalMap,
                         const TGAImage &specularMap,
                         const Uniforms &uniforms,
                         bool useAlphaTest)
    : diffuseMap(diffuseMap), normalMap(normalMap), specularMap(specularMap), useAlphaTest(useAlphaTest) {
    this->uniforms = uniforms;
}

Varyings PhongShader::vertex(const Vec3f &localPos,
                             const Vec3f &normal,
                             const Vec2f &uv,
                             const Vec3f &tangent,
                             const Vec3f &bitangent) {
    Varyings out;

    Vec4f clip = uniforms.projection * uniforms.modelView * Vec4f(localPos);
    out.invW = 1.0f / clip.w();

    const Vec4f ndc = clip * out.invW;
    const Vec4f screen = uniforms.viewport * ndc;

    out.screenPos = Vec3f(screen.x(), screen.y(), screen.z());

    out.uv = uv * out.invW;
    Vec4f world = uniforms.model * Vec4f(localPos);
    out.worldPos = Vec3f(world.x(), world.y(), world.z()) * out.invW;

    out.normal = (uniforms.normalMatrix * normal).normalize() * out.invW;
    out.tangent = (uniforms.normalMatrix * tangent).normalize() * out.invW;
    out.bitangent = (uniforms.normalMatrix * bitangent).normalize() * out.invW;

    return out;
}


bool PhongShader::fragment(const Varyings &varyings, TGAColor &color) {
    constexpr int alphaTestLimit = 200;
    const float w = 1.0f / varyings.invW;
    const Vec2f uv = varyings.uv * w;
    const Vec3f worldPos = varyings.worldPos * w;
    const Vec3f N = varyings.normal.normalize();
    const Vec3f T = varyings.tangent.normalize();
    const Vec3f B = varyings.bitangent.normalize();

    float shadowFactor = calculateShadowFactor(worldPos);
    Vec3f finalNormal = calculateNormal(uv, T, B, N);

    this->outNormal = finalNormal;

    float diffuseIntensity, specIntensity;
    calculateLighting(finalNormal, worldPos, uv, shadowFactor, diffuseIntensity, specIntensity);

    TGAColor texColor = diffuseMap.get(
        static_cast<int>(uv.x() * diffuseMap.width()),
        static_cast<int>(uv.y() * diffuseMap.height())
    );

    constexpr float ambient = 0.3f;
    const float totalIntensity = ambient + diffuseIntensity + specIntensity;

    for (int i = 0; i < 3; i++) {
        color[i] = (unsigned char)std::min(255.0f, texColor[i] * totalIntensity);
    }

    return useAlphaTest ? texColor[3] < alphaTestLimit : false;
}

float PhongShader::calculateShadowFactor(const Vec3f& worldPos) const {
    if (!uniforms.shadowMap) return 1.0f;

    Vec4f lightClip = uniforms.lightProjView * Vec4f(worldPos);
    Vec3f lightNDC = Vec3f(lightClip.x(), lightClip.y(), lightClip.z()) / lightClip.w();

    const float scX = (lightNDC.x() + 1.0f) * 0.5f * uniforms.shadowWidth;
    const float scY = (lightNDC.y() + 1.0f) * 0.5f * uniforms.shadowHeight;
    const float currentDepth = (lightNDC.z() + 1.0f) * 0.5f;
    constexpr float bias = 0.005f;

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

                shadowSum += (currentDepth < closestDepth - bias) ? 0.0f : 1.0f;
                sampleCount++;
            }
        }
    }
    return sampleCount > 0 ? shadowSum / static_cast<float>(sampleCount) : 1.0f;
}

Vec3f PhongShader::calculateNormal(const Vec2f& uv, const Vec3f& T, const Vec3f& B, const Vec3f& N) const {

    TGAColor nmC = normalMap.get(
        static_cast<int>(uv.x() * normalMap.width()),
        static_cast<int>(uv.y() * normalMap.height())
    );
    Vec3f mapNormal(
        (static_cast<float>(nmC[2]) / 255.0f) * 2.0f - 1.0f,
        (static_cast<float>(nmC[1]) / 255.0f) * 2.0f - 1.0f,
        (static_cast<float>(nmC[0]) / 255.0f) * 2.0f - 1.0f
    );
    return (T * mapNormal.x() + B * mapNormal.y() + N * mapNormal.z()).normalize();
}

void PhongShader::calculateLighting(const Vec3f& normal,
                                    const Vec3f& worldPos,
                                    const Vec2f& uv,
                                    float shadowFactor,
                                    float& outDiffuse,
                                    float& outSpec) const {

    const Vec3f L = uniforms.lightDir.normalize();
    const Vec3f V = (uniforms.cameraPos - worldPos).normalize();

    const float dotNL = dotProduct(normal, L);
    outDiffuse = std::max(0.0f, dotNL) * shadowFactor;

    Vec3f R = (normal * (2.0f * dotNL)) - L;
    R = R.normalize();

    TGAColor specData = specularMap.get(
        static_cast<int>(uv.x() * specularMap.width()),
        static_cast<int>(uv.y() * specularMap.height())
    );
    outSpec = std::pow(std::max(0.0f, dotProduct(R, V)), 10.0f) * (specData[0] / 255.0f) * shadowFactor;
}
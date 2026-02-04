#ifndef RENDERER_PHONGSHADER_H
#define RENDERER_PHONGSHADER_H

#include "../Core/IShader.h"

class PhongShader: public IShader {
public:
    PhongShader(const TGAImage& diffuseMap, const Uniforms& uniforms)
        : diffuseMap(diffuseMap) {
        this->uniforms = uniforms;
    }

    Varyings vertex(const Vec3f& localPos, const Vec3f& normal, const Vec2f& uv) override {
        Varyings out;

        Vec4f clip = uniforms.projection * uniforms.modelView * Vec4f(localPos);
        out.invW = 1.0f / clip.w();

        const Vec4f ndc = clip * out.invW;
        const Vec4f screen = uniforms.viewport * ndc;

        out.screenPos = Vec3f(screen.x(), screen.y(), screen.z());

        out.uv = uv * out.invW;
        Vec4f world = uniforms.model * Vec4f(localPos);
        out.worldPos = Vec3f(world.x(), world.y(), world.z()) * out.invW;

        const Vec3f worldNormal = uniforms.normalMatrix * normal;
        out.normal = worldNormal.normalize() * out.invW;

        return out;
    }


    bool fragment(const Varyings& varyings, TGAColor &color) override  {
        const float w = 1.0f / varyings.invW;
        const Vec2f uv = varyings.uv * w;
        const Vec3f worldPos = varyings.worldPos * w;

        const Vec3f N = (varyings.normal * w).normalize();
        const Vec3f L = uniforms.lightDir.normalize();

        const Vec3f V = (uniforms.cameraPos - worldPos).normalize();
        const Vec3f H = (L + V).normalize();

        constexpr float ambient = 0.4f;
        const float diffuse = std::max(0.0f, dotProduct(N, L));
        const float specular = std::pow(std::max(0.0f, dotProduct(N, H)), 64.0f);

        const float totalIntensity = ambient + diffuse + 0.6f * specular;

        /*
        const unsigned char g = (unsigned char)(std::min(1.f, totalIntensity) * 255);
        color = {g, g, g, 255};
        */

        TGAColor texColor = diffuseMap.get(
                static_cast<int>(uv.x() * diffuseMap.width()),
                static_cast<int>(uv.y() * diffuseMap.height())
                );

        for (int i = 0; i < 3; i++) {
            color[i] = (unsigned char)std::min(255.0f, texColor[i] * totalIntensity);
        }
        color[3] = 255.0f;

        return false;
    }

private:
    const TGAImage& diffuseMap;
};

#endif //RENDERER_PHONGSHADER_H
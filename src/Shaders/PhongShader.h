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
        Vec4f ndc = clip / clip.w();
        Vec4f screen = uniforms.viewport * ndc;

        out.screenPos = Vec3f(screen.x(), screen.y(), screen.z());

        out.uv = uv;
        out.worldPos = localPos;

        Vec4f rotNormal = uniforms.modelView * Vec4f(normal.x(), normal.y(), normal.z(), 0.0f);
        out.normal = Vec3f(rotNormal.x(), rotNormal.y(), rotNormal.z()).normalize();

        return out;
    }


    bool fragment(const Varyings& varyings, TGAColor &color) override  {
        Vec3f normal = varyings.normal.normalize();
        Vec2f uv = varyings.uv;

        TGAColor texColor = diffuseMap.get(uv.x() * diffuseMap.width(), uv.y() * diffuseMap.height());

        float intensity = std::max(0.0f, dotProduct(normal, uniforms.lightDir));
        color = texColor;

        for (int i = 0 ; i < 3 ; i++) {
            color[i] = static_cast<unsigned char>(color[i] * intensity);
        }

        return false;
    }

private:
    const TGAImage& diffuseMap;
};

#endif //RENDERER_PHONGSHADER_H
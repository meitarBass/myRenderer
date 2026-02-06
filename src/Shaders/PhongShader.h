#ifndef RENDERER_PHONGSHADER_H
#define RENDERER_PHONGSHADER_H

#include "../Core/IShader.h"

class PhongShader: public IShader {
public:
    PhongShader(const TGAImage& diffuseMap, const TGAImage& normalMap, const TGAImage& specularMap, const Uniforms& uniforms, const bool useAlphaTest)
        : diffuseMap(diffuseMap), normalMap(normalMap), specularMap(specularMap), useAlphaTest(useAlphaTest) {
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

        out.uv = uv * out.invW;
        Vec4f world = uniforms.model * Vec4f(localPos);
        out.worldPos = Vec3f(world.x(), world.y(), world.z()) * out.invW;

        out.normal = (uniforms.normalMatrix * normal).normalize() * out.invW;
        out.tangent = (uniforms.normalMatrix * tangent).normalize() * out.invW;
        out.bitangent = (uniforms.normalMatrix * bitangent).normalize() * out.invW;

        return out;
    }


   bool fragment(const Varyings& varyings, TGAColor &color) override  {
    const float w = 1.0f / varyings.invW;
    const Vec2f uv = varyings.uv * w;
    const Vec3f worldPos = varyings.worldPos * w;

    // --- חישוב צל (Shadow Calculation) ---
        float shadowFactor = 1.0f;
        if (uniforms.shadowMap) {
            // 1. הטלה למרחב האור (בלי Viewport עדיין)
            Vec4f lightClip = uniforms.lightProjView * Vec4f(worldPos);

            // 2. פרספקטיבה ידנית
            Vec3f lightNDC = Vec3f(lightClip.x(), lightClip.y(), lightClip.z()) / lightClip.w();

            // 3. המרה ידנית מ-NDC (-1..1) לקואורדינטות טקסטורה (0..W, 0..H)
            // הסבר: (NDC + 1) / 2 נותן טווח 0..1. ואז מכפילים ברוחב.
            float scX = (lightNDC.x() + 1.0f) * 0.5f * uniforms.shadowWidth;
            float scY = (lightNDC.y() + 1.0f) * 0.5f * uniforms.shadowHeight;
            // את ה-Z אנחנו גם צריכים לנרמל לסקאלה של ה-ZBuffer שלך (תלוי איך Viewport מממש את זה)
            // ה-Viewport שלך עושה: (z + 1) * 255/2. בואי נחקה את זה:
            float currentDepth = (lightNDC.z() + 1.0f) * 0.5f * 255.0f; // הנחה שהעומק הוא 0-255

            int x = static_cast<int>(scX);
            int y = static_cast<int>(scY);

            if (x >= 0 && x < uniforms.shadowWidth && y >= 0 && y < uniforms.shadowHeight) {
                int idx = x + y * uniforms.shadowWidth;
                float closestDepth = (*uniforms.shadowMap)[idx];

                // תיקון הלוגיקה!
                // אצלך: מספר גדול = קרוב. מספר קטן = רחוק.
                // אנחנו בצל אם אנחנו "מאחורי" מה ששמור במפה.
                // כלומר: currentDepth (שלי) < closestDepth (המחסום)

                float bias = 0.5f;
                if (currentDepth < closestDepth - bias) {
                    shadowFactor = 0.0f; // אני רחוק יותר -> אני בצל
                }
            }
        }
    // -------------------------------------

    const Vec3f N = varyings.normal.normalize();
    const Vec3f T = varyings.tangent.normalize();
    const Vec3f B = varyings.bitangent.normalize();

    // ... (חישוב נורמלים - אותו דבר כמו קודם) ...
    TGAColor nmC = normalMap.get(
        static_cast<int>(uv.x() * normalMap.width()),
        static_cast<int>(uv.y() * normalMap.height())
    );
    const Vec3f mapNormal (
        (static_cast<float>(nmC[2]) / 255.0f) * 2.0f - 1.0f,
        (static_cast<float>(nmC[1]) / 255.0f) * 2.0f - 1.0f,
        (static_cast<float>(nmC[0]) / 255.0f) * 2.0f - 1.0f
    );
    const Vec3f finalNormal = (T * mapNormal.x() + B * mapNormal.y() + N * mapNormal.z()).normalize();


    // חישובי תאורה
    const Vec3f L = uniforms.lightDir.normalize();
    const Vec3f V = (uniforms.cameraPos - worldPos).normalize();

    const float dotNL = dotProduct(finalNormal, L);

    // כאן השינוי הגדול: הצל משפיע על ה-Diffuse וה-Specular
    const float diffuse = std::max(0.0f, dotNL) * shadowFactor;

    Vec3f R = (finalNormal * (2.0f * dotNL)) - L;
    R = R.normalize();

    TGAColor specData = specularMap.get(
        static_cast<int>(uv.x() * specularMap.width()),
        static_cast<int>(uv.y() * specularMap.height())
    );

    // גם ה-Specular מושפע מהצל
    const float spec = std::pow(std::max(0.0f, dotProduct(R, V)), 10.0f) * (specData[0] / 255.0f) * shadowFactor;

    constexpr float ambient = 0.2f; // הורדתי קצת כדי שהצל יהיה דרמטי

    // ה-Ambient לא מושפע מהצל (אחרת הצל יהיה שחור מוחלט ושטוח)
    const float totalIntensity = ambient + diffuse + spec;

    TGAColor texColor = diffuseMap.get(
            static_cast<int>(uv.x() * diffuseMap.width()),
            static_cast<int>(uv.y() * diffuseMap.height())
    );

    for (int i = 0; i < 3; i++) {
        color[i] = (unsigned char)std::min(255.0f, texColor[i] * totalIntensity);
    }

    return useAlphaTest ? texColor[3] < 200 : false;
}

private:
    const TGAImage& diffuseMap;
    const TGAImage& normalMap;
    const TGAImage& specularMap;
    const bool useAlphaTest;
};

#endif //RENDERER_PHONGSHADER_H
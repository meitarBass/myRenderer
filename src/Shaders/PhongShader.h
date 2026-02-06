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

        // --- חישוב צל משודרג (PCF) ---
        float shadowFactor = 1.0f; // ברירת מחדל: מואר
        if (uniforms.shadowMap) {
            Vec4f lightClip = uniforms.lightProjView * Vec4f(worldPos);
            Vec3f lightNDC = Vec3f(lightClip.x(), lightClip.y(), lightClip.z()) / lightClip.w();

            // המרה לקואורדינטות מפה
            float scX = (lightNDC.x() + 1.0f) * 0.5f * uniforms.shadowWidth;
            float scY = (lightNDC.y() + 1.0f) * 0.5f * uniforms.shadowHeight;
            float currentDepth = (lightNDC.z() + 1.0f) * 0.5f * 255.0f;

            float bias = 0.05f; // הערך שמצאת שעובד טוב
            float shadowSum = 0.0f;
            int sampleCount = 0;

            // לולאת PCF: דוגמים 3x3 סביב הפיקסל
            for (int yOffset = -1; yOffset <= 1; yOffset++) {
                for (int xOffset = -1; xOffset <= 1; xOffset++) {

                    int sampleX = static_cast<int>(scX) + xOffset;
                    int sampleY = static_cast<int>(scY) + yOffset;

                    // בדיקת גבולות (שלא נחרוג מהמערך ונקרוס)
                    if (sampleX >= 0 && sampleX < uniforms.shadowWidth &&
                        sampleY >= 0 && sampleY < uniforms.shadowHeight) {

                        int idx = sampleX + sampleY * uniforms.shadowWidth;
                        float closestDepth = (*uniforms.shadowMap)[idx];

                        // אם הדגימה הזו בצל, נוסיף 0. אם היא באור, נוסיף 1
                        if (currentDepth < closestDepth - bias) {
                            shadowSum += 0.0f; // בצל
                        } else {
                            shadowSum += 1.0f; // באור
                        }
                        sampleCount++;
                        }
                }
            }

            // הממוצע הוא עוצמת הצל הסופית
            if (sampleCount > 0) {
                shadowFactor = shadowSum / static_cast<float>(sampleCount);
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
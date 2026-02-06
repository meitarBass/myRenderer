#ifndef RENDERER_VISUALTESTS_H
#define RENDERER_VISUALTESTS_H

#include <vector>
#include "../src/Core/ModelInstance.h"
#include "../src/Math/Vec.h"
#include "../src/Math/Matrix.h"
#include "../src/Core/IShader.h"

class VisualTests {
public:
    static void runVisualSuite(const std::vector<ModelInstance>& scene, int width, int height);

private:
    struct RenderBuffers {
        TGAImage framebuffer;
        std::vector<float> zbuffer;
        std::vector<Vec3f> normalBuffer;
    };

    static std::vector<float> runShadowPass(const std::vector<ModelInstance>& scene,
                                            const Matrix4f4& lightMVP, int w, int h);

    static RenderBuffers runColorPass(const std::vector<ModelInstance>& scene,
                                      int width, int height,
                                      const Uniforms& baseUniforms,
                                      const std::vector<float>& shadowMap,
                                      const Matrix4f4& lightProjView);

    static void applySSAO(TGAImage& framebuffer,
                          const std::vector<float>& zbuffer,
                          const std::vector<Vec3f>& normals);

    static std::vector<float> computeSSAO(const std::vector<float>& zbuffer,
                                          const std::vector<Vec3f>& normalBuffer,
                                          int width, int height);

    static inline float randf() {
        return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }
};


#endif //RENDERER_VISUALTESTS_H
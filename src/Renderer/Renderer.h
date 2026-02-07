#ifndef RENDERER_RENDERER_H
#define RENDERER_RENDERER_H

#include "Scene.h"
#include "../Core/IShader.h"
#include "../IO/tgaimage.h"
#include "../Core/Rasterizer.h"
#include <vector>
#include <limits>
#include <cstdlib>

struct RenderBuffers {
    TGAImage framebuffer;
    std::vector<float> zbuffer;
    std::vector<Vec3f> normalBuffer;

    RenderBuffers(int w, int h)
        : framebuffer(w, h, TGAImage::RGB),
          zbuffer(w * h, -std::numeric_limits<float>::max()),
          normalBuffer(w * h, Vec3f(0, 0, 0))
    {}
};

class Renderer {
public:
    void render(const Scene& scene, RenderBuffers& target);

private:
    void runShadowPass(const Scene& scene, std::vector<float>& shadowMap, const Matrix4f4& lightProjView);

    void runColorPass(const Scene& scene, RenderBuffers& target,
                      const std::vector<float>& shadowMap, const Matrix4f4& lightProjView);

    void applySSAO(RenderBuffers& target);

    std::vector<float> computeSSAO(const std::vector<float>& zbuffer,
                                   const std::vector<Vec3f>& normalBuffer,
                                   int width, int height);

    static inline float randf() {
        return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }
};


#endif //RENDERER_RENDERER_H
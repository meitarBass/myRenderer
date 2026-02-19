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
    std::vector<float> shadowMap;

    int width, height;
    int shadowW, shadowH;

    RenderBuffers(const RenderBuffers&) = delete;
    RenderBuffers& operator=(const RenderBuffers&) = delete;
    RenderBuffers(RenderBuffers&&) = delete;
    RenderBuffers& operator=(RenderBuffers&&) = delete;

    RenderBuffers(const int w,  const int h, const int sw, const int sh)
        : framebuffer(w, h, TGAImage::RGB),
          zbuffer(w * h, -std::numeric_limits<float>::max()),
          normalBuffer(w * h, Vec3f(0, 0, 0)),
          shadowMap(sw * sh, -std::numeric_limits<float>::max()),
          width(w), height(h), shadowW(sw), shadowH(sh)
    {}

    void reset() {
        std::fill(framebuffer.buffer(), framebuffer.buffer() + (width * height * 3), 0);
        std::ranges::fill(zbuffer, -std::numeric_limits<float>::max());
        std::ranges::fill(normalBuffer, Vec3f(0, 0, 0));
        std::ranges::fill(shadowMap, -std::numeric_limits<float>::max());
    }
};

class Renderer {
public:
    void render(const Scene& scene, RenderBuffers& target);

private:
    void runShadowPass(const Scene& scene,
                              RenderBuffers &target,
                              const Matrix4f4& lightProjView);

    void runColorPass(const Scene& scene, RenderBuffers& target, const Matrix4f4& lightProjView);

    void applySSAO(RenderBuffers& target);

    static float randf() {
        return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }
};


#endif //RENDERER_RENDERER_H
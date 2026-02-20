#ifndef RENDERER_RENDERER_H
#define RENDERER_RENDERER_H

#include "Scene.h"
#include "../Core/IShader.h"
#include "../IO/tgaimage.h"
#include "../Core/Rasterizer.h"
#include <vector>
#include <limits>
#include <cstdlib>


/**
 * Contains all buffers relevant to the rendering pipeline.
 * Used also in order to avoid memory allocation for each frame,
 * hence the reset function.
 */
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

    void reset()
    {
        std::fill(framebuffer.buffer(), framebuffer.buffer() + (width * height * 3), 0);
        std::ranges::fill(zbuffer, -std::numeric_limits<float>::max());
        std::ranges::fill(normalBuffer, Vec3f(0, 0, 0));
        std::ranges::fill(shadowMap, -std::numeric_limits<float>::max());
    }
};

class Renderer {
public:
    /**
     * @brief Renders the scene using the given buffers from target.
     *        The logic is - shadow pass -> color pass -> SSAO
     * @param scene - Contains the model, camera and lighting relevant for the scene.
     * @param target - Contains the z-buffer, framebuffer, normal map and shadow map.
     */
    static void render(const Scene& scene, RenderBuffers& target);

private:
    /**
     * The function checks which pixels are hidden.
     * 'Hidden pixels' are pixels hidden from the light source - assuming a single light source.
     * The result is saved using the shadow map buffer and then used during
     * color pass.
     * This function does not affect the framebuffer.
     * */
    static void runShadowPass(const Scene& scene,
                              RenderBuffers &target,
                              const Matrix4f4& lightProjView);

    /** For each pixel, draws the correct color based on lighting, shadow, model texture file,
       and occlusions. */
    static void runColorPass(const Scene& scene,
                             RenderBuffers& target,
                             const Matrix4f4& lightProjView);

    // Adds the SSAO effect to the scene.
    static void applySSAO(RenderBuffers& target);


    static void initSSAOSamples(std::vector<Vec2f>& kernel, std::vector<Vec2f>& noise);

    static float computePixelOcclusion(int x, int y,
                                       int width, int height,
                                       const std::vector<float>& zbuffer,
                                       const std::vector<Vec2f>& kernel,
                                       const std::vector<Vec2f>& noise);

    static float randf() {
        return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }

    static constexpr float SSAO_BACKGROUND_THRESHOLD = 100.0f;
    static constexpr float SSAO_MAX_OCCLUSION_DISTANCE = 2.0f;
    static constexpr float LIGHT_PROJECTION_SIZE = 3.0f;

    static constexpr float SSAO_SAMPLE_RADIUS = 25.0f;
    static constexpr float SSAO_BIAS = 0.05f;
    static constexpr float SSAO_STRENGTH = 0.3f;
    static constexpr int SSAO_RANDOM_PIXEL_SAMPLES = 16;
};


#endif //RENDERER_RENDERER_H
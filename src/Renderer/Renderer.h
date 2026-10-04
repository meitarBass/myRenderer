#ifndef RENDERER_RENDERER_H
#define RENDERER_RENDERER_H

#include "Scene.h"
#include "../Core/IShader.h"
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
    std::vector<unsigned char> colorBuffer;

    std::vector<float> zbuffer;
    std::vector<Vec3f> normalBuffer;
    std::vector<float> distanceBuffer;          // Used for dof.
    std::vector<float> shadowMap;

    std::vector<float> shadowCasterOpacity; // per-texel opacity of the shadow's caster, for shadow fade
    std::vector<float> blendScratchZ; // per-object scratch depth, reset before each blended model's draw

    int width, height;
    int shadowW, shadowH;

    // Frustum-culling stats from the last color pass, for the UI (see
    // Renderer::runColorPass / Application::run). Not touched by reset(),
    // since they describe the frame that was just drawn, not the next one.
    int lastFrameModelCount = 0;
    int lastFrameCulledCount = 0;

    RenderBuffers(const RenderBuffers&) = delete;
    RenderBuffers& operator=(const RenderBuffers&) = delete;
    RenderBuffers(RenderBuffers&&) = delete;
    RenderBuffers& operator=(RenderBuffers&&) = delete;

    RenderBuffers(const int w,  const int h, const int sw, const int sh)
        : colorBuffer(w * h * 3, 0),
          zbuffer(w * h, -std::numeric_limits<float>::max()),
          normalBuffer(w * h, Vec3f(0, 0, 0)),
          distanceBuffer(w * h, std::numeric_limits<float>::max()),
          shadowMap(sw * sh, -std::numeric_limits<float>::max()),
          shadowCasterOpacity(sw * sh, 1.0f),
          blendScratchZ(w * h, -std::numeric_limits<float>::max()),
          width(w), height(h), shadowW(sw), shadowH(sh)
    {}


    void resize(const int newW, const int newH)
    {
        if (width == newW && height == newH) return;

        width = newW;
        height = newH;

        colorBuffer.resize(width * height * 3);
        zbuffer.resize(width * height);
        normalBuffer.resize(width * height);
        distanceBuffer.resize(width * height);
        blendScratchZ.resize(width * height);

        reset();
    }

    void reset()
    {
        std::ranges::fill(colorBuffer.begin(), colorBuffer.end(), 0);
        std::ranges::fill(zbuffer, -std::numeric_limits<float>::max());
        std::ranges::fill(normalBuffer, Vec3f(0, 0, 0));
        std::ranges::fill(distanceBuffer, std::numeric_limits<float>::max());
        std::ranges::fill(shadowMap, -std::numeric_limits<float>::max());
        std::ranges::fill(shadowCasterOpacity, 1.0f);
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

    static void applySkybox(const Scene& scene, RenderBuffers& target);

    // Fakes depth of field by blending each pixel between the sharp image and
    // a fully blurred copy.
    static void applyDepthOfField(const Scene& scene, RenderBuffers& target);

    // Blurs the entire framebuffer with a separable gaussian filter.
    static void applyFullScreenBlur(RenderBuffers& target);

    // Extracts pixels brighter than bloomThreshold, blurs just those, and
    // adds the glow back onto the original image scaled by bloomIntensity.
    static void applyBloom(RenderBuffers& target, float bloomThreshold, float bloomIntensity);

    // Separable gaussian blur of an RGB buffer.
    static void gaussianBlurRGB(std::vector<float>& r, std::vector<float>& g,
                                std::vector<float>& b, int width, int height);

    static bool isAABBVisible(const AABB& worldBox, const Matrix4f4& view, const Matrix4f4& viewProj);


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
    static constexpr float LIGHT_PROJECTION_DISTANCE = 5.0f;

    static constexpr float SSAO_SAMPLE_RADIUS = 25.0f;
    static constexpr float SSAO_BIAS = 0.05f;
    static constexpr float SSAO_STRENGTH = 0.3f;
    static constexpr int SSAO_RANDOM_PIXEL_SAMPLES = 16;

    static constexpr int BLUR_RADIUS = 4;
};


#endif //RENDERER_RENDERER_H
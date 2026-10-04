#include "Renderer.h"
#include "../Shaders/PhongShader.h"
#include "../Shaders/DepthShader.h"
#include "../Utils/ThreadPool.h"
#include <iostream>
#include <thread>
#include <algorithm>

bool Renderer::isAABBVisible(const AABB& worldBox, const Matrix4f4& view, const Matrix4f4& viewProj)
{
    const Vec3f corners[8] = {
        {worldBox.min.x(), worldBox.min.y(), worldBox.min.z()},
        {worldBox.min.x(), worldBox.min.y(), worldBox.max.z()},
        {worldBox.min.x(), worldBox.max.y(), worldBox.min.z()},
        {worldBox.min.x(), worldBox.max.y(), worldBox.max.z()},
        {worldBox.max.x(), worldBox.min.y(), worldBox.min.z()},
        {worldBox.max.x(), worldBox.min.y(), worldBox.max.z()},
        {worldBox.max.x(), worldBox.max.y(), worldBox.min.z()},
        {worldBox.max.x(), worldBox.max.y(), worldBox.max.z()},
    };

    int inFrontCount = 0;
    float minX = std::numeric_limits<float>::max();
    float maxX = std::numeric_limits<float>::lowest();
    float minY = std::numeric_limits<float>::max();
    float maxY = std::numeric_limits<float>::lowest();

    for (const auto& corner : corners) {
        const Vec4f viewPos = view * Vec4f(corner);
        if (viewPos.z() >= -GraphicsUtils::EPSILON) continue; // at or behind the camera
        inFrontCount++;
        const Vec4f clip = viewProj * Vec4f(corner);
        const float ndcX = clip.x() / clip.w();
        const float ndcY = clip.y() / clip.w();
        minX = std::min(minX, ndcX);
        maxX = std::max(maxX, ndcX);
        minY = std::min(minY, ndcY);
        maxY = std::max(maxY, ndcY);
    }

    if (inFrontCount == 0) return false; // the whole box is behind the camera

    // part of the box and can't be trusted to say it's offscreen. Keep it.
    if (inFrontCount < 8) return true;

    return !(maxX < -1.0f || minX > 1.0f || maxY < -1.0f || minY > 1.0f);
}

void Renderer::render(const Scene& scene, RenderBuffers& target)
{
    // Only light 0 casts a shadow, we only build one shadow map.
    // A directional light doesn't have a real position, so a stand-in point
    // is placed a fixed distance out along its direction just so lookat()
    // has somewhere to sit.
    Matrix4f4 lightProjView = Matrix4f4::identity();
    if (!scene.lights.empty()) {
        const Light& shadowLight = scene.lights.front();
        const Vec3f shadowLightPos = shadowLight.type == LightType::Directional
            ? shadowLight.direction.normalize() * LIGHT_PROJECTION_DISTANCE
            : shadowLight.position;

        const Matrix4f4 lightView = Matrix4f4::lookat(shadowLightPos, Vec3f(0.0f, 0.0f, 0.0f), Vec3f(0.0f, 1.0f, 0.0f));
        const float shadowAspect = target.shadowH > 0 ? static_cast<float>(target.shadowW) / static_cast<float>(target.shadowH) : 1.0f;

        const Matrix4f4 lightProj = Matrix4f4::projection(LIGHT_PROJECTION_SIZE, shadowAspect);
        lightProjView = lightProj * lightView;
    }

    // Reset all buffers.
    target.reset();
    // --- STEP 1: Shadow Pass ---
    if (scene.useShadows && !scene.lights.empty()) {
        runShadowPass(scene, target, lightProjView);
    }

    // --- STEP 2: Fill Z-Buffer (Crucial for SSAO) ---
    runColorPass(scene, target, lightProjView);

    if (scene.useSkybox) {
        applySkybox(scene, target);
    }

    // --- STEP 3: Apply SSAO ---
    if (scene.useSSAO) {
        applySSAO(target);
    }

    if (scene.useDepthOfField) {
        applyDepthOfField(scene, target);
    }

    // --- STEP 4: Bloom ---
    if (scene.useBloom) {
        applyBloom(target, scene.bloomThreshold, scene.bloomIntensity);
    }

    // --- STEP 5: Full-screen blur ---
    if (scene.useFullScreenBlur) {
        applyFullScreenBlur(target);
    }
}

void Renderer::runShadowPass(const Scene& scene,
                             RenderBuffers &target,
                             const Matrix4f4& lightProjView)
{

    const Matrix4f4 lightViewport = Matrix4f4::viewport(0, 0, target.shadowW, target.shadowH);

    for (const auto& object : scene.models) {
        Uniforms depthUniforms;
        depthUniforms.projection = Matrix4f4::identity();
        depthUniforms.viewport = lightViewport;

        // Carried through to Varyings by DepthShader::vertex, then read
        // back per shadow-map texel in PhongShader::calculateShadowFactor,
        // Caster's own opacity, so its shadow fades along with it (see calculateShadowFactor).
        depthUniforms.casterOpacity = object.useBlending ? object.material.opacity : 1.0f;

        Matrix4f4 modelMat = object.getModelMatrix();
        depthUniforms.modelView = lightProjView * modelMat;

        DepthShader depthShader(depthUniforms);

        const RenderContext ctx = {
            .model = object.resource->model,
            .zbuffer = target.shadowMap,
            .width = target.shadowW,
            .height = target.shadowH,
            .opacityBuffer = &target.shadowCasterOpacity,
        };
        drawModel(ctx, depthShader);
    }
}

void Renderer::runColorPass(const Scene& scene,
                  RenderBuffers& target,
                  const Matrix4f4& lightProjView)
{

    const Camera &cam = scene.getActiveCamera();
    const Matrix4f4 view = Matrix4f4::lookat(cam.pos, cam.lookAt, cam.up);

    const float aspect = target.height > 0 ? static_cast<float>(target.width) / static_cast<float>(target.height) : 1.0f;

    const Matrix4f4 projection = buildCameraProjection(cam, aspect);
    const Matrix4f4 viewport = Matrix4f4::viewport(0, 0, target.width, target.height);
    const Matrix4f4 viewProj = projection * view;

    target.lastFrameModelCount = static_cast<int>(scene.models.size());
    target.lastFrameCulledCount = 0;

    // True if the model's bounding box is entirely off screen and can be skipped.
    const auto isCulled = [&](const ModelInstance& object) {
        if (scene.useFrustumCulling && !isAABBVisible(object.getWorldAABB(), view, viewProj)) {
            target.lastFrameCulledCount++;
            return true;
        }
        return false;
    };

    const auto drawObject = [&](const ModelInstance& object) {
        Uniforms uniforms;

        uniforms.model = object.getModelMatrix();
        uniforms.modelView = view * uniforms.model;
        uniforms.projection = projection;
        uniforms.viewport = viewport;

        uniforms.lights = &scene.lights;
        uniforms.ambientLight = scene.ambientLight;
        uniforms.useFog = scene.useFog;
        uniforms.fogColor = scene.fogColor;
        uniforms.fogStart = scene.fogStart;
        uniforms.fogEnd = scene.fogEnd;
        uniforms.lightProjView = lightProjView;
        uniforms.shadowMap = scene.useShadows ? &target.shadowMap : nullptr;
        uniforms.shadowCasterOpacity = scene.useShadows ? &target.shadowCasterOpacity : nullptr;
        uniforms.shadowWidth = target.shadowW;
        uniforms.shadowHeight = target.shadowH;

        uniforms.normalMatrix = uniforms.model.inverseTranspose3x3();
        uniforms.cameraPos = cam.pos;

        PhongShader shader(object.resource->diffuse, object.resource->normal, object.resource->specular, uniforms,
                           object.material, object.shadingMode, object.useAlphaTest, object.useBlending, object.useDiffuse,
                           object.useNormalMap, object.useSpecularMap, object.useVertexNormalDrawing, object.useFaceNormalDrawing,
                           object.useBBoxDrawing, object.fillColor, object.useWireframe);

        RenderContext ctx = { object.resource->model, target.zbuffer,
                              &target.colorBuffer, &target.normalBuffer, &target.distanceBuffer,
                              target.width, target.height,
                              &scene, &target.blendScratchZ };
        drawModel(ctx, shader);
    };

    // Opaque objects render first, then blended ones back-to-front by distance to camera
    // -- the standard cheap approximation to correct transparency ordering.
    // Built as a separate list of pointers rather than reordering
    // scene.models itself, since Scene::activeModelIndex and the ImGui
    // model list both index into scene.models directly.
    std::vector<const ModelInstance*> blended;
    for (const auto& object : scene.models) {
        if (isCulled(object)) continue;
        if (object.useBlending) {
            blended.push_back(&object);
        } else {
            drawObject(object);
        }
    }

    std::sort(blended.begin(), blended.end(), [&cam](const ModelInstance* a, const ModelInstance* b) {
        const auto distSqToCamera = [&cam](const ModelInstance* m) {
            const AABB box = m->getWorldAABB();
            const Vec3f center = (box.min + box.max) * 0.5f;
            const Vec3f diff = center - cam.pos;
            return dotProduct(diff, diff);
        };
        return distSqToCamera(a) > distSqToCamera(b); // farthest first
    });

    for (const ModelInstance* object : blended) {
        // Reset per-object so blendScratchZ only resolves this model's own self-overlap.
        std::fill(target.blendScratchZ.begin(), target.blendScratchZ.end(), -std::numeric_limits<float>::max());
        drawObject(*object);
    }
}

void Renderer::applySSAO(RenderBuffers& target)
{
    const int width = target.width;
    const int height = target.height;

    std::uint8_t* rawFB = target.colorBuffer.data();

    static std::vector<Vec2f> kernel;
    static std::vector<Vec2f> noise;

    initSSAOSamples(kernel, noise);

    const unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
    const int rowsPerThread =  static_cast<int>(height / numThreads);

    for (int t = 0; t < numThreads; ++t) {
        ThreadPool::instance().enqueue([&, t]() {
            const int startY = t * rowsPerThread;
            const int endY = (t == numThreads - 1) ? height : (startY + rowsPerThread);
            for (int y = startY; y < endY; y++) {
                for (int x = 0; x < width; x++) {
                    const int idx = x + y * width;
                    const float intensity = computePixelOcclusion(x, y, width, height,
                                                                  target.zbuffer, kernel, noise);

                    if (intensity < 0.0f) continue;

                    const int offset = idx * 3;
                    rawFB[offset]     = static_cast<uint8_t>(rawFB[offset]     * intensity);
                    rawFB[offset + 1] = static_cast<uint8_t>(rawFB[offset + 1] * intensity);
                    rawFB[offset + 2] = static_cast<uint8_t>(rawFB[offset + 2] * intensity);
                }
            }
        });
    }

    ThreadPool::instance().waitFinished();
}

void Renderer::initSSAOSamples(std::vector<Vec2f>& kernel, std::vector<Vec2f>& noise)
{
    if (kernel.empty()) {
        for (int i = 0; i < SSAO_RANDOM_PIXEL_SAMPLES; ++i) {
            Vec2f sample(randf() * 2.0f - 1.0f, randf() * 2.0f - 1.0f);
            sample = sample.normalize() * (0.1f + 0.9f * (float)i / SSAO_RANDOM_PIXEL_SAMPLES);
            kernel.push_back(sample);
        }
        for (int i = 0; i < SSAO_RANDOM_PIXEL_SAMPLES; i++) {
            noise.push_back(Vec2f(randf() * 2.0f - 1.0f, randf() * 2.0f - 1.0f).normalize());
        }
    }
}


void Renderer::applySkybox(const Scene& scene, RenderBuffers& target)
{
    const Camera& cam = scene.getActiveCamera();
    const Matrix4f4 view = Matrix4f4::lookat(cam.pos, cam.lookAt, cam.up);
    const float aspect = target.height > 0 ? static_cast<float>(target.width) / static_cast<float>(target.height) : 1.0f;

    const Matrix4f4 invProjection = buildCameraProjection(cam, aspect).inverse4x4();
    const Matrix4f4 invView = view.inverse4x4();

    const bool isOrtho = cam.projectionType == ProjectionType::Orthographic;
    const bool isSimple = cam.projectionType == ProjectionType::PerspectiveSimple;
    const float copZ = isSimple ? cam.focalLength : 0.0f;
    const Vec3f orthoDirection = (cam.lookAt - cam.pos).normalize();

    const int width = target.width;
    const int height = target.height;
    std::uint8_t* rawFB = target.colorBuffer.data();

    const unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
    const int rowsPerThread = static_cast<int>(height / numThreads);

    // Paints a horizon-to-zenith gradient by view-ray direction into pixels the color pass never touched.
    for (unsigned int t = 0; t < numThreads; ++t) {
        ThreadPool::instance().enqueue([&, t]() {
            const int startY = t * rowsPerThread;
            const int endY = (t == numThreads - 1) ? height : (startY + rowsPerThread);
            for (int y = startY; y < endY; ++y) {
                for (int x = 0; x < width; ++x) {
                    const int idx = x + y * width;
                    if (target.zbuffer[idx] > -std::numeric_limits<float>::max()) continue;

                    Vec3f direction = orthoDirection;
                    if (!isOrtho) {
                        const float xNdc = (2.0f * x) / width - 1.0f;
                        const float yNdc = 1.0f - (2.0f * y) / height;
                        Vec4f rayEye = invProjection * Vec4f(xNdc, yNdc, 1.0f, 1.0f);
                        rayEye = {rayEye.x(), rayEye.y(), -copZ - (isSimple ? 0.0f : 1.0f), 0.0f};
                        const Vec4f rayWorld = invView * rayEye;
                        direction = Vec3f(rayWorld.x(), rayWorld.y(), rayWorld.z()).normalize();
                    }

                    const float blend = std::max(0.0f, std::min(1.0f, direction.y() * 0.5f + 0.5f));
                    const Vec3f sky = scene.skyHorizonColor + (scene.skyZenithColor - scene.skyHorizonColor) * blend;

                    rawFB[idx * 3]     = static_cast<uint8_t>(sky.x() * 255.0f);
                    rawFB[idx * 3 + 1] = static_cast<uint8_t>(sky.y() * 255.0f);
                    rawFB[idx * 3 + 2] = static_cast<uint8_t>(sky.z() * 255.0f);
                }
            }
        });
    }
    ThreadPool::instance().waitFinished();
}

void Renderer::applyDepthOfField(const Scene& scene, RenderBuffers& target)
{
    const int width = target.width;
    const int height = target.height;
    std::uint8_t* rawFB = target.colorBuffer.data();

    std::vector<float> r(width * height), g(width * height), b(width * height);
    for (int i = 0; i < width * height; ++i) {
        r[i] = rawFB[i * 3] / 255.0f;
        g[i] = rawFB[i * 3 + 1] / 255.0f;
        b[i] = rawFB[i * 3 + 2] / 255.0f;
    }

    std::vector<float> blurredR = r, blurredG = g, blurredB = b;
    gaussianBlurRGB(blurredR, blurredG, blurredB, width, height);

    const float safeBlurDistance = std::max(0.001f, scene.dofBlurDistance);

    // Circle of confusion: 0 inside the focus range, ramping to 1 (fully blurred) dofBlurDistance past it.
    for (int i = 0; i < width * height; ++i) {
        const float outOfFocus = std::abs(target.distanceBuffer[i] - scene.focusDistance) - scene.focusRange;
        const float coc = std::max(0.0f, std::min(1.0f, outOfFocus / safeBlurDistance));

        const float outR = r[i] + (blurredR[i] - r[i]) * coc;
        const float outG = g[i] + (blurredG[i] - g[i]) * coc;
        const float outB = b[i] + (blurredB[i] - b[i]) * coc;

        rawFB[i * 3]     = static_cast<uint8_t>(std::min(255.0f, outR * 255.0f));
        rawFB[i * 3 + 1] = static_cast<uint8_t>(std::min(255.0f, outG * 255.0f));
        rawFB[i * 3 + 2] = static_cast<uint8_t>(std::min(255.0f, outB * 255.0f));
    }
}

void Renderer::gaussianBlurRGB(std::vector<float>& r, std::vector<float>& g,
                               std::vector<float>& b, const int width, const int height)
{
    // Weights built once and reused, a gaussian centered on the middle tap,
    // sigma picked so the kernel tapers to ~0 by the edges of BLUR_RADIUS.
    static std::vector<float> weights;
    if (weights.empty()) {
        const float sigma = BLUR_RADIUS / 2.0f;
        float sum = 0.0f;
        for (int i = -BLUR_RADIUS; i <= BLUR_RADIUS; ++i) {
            const float w = std::exp(-(i * i) / (2.0f * sigma * sigma));
            weights.push_back(w);
            sum += w;
        }
        for (auto& w : weights) w /= sum;
    }

    std::vector<float> tmpR(width * height), tmpG(width * height), tmpB(width * height);

    const unsigned int numThreads = std::max(1u, std::thread::hardware_concurrency());
    const int rowsPerThread = static_cast<int>(height / numThreads);

    // Horizontal pass: source -> tmp.
    for (unsigned int t = 0; t < numThreads; ++t) {
        ThreadPool::instance().enqueue([&, t]() {
            const int startY = t * rowsPerThread;
            const int endY = (t == numThreads - 1) ? height : (startY + rowsPerThread);
            for (int y = startY; y < endY; ++y) {
                for (int x = 0; x < width; ++x) {
                    float rSum = 0.0f, gSum = 0.0f, bSum = 0.0f;
                    for (int k = -BLUR_RADIUS; k <= BLUR_RADIUS; ++k) {
                        const int sx = std::min(std::max(x + k, 0), width - 1);
                        const float w = weights[k + BLUR_RADIUS];
                        const int idx = sx + y * width;
                        rSum += r[idx] * w;
                        gSum += g[idx] * w;
                        bSum += b[idx] * w;
                    }
                    const int idx = x + y * width;
                    tmpR[idx] = rSum;
                    tmpG[idx] = gSum;
                    tmpB[idx] = bSum;
                }
            }
        });
    }
    ThreadPool::instance().waitFinished();

    // Vertical pass: tmp -> source (blurred result written back in place).
    for (unsigned int t = 0; t < numThreads; ++t) {
        ThreadPool::instance().enqueue([&, t]() {
            const int startY = t * rowsPerThread;
            const int endY = (t == numThreads - 1) ? height : (startY + rowsPerThread);
            for (int y = startY; y < endY; ++y) {
                for (int x = 0; x < width; ++x) {
                    float rSum = 0.0f, gSum = 0.0f, bSum = 0.0f;
                    for (int k = -BLUR_RADIUS; k <= BLUR_RADIUS; ++k) {
                        const int sy = std::min(std::max(y + k, 0), height - 1);
                        const float w = weights[k + BLUR_RADIUS];
                        const int idx = x + sy * width;
                        rSum += tmpR[idx] * w;
                        gSum += tmpG[idx] * w;
                        bSum += tmpB[idx] * w;
                    }
                    const int idx = x + y * width;
                    r[idx] = rSum;
                    g[idx] = gSum;
                    b[idx] = bSum;
                }
            }
        });
    }
    ThreadPool::instance().waitFinished();
}

// TODO: Remove code duplication, here and applyDepthOfField
void Renderer::applyFullScreenBlur(RenderBuffers& target)
{
    const int width = target.width;
    const int height = target.height;
    std::uint8_t* rawFB = target.colorBuffer.data();

    std::vector<float> r(width * height), g(width * height), b(width * height);
    for (int i = 0; i < width * height; ++i) {
        r[i] = rawFB[i * 3] / 255.0f;
        g[i] = rawFB[i * 3 + 1] / 255.0f;
        b[i] = rawFB[i * 3 + 2] / 255.0f;
    }

    gaussianBlurRGB(r, g, b, width, height);

    for (int i = 0; i < width * height; ++i) {
        rawFB[i * 3]     = static_cast<uint8_t>(std::min(255.0f, r[i] * 255.0f));
        rawFB[i * 3 + 1] = static_cast<uint8_t>(std::min(255.0f, g[i] * 255.0f));
        rawFB[i * 3 + 2] = static_cast<uint8_t>(std::min(255.0f, b[i] * 255.0f));
    }
}

void Renderer::applyBloom(RenderBuffers& target, const float bloomThreshold, const float bloomIntensity)
{
    const int width = target.width;
    const int height = target.height;
    std::uint8_t* rawFB = target.colorBuffer.data();

    // Bright pass: keep only pixels brighter than bloomThreshold, in a
    // separate buffer so blurring doesn't smear the untouched original.
    std::vector<float> r(width * height, 0.0f), g(width * height, 0.0f), b(width * height, 0.0f);
    for (int i = 0; i < width * height; ++i) {
        const float pr = rawFB[i * 3] / 255.0f;
        const float pg = rawFB[i * 3 + 1] / 255.0f;
        const float pb = rawFB[i * 3 + 2] / 255.0f;
        const float luminance = 0.299f * pr + 0.587f * pg + 0.114f * pb;    // Luma formula standard definition.

        if (luminance > bloomThreshold) {
            r[i] = pr;
            g[i] = pg;
            b[i] = pb;
        }
    }

    gaussianBlurRGB(r, g, b, width, height);

    // Add the blurred glow back onto the original image.
    for (int i = 0; i < width * height; ++i) {
        rawFB[i * 3]     = static_cast<uint8_t>(std::min(255.0f, rawFB[i * 3]     + r[i] * bloomIntensity * 255.0f));
        rawFB[i * 3 + 1] = static_cast<uint8_t>(std::min(255.0f, rawFB[i * 3 + 1] + g[i] * bloomIntensity * 255.0f));
        rawFB[i * 3 + 2] = static_cast<uint8_t>(std::min(255.0f, rawFB[i * 3 + 2] + b[i] * bloomIntensity * 255.0f));
    }
}


float Renderer::computePixelOcclusion(const int x, const int y,
                                      const int width, const int height,
                                      const std::vector<float>& zbuffer,
                                      const std::vector<Vec2f>& kernel,
                                      const std::vector<Vec2f>& noise)
{
    const int idx = x + y * width;
    const float currentZ = zbuffer[idx];

    if (currentZ <= -std::numeric_limits<float>::max() + SSAO_BACKGROUND_THRESHOLD) return -1.0f;
    float occlusion = 0.0f;
    Vec2f rot = noise[(x % 4) + (y % 4) * 4];

    for (int i = 0; i < SSAO_RANDOM_PIXEL_SAMPLES; i++) {
        float rx = kernel[i].x() * rot.x() - kernel[i].y() * rot.y();
        float ry = kernel[i].x() * rot.y() + kernel[i].y() * rot.x();

        int sx = x + static_cast<int>(rx * SSAO_SAMPLE_RADIUS);
        int sy = y + static_cast<int>(ry * SSAO_SAMPLE_RADIUS);


        // Occlusion check.
        if (sx >= 0 && sx < width && sy >= 0 && sy < height) {
            const float sampleZ = zbuffer[sx + sy * width];

            if (sampleZ > currentZ + SSAO_BIAS) {
                const float dist = std::abs(currentZ - sampleZ);
                if (dist < SSAO_MAX_OCCLUSION_DISTANCE) {
                    occlusion += 1.0f;
                }
            }
        }
    }

     return 1.0f - std::min(1.0f, (occlusion / SSAO_RANDOM_PIXEL_SAMPLES) * SSAO_STRENGTH);
}

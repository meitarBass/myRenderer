#ifndef RENDERER_RASTERIZER_H
#define RENDERER_RASTERIZER_H

#include "../Math/Geometry.h"
#include "../IO/tgaimage.h"
#include "../IO/ModelLoader.h"
#include "IShader.h"

/**
 * Contains the context of the scene as well as the model to
 * be rendered.
 */
struct RenderContext {
    const ModelLoader &model;
    std::vector<float>& zbuffer;
    TGAImage* framebuffer = nullptr;
    std::vector<Vec3f>* normalBuffer = nullptr;
    int width = 0;
    int height = 0;
};


/**
 * @brief The function determines P barycentric coordinates.
 *         Input contains the triangle vertices in 2D + the
 *           point P which coordinates we want to determine.
 *
 * @param A                         Triangle's vertex in 2D.
 * @param B                         Triangle's vertex in 2D.
 * @param C                         Triangle's vertex in 2D.
 * @param P                                  Input P vertex.
 * @return
 */
Point3 barycentric(const Vec2f& A, const Vec2f& B, const Vec2f& C, const Vec2f& P);


/**
 * @brief Calculates the bounding box of a triangle.
 *
 * @param pts                   Triangle 3 vertices.
 * @return             returns the BBox coordinates.
 */
BBox computeTriangleBBox(const Vec3f pts[3]);


/**
 * @brief Draws the model given its context and a shader.
 *        The function uses multi-threading tiles approach -
 *        1. The framebuffer is divided into tiles sized 32x32 pixels.
 *        2. Each thread works on a single tile until finished.
 *        3. The thread process the next tile available.
 *        4. All tiles marked finished.
 *
 * @param ctx                                       The scene context.
 * @param shader                      How to draw the pixel correctly.
 */
void drawModel(const RenderContext &ctx, IShader& shader);


/**
 * @brief                    Calculates the TBN basis for a triangle.
 *               The calculation requires both the triangle vertices
 *                            and the UV coordinates for each vertex.
 *            The TBN matrix is used for correct normal calculations
 *                                     as well as correct UV mapping.
 *
 * @param pts                                    Triangle 3 vertices.
 * @param uvs                              Triangle 3 UV coordinates.
 * @return                     Returns the new tangent and bitangent.
 */
std::pair<Vec3f, Vec3f> calculateTriangleBasis(const Vec3f pts[3], const Vec2f uvs[3]);

#endif //RENDERER_RASTERIZER_H

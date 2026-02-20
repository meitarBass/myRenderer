#ifndef RENDERER_SHADER_H
#define RENDERER_SHADER_H

#include "../Math/Vec.h"
#include "../Math/Matrix.h"
#include "../IO/tgaimage.h"

/**
 * Contains the pixel's relevant geometrical information
 * in order to render a pixel correctly.
 */
struct Varyings {
    Vec3f screenPos;
    Vec2f uv;
    Vec3f normal;
    Vec3f worldPos;
    Vec3f tangent;
    Vec3f bitangent;
    float invW{1.0f};            // used in '' formula.
    Vec3f normalForBuffer;      // used for out normal.
};


/**
 * Contains the scene's geometrical information
 * in order to render the model correctly.
 */
struct Uniforms {
    Matrix4f4 model;
    Matrix4f4 modelView;
    Matrix4f4 projection;
    Matrix4f4 viewport;
    Matrix3f3 normalMatrix;             // TBN matrix.
    Vec3f lightDir;
    Vec3f cameraPos;

    Matrix4f4 lightSpaceMatrix;

    Matrix4f4 lightProjView;
    const std::vector<float>* shadowMap = nullptr;
    int shadowWidth = 0;
    int shadowHeight = 0;
};

class IShader {
public:
    virtual ~IShader() = default;
    Uniforms uniforms;

    /**
     * @brief Calculates the vertex varyings given -
     * local position, normal, uv coordinates, tangent and bitangent  .
     *
     *
     * @param localPos -        vertex's position in local coordinates.
     * @param normal -            vertex's normal in local coordinates.
     * @param uv -              original uv coordinates from the model.
     * @param tangent -                    original calculated tangent.
     * @param bitangent -                original calculated bitangent.
     * @return -       contains the new coordinates for each component,
     *                                  applying the relevant matrices.
     */
    virtual Varyings vertex(const Vec3f& localPos,
                            const Vec3f& normal,
                            const Vec2f& uv,
                            const Vec3f& tangent,
                            const Vec3f& bitangent) = 0;

    /**
     * @brief Calculates the final color given the varyings and color.
     *        Lighting calculations also happen in this function.
     *
     * @param varyings        output from the vertex shader and linear
     *                  interpolation based on the 3 triangle vertices.
     * @param color            relevant color from the model's texture.
     * @return
     */
    virtual bool fragment(Varyings& varyings,
                          TGAColor &color) = 0;


    /**
     *
     * @param v1                      first triangle vertex.
     * @param v2                     second triangle vertex.
     * @param v3                      third triangle vertex.
     * @param barycentric coordinates of the relevant point.
     *                                  v1 is at location x,
     *                                  v2 is at location y,
     *                                  v3 is at location z.
     * @return
     */
    static Varyings interpolate(const Varyings& v1,
                                const Varyings& v2,
                                const Varyings& v3,
                                const Vec3f& barycentric);
};


#endif //RENDERER_SHADER_H
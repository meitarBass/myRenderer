#ifndef RENDERER_DEPTHSHADER_H
#define RENDERER_DEPTHSHADER_H

#include "../Core/IShader.h"

class DepthShader: public IShader {
public:
    explicit DepthShader(const Uniforms& uniforms);


    /**
     * Calculates the vertex position from light perspective
     * in order to determine which pixels are hidden.
     *
     * Please see IShader.h for more information.
     */
    Varyings vertex(const Vec3f& localPos,
                    const Vec3f& normal,
                    const Vec2f& uv,
                    const Vec3f& tangent,
                    const Vec3f& bitangent) override;


    /**
     * Returns no color, this shader used to determine the shadowMap
     * vector values.
     *
     * Please see IShader.h for more information.
     */
    bool fragment(Varyings& varyings, TGAColor &color) override;
};

#endif //RENDERER_DEPTHSHADER_H
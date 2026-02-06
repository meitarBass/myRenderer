#include "IShader.h"

Varyings IShader::interpolate(const Varyings& v1,
                              const Varyings& v2,
                              const Varyings& v3,
                              const Vec3f& barycentric) {
    Varyings res;

    res.screenPos = v1.screenPos * barycentric.x() + v2.screenPos * barycentric.y() + v3.screenPos * barycentric.z();
    res.invW = v1.invW * barycentric.x() + v2.invW * barycentric.y() + v3.invW * barycentric.z();
    res.uv = v1.uv * barycentric.x() + v2.uv * barycentric.y() + v3.uv * barycentric.z();
    res.normal = v1.normal * barycentric.x() + v2.normal * barycentric.y() + v3.normal * barycentric.z();

    res.worldPos = v1.worldPos * barycentric.x() + v2.worldPos * barycentric.y() + v3.worldPos * barycentric.z();

    res.tangent = v1.tangent * barycentric.x() + v2.tangent * barycentric.y() + v3.tangent * barycentric.z();
    res.bitangent = v1.bitangent * barycentric.x() + v2.bitangent * barycentric.y() + v3.bitangent * barycentric.z();

    return res;
}
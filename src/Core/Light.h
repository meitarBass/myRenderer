#ifndef RENDERER_LIGHT_H
#define RENDERER_LIGHT_H

#include "../Math/Vec.h"

enum class LightType {
    Directional,
    Point
};

struct Light {
    LightType type = LightType::Directional;

    // Used when type is Directional. Points from a lit surface toward the
    // light, not the direction the rays travel in.
    Vec3f direction = {2, 3, 3};

    // Used when type is Point.
    Vec3f position = {2, 3, 3};

    Vec3f color = {1, 1, 1};
    float intensity = 1.0f;
};

#endif //RENDERER_LIGHT_H

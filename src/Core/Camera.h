#ifndef RENDERER_CAMERA_H
#define RENDERER_CAMERA_H

#include "../Math/Vec.h"

struct Camera {
    Point3 pos;
    Vec3f lookAt;
    Vec3f up;

    float focalLength;

    Camera(const Point3 &pos, const Vec3f &lookAt, const Vec3f &up, const float f) :
           pos(pos), lookAt(lookAt), up(up), focalLength(f) {}
};

#endif //RENDERER_CAMERA_H
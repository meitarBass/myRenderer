#ifndef RENDERER_CAMERA_H
#define RENDERER_CAMERA_H

#include "../Math/Vec.h"

struct Camera {
    Point3 pos;
    Vec3f lookAt;
    Vec3f up;

    Vec3f originalPos;
    double yaw = -90.0f;
    double pitch = 0.0f;
    float focalLength;

    Camera(const Point3 &pos, const Vec3f &lookAt, const Vec3f &up, const float f) :
           pos(pos), lookAt(lookAt), up(up), focalLength(f), originalPos(Vec3f(pos)) {}

    void reset() {
        pos = originalPos;
        yaw = -90.0f;
        pitch = 0.0f;
    }
};

#endif //RENDERER_CAMERA_H
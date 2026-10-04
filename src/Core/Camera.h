#ifndef RENDERER_CAMERA_H
#define RENDERER_CAMERA_H

#include "../Math/Matrix.h"

// PerspectiveSimple is the original projection this renderer has always
// used, and stays the default - SSAO/shadows are tuned around its z range.
// FOV and Orthographic are new, opt-in modes that don't touch it.
enum class ProjectionType {
    PerspectiveSimple,
    PerspectiveFov,
    Orthographic
};

struct Camera {
    Point3 pos;
    Vec3f lookAt;
    Vec3f up;

    Vec3f originalPos;
    double yaw = -90.0f;
    double pitch = 0.0f;
    float focalLength;

    float moveSpeed = 2.5f;

    ProjectionType projectionType = ProjectionType::PerspectiveSimple;
    float fov = 60.0f;         // used by PerspectiveFov, degrees
    float nearPlane = 0.1f;    // used by PerspectiveFov / Orthographic
    float farPlane = 100.0f;   // used by PerspectiveFov / Orthographic
    float orthoSize = 3.0f;    // half-height of the view volume, used by Orthographic

    Camera(const Point3 &pos, const Vec3f &lookAt, const Vec3f &up, const float f) :
           pos(pos), lookAt(lookAt), up(up), focalLength(f), originalPos(Vec3f(pos)) {}

    void reset() {
        pos = originalPos;
        yaw = -90.0f;
        pitch = 0.0f;
    }
};

inline Matrix4f4 buildCameraProjection(const Camera& cam, const float aspect) {
    switch (cam.projectionType) {
        case ProjectionType::PerspectiveFov:
            return Matrix4f4::perspective(cam.fov, aspect, cam.nearPlane, cam.farPlane);
        case ProjectionType::Orthographic:
            return Matrix4f4::orthographic(cam.orthoSize * aspect, cam.orthoSize, cam.nearPlane, cam.farPlane);
        case ProjectionType::PerspectiveSimple:
        default:
            return Matrix4f4::projection(cam.focalLength, aspect);
    }
}

#endif //RENDERER_CAMERA_H
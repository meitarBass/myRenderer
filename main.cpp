#include <iostream>
#include <vector>
#include "src/Core/ModelInstance.h"
#include "src/Renderer/Renderer.h"
#include "tests/RendererUnitTests.h"

int main() {
    RendererUnitTests::runAll();

    constexpr int width = 2000;
    constexpr int height = 2000;

    const auto cam = Camera({0, 1, 6}, {0,0,0}, {0, 1, 0}, 3.0);
    Vec3f lightPos = Vec3f(1, 1, 1).normalize() * 3.0f;
    const auto lightDir = (lightPos - cam.lookAt).normalize();
    auto rb = RenderBuffers(width, height);
    auto scene = Scene(cam, lightDir);

    const std::string diabloRoot = "../Models/obj/diablo3_pose/";
    constexpr std::string floorRoot = "../Models/obj/";

    auto diabloModel = ModelInstance(diabloRoot, "diablo3_pose.obj", "diablo3_pose_diffuse.tga", "diablo3_pose_nm_tangent.tga", "diablo3_pose_spec.tga", false);
    diabloModel.rotation = {10, 30, 0};
    // diabloModel.position = {0.2, -0.05, 0};
    auto floorModel = ModelInstance(floorRoot, "floor.obj", "floor_diffuse.tga", "floor_nm_tangent.tga", "floor_spec.tga", false);

    scene.addModel(diabloModel);
    scene.addModel(floorModel);

    Renderer re = {};
    re.render(scene, rb);

    bool res = rb.framebuffer.write_tga_file("diablo2_scene.tga");
    std::cout << "Done! Saved diablo_scene.tga" << std::endl;

    return 0;
}

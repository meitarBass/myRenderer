#include <iostream>
#include "src/Core/ModelInstance.h"
#include "src/Renderer/Renderer.h"
#include "tests/RendererUnitTests.h"

bool scene1() {
    constexpr int width = 2048;
    constexpr int height = 2048;

    const auto cam = Camera({0, 2, 6}, {0,0,0}, {0, 1, 0}, 3.0);
    Vec3f lightPos = Vec3f(2, 3, 3).normalize() * 5.0f;
    const auto lightDir = (lightPos - cam.lookAt).normalize();
    auto rb = RenderBuffers(width, height, width, height);
    auto scene = Scene(cam, lightDir, lightPos);

    const std::string diabloRoot = "../Models/obj/diablo3_pose/";
    const std::string headRoot = "../Models/obj/african_head/";
    constexpr std::string floorRoot = "../Models/obj/";

    auto diabloModel = ModelInstance(diabloRoot, "diablo3_pose.obj", "diablo3_pose_diffuse.tga", "diablo3_pose_nm_tangent.tga", "diablo3_pose_spec.tga", false);
    diabloModel.rotation = {0, 40, 0};
    diabloModel.scale = {0.55, 0.55, 0.55};
    diabloModel.position = {-0.5, -0.42, -1.5};

    scene.addModel(diabloModel);

    diabloModel.rotation = {0, -50, 0};
    diabloModel.scale = {0.45, 0.45, 0.45};
    diabloModel.position = {1.3, -0.53, -2.8};

    scene.addModel(diabloModel);

    diabloModel.rotation = {0, 100, 0};
    diabloModel.scale = {0.35, 0.35, 0.35};
    diabloModel.position = {-1.15, -0.60, -0.1};

    scene.addModel(diabloModel);

    auto head = ModelInstance(headRoot , "african_head.obj", "african_head_diffuse.tga", "african_head_nm_tangent.tga", "african_head_spec.tga", false);
    auto eyes_in = ModelInstance(headRoot , "african_head_eye_inner.obj", "african_head_eye_inner_diffuse.tga", "african_head_eye_inner_nm_tangent.tga", "african_head_eye_inner_spec.tga", false);
    head.rotation = {-20, 10,0 };
    eyes_in.rotation = {-20, 10, 0};

    head.scale = {0.6, 0.6, 0.6};
    eyes_in.scale = {0.6, 0.6, 0.6};

    head.position = {0, -0.8, -1.2};
    eyes_in.position = {0, -0.8, -1.2};

    scene.addModel(head);
    scene.addModel(eyes_in);

    auto floorModel = ModelInstance(floorRoot, "floor.obj", "floor_diffuse.tga", "floor_nm_tangent.tga", "floor_spec.tga", false);
    floorModel.scale = {2.0, 1.0, 2.0};
    floorModel.position = {0.0, 0.0, -2.0};

    scene.addModel(floorModel);
    Renderer::render(scene, rb);

    bool res = rb.framebuffer.write_tga_file("final_scene.tga");
    std::cout << "Done! Saved diablo_scene.tga" << std::endl;
    return res;
}

bool scene2() {
    constexpr int width = 2048;
    constexpr int height = 2048;

    const auto cam = Camera({0, 0, 1}, {0,0,0}, {0, 1, 0}, 3.0);
    Vec3f lightPos = Vec3f(2, 3, 3).normalize() * 5.0f;
    const auto lightDir = (lightPos - cam.lookAt).normalize();
    auto rb = RenderBuffers(width, height, width, height);
    auto scene = Scene(cam, lightDir, lightPos);

    const std::string headRoot = "../Models/obj/african_head/";

    auto head = ModelInstance(headRoot , "african_head.obj", "african_head_diffuse.tga", "african_head_nm_tangent.tga", "african_head_spec.tga", false);
    auto eyes_in = ModelInstance(headRoot , "african_head_eye_inner.obj", "african_head_eye_inner_diffuse.tga", "african_head_eye_inner_nm_tangent.tga", "african_head_eye_inner_spec.tga", false);
    auto eyes_out = ModelInstance(headRoot , "african_head_eye_inner.obj", "african_head_eye_outer_diffuse.tga", "african_head_eye_outer_nm_tangent.tga", "african_head_eye_outer_spec.tga", false);

    head.scale = {0.9, 0.9, 0.9};
    eyes_in.scale = {0.9, 0.9, 0.9};
    eyes_out.scale = {0.9, 0.9, 0.9};

    scene.addModel(head);
    scene.addModel(eyes_in);
    scene.addModel(eyes_out);

    Renderer::render(scene, rb);

    bool res = rb.framebuffer.write_tga_file("final_scene.tga");
    std::cout << "Done! Saved diablo_scene.tga" << std::endl;
    return res;
}

bool scene3() {
    constexpr int width = 2048;
    constexpr int height = 2048;

    const auto cam = Camera({0, 2, 5}, {0,0,0}, {0, 1, 0}, 3.0);
    Vec3f lightPos = Vec3f(2, 3, 3).normalize() * 5.0f;
    const auto lightDir = (lightPos - cam.lookAt).normalize();
    auto rb = RenderBuffers(width, height, width, height);
    auto scene = Scene(cam, lightDir, lightPos);

    const std::string diabloRoot = "../Models/obj/diablo3_pose/";
    constexpr std::string floorRoot = "../Models/obj/";

    auto diabloModel = ModelInstance(diabloRoot, "diablo3_pose.obj", "diablo3_pose_diffuse.tga", "diablo3_pose_nm_tangent.tga", "diablo3_pose_spec.tga", false);
    diabloModel.rotation = {0, 0, 0};
    diabloModel.scale = {1.0, 1.0, 1.0};
    diabloModel.position = {0.0, 0.0, -0.5};

    scene.addModel(diabloModel);

    auto floorModel = ModelInstance(floorRoot, "floor.obj", "floor_diffuse.tga", "floor_nm_tangent.tga", "floor_spec.tga", false);
    floorModel.scale = {2.0, 1.0, 2.0};
    floorModel.position = {0.0, 0.0, -2.0};

    scene.addModel(floorModel);

    Renderer::render(scene, rb);

    bool res = rb.framebuffer.write_tga_file("final_scene.tga");
    std::cout << "Done! Saved diablo_scene.tga" << std::endl;
    return res;
}

int main() {
    RendererUnitTests::runAll();

    return scene1();
}

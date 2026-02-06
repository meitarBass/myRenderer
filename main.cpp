#include "src/Core/Rasterizer.h"
#include "src/Shaders/PhongShader.h"
#include "tests/Tests.h"
#include "src/Core/ModelInstance.h"


int main() {
    RendererTests::runAll();

    constexpr int width = 3200;
    constexpr int height = 3200;

    std::cout << "Loading model and texture..." << std::endl;

    std::vector<ModelInstance> scene;

    // std::string modelRoot = "../Models/obj/african_head/";
    // scene.emplace_back(modelRoot, "african_head.obj", "african_head_diffuse.tga", "african_head_nm_tangent.tga", "african_head_spec.tga", false);
    // scene.emplace_back(modelRoot, "african_head_eye_inner.obj", "african_head_eye_inner_diffuse.tga", "african_head_eye_inner_nm_tangent.tga", "african_head_eye_inner_spec.tga", false);
    // scene.emplace_back(modelRoot, "african_head_eye_outer.obj", "african_head_eye_outer_diffuse.tga", "african_head_eye_outer_nm_tangent.tga", "african_head_eye_outer_spec.tga", true);

    // RendererTests::runVisualSuite(scene, width, height);

    std::cout << "All processes completed successfully." << std::endl;

    std::string modelRoot = "../Models/obj/diablo3_pose/";
    scene.emplace_back(modelRoot, "diablo3_pose.obj", "diablo3_pose_diffuse.tga", "diablo3_pose_nm_tangent.tga", "diablo3_pose_spec.tga", false);

    modelRoot = "../Models/obj/";
    scene.emplace_back(modelRoot, "floor.obj", "floor_diffuse.tga", "floor_nm_tangent.tga", "floor_spec.tga", false);

    RendererTests::runVisualSuite(scene, width, height);
    return 0;
}
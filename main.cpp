#include <iostream>
#include <vector>
#include "src/Core/ModelInstance.h"
#include "tests/RendererUnitTests.h"
#include "tests/VisualTests.h"

int main() {
    RendererUnitTests::runAll();

    constexpr int width = 2000;
    constexpr int height = 2000;

    std::cout << "Loading assets for Visual Test..." << std::endl;

    std::vector<ModelInstance> scene;

    std::string modelRoot = "../Models/obj/diablo3_pose/";
    scene.emplace_back(modelRoot, "diablo3_pose.obj", "diablo3_pose_diffuse.tga", "diablo3_pose_nm_tangent.tga", "diablo3_pose_spec.tga", false);

    modelRoot = "../Models/obj/";
    scene.emplace_back(modelRoot, "floor.obj", "floor_diffuse.tga", "floor_nm_tangent.tga", "floor_spec.tga", false);

    VisualTests::runVisualSuite(scene, width, height);

    return 0;
}

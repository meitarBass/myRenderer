#include "src/Core/Rasterizer.h"
#include "src/Shaders/PhongShader.h"
#include "tests/Tests.h"


int main() {
    RendererTests::runAll();

    constexpr int width = 800;
    constexpr int height = 800;

    std::cout << "Loading model and texture..." << std::endl;
    ModelLoader model("../Models/obj/african_head/african_head.obj");
    TGAImage texture;
    texture.read_tga_file("../Models/obj/african_head/african_head_diffuse.tga");
    texture.flip_vertically();

    RendererTests::runVisualSuite(model, texture, width, height);

    std::cout << "All processes completed successfully." << std::endl;
    return 0;
}
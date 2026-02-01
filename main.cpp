#include "Constants.h"
#include "Rasterizer.h"

#include <cmath>
#include <algorithm>
#include <limits>

int main(int argc, char **argv) {
    constexpr float width = 800.0f;
    constexpr float height = 800.0f;

    Vec3f eye(0, 0, 1);             // Where the viewer is
    Vec3f center(0, 0, 0);           // Where the viewer is watching
    Vec3f up(0, 1, 0);               // The new up

    auto ModelView  = Matrix4f4::lookat(eye, center, up);
    auto Projection = Matrix4f4::projection(7);
    auto View       = Matrix4f4::viewport(width/8, height/8, width*3/4, height*3/4);

    auto mat = View * Projection * ModelView;

    TGAImage framebuffer(width, height, TGAImage::RGB);
    TGAImage textureFile;

    textureFile.read_tga_file("../Models/obj/diablo3_pose/diablo3_pose_diffuse.tga");
    textureFile.flip_vertically();

    auto zbuffer = std::vector<float>(width * height);
    for (int i = 0; i < width * height; i++) {
        zbuffer[i] = -std::numeric_limits<float>::max();
    }

    ModelLoader mdl("../Models/obj/diablo3_pose/diablo3_pose.obj");
    drawModel(mdl, framebuffer, textureFile, zbuffer, mat, eye);

    framebuffer.write_tga_file("output.tga");
    return 0;
}
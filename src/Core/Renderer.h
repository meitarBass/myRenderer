#ifndef RENDERER_RENDERER_H
#define RENDERER_RENDERER_H

#include "../Math/Geometry.h"
#include "../IO/tgaimage.h"
#include "IShader.h"
#include <vector>

class Renderer {
public:
    Renderer(int width, int height)
        : framebuffer(width, height, TGAImage::RGB), zbuffer(width * height, -std::numeric_limits<float>::max()) {}

    void clear(const TGAColor& color) {
        for (int i = 0 ; i < framebuffer.width() ; i++) {
            for (int j = 0 ; j < framebuffer.height(); j++) {
                framebuffer.set(i, j, color);
            }
        }
    }


    void clearDepth() {
        std::fill(zbuffer.begin(), zbuffer.end(), -std::numeric_limits<float>::max());
    }

    TGAImage& getFramebuffer() { return framebuffer; }
    std::vector<float>& getZbuffer() { return zbuffer; }

private:
    TGAColor framebuffer;
    std::vector<float> zbuffer;
};

#endif //RENDERER_RENDERER_H
#ifndef RENDERER_MODELINSTANCE_H
#define RENDERER_MODELINSTANCE_H

#include "../IO/tgaimage.h"
#include "../IO/ModelLoader.h"

struct ModelInstance {
    ModelLoader model;
    TGAImage diffuse;
    TGAImage normal;
    bool useAlphaTest;

    ModelInstance(const std::string& modelRoot,
                  const std::string &objPath, const std::string &diffPath, const std::string &nmPath, const bool useAlphaTest)
        : model(modelRoot + objPath), useAlphaTest(useAlphaTest) {
        diffuse.read_tga_file(modelRoot + diffPath);
        normal.read_tga_file(modelRoot + nmPath);

        diffuse.flip_vertically();
        normal.flip_vertically();
    }
};

#endif //RENDERER_MODELINSTANCE_H
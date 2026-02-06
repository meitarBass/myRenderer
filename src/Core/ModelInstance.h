#ifndef RENDERER_MODELINSTANCE_H
#define RENDERER_MODELINSTANCE_H

#include "../IO/tgaimage.h"
#include "../IO/ModelLoader.h"

struct ModelInstance {
    ModelLoader model;
    TGAImage diffuse;
    TGAImage normal;
    TGAImage specular;
    bool useAlphaTest;

    ModelInstance(const std::string& modelRoot,
                  const std::string &objPath,
                  const std::string &diffPath,
                  const std::string &nmPath,
                  const std::string &specPath,
                  const bool useAlphaTest)
        : model(modelRoot + objPath), useAlphaTest(useAlphaTest) {

        diffuse.read_tga_file(modelRoot + diffPath);
        normal.read_tga_file(modelRoot + nmPath);
        specular.read_tga_file(modelRoot + specPath);

        diffuse.flip_vertically();
        normal.flip_vertically();
        specular.flip_vertically();
    }
};

#endif //RENDERER_MODELINSTANCE_H
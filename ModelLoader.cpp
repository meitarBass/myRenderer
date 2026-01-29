#include "ModelLoader.h"

void ModelLoader::loadFile(const std::string fileName) {
    std::ifstream inputFile(fileName);

    if(!inputFile.is_open()) {
        std::cerr << "Failed to open the file." << std::endl;
        return;
    }

    std::string line;
    while(std::getline(inputFile, line)) {
        std::istringstream iss(line);
        std::string word;

        iss >> word;

        if(word == "v") {
            // Handle vertices
            float x, y, z;
            iss >> x;
            iss >> y;
            iss >> z;

            Vec3f v{x, y, z} ;
            vertices.push_back(v);
        }

        if (word == "f") {
            // Handle faces
            int v, vt, vn;
            int verticesIndices[3], normalIndices[3];

            for (int i = 0; i < 3; i++) {
                iss >> word;
                if (sscanf(word.c_str(), "%d/%d/%d", &v, &vt, &vn) >= 1) {
                    verticesIndices[i] = v - 1; // zero based.
                    normalIndices[i] = vn - 1;
                }
            }

            Face f { verticesIndices, normalIndices };
            faces.push_back(f);
        }


        if (word == "vn") {
            float x, y, z;
            iss >> x;
            iss >> y;
            iss >> z;

            Vec3f vn{x, y, z};
            normals.push_back(vn);
        }
    }

    inputFile.close();
}

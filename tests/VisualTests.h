#ifndef RENDERER_VISUALTESTS_H
#define RENDERER_VISUALTESTS_H

#include <vector>
#include "../src/Core/ModelInstance.h"
#include "../src/Math/Vec.h"

class VisualTests {
public:
    static void runVisualSuite(const std::vector<ModelInstance>& scene, int width, int height);

private:
    static std::vector<float> computeSSAO(const std::vector<float>& zbuffer,
                                          const std::vector<Vec3f>& normalBuffer,
                                          int width, int height);

    static inline float randf() {
        return static_cast<float>(rand()) / static_cast<float>(RAND_MAX);
    }
};


#endif //RENDERER_VISUALTESTS_H
#ifndef RENDERER_BVH_H
#define RENDERER_BVH_H

#include "ModelInstance.h"
#include <vector>


class BVH {
public:

    void build(const std::vector<AABB>& boxes, std::vector<int> indices);
    [[nodiscard]] int closestHit(const Vec3f& rayOrigin, const Vec3f& rayDir) const;

private:
    struct Item {
        int index;
        AABB box;
    };

    struct Node {
        AABB bounds;
        int firstItem = 0; // leaf only: start into `items`
        int count = 0;     // leaf only: number of items; 0 marks an interior node
        int left = -1;
        int right = -1;
    };

    int buildRecursive(int first, int count);
    AABB boundsOf(int first, int count) const;
    static AABB unionOf(const AABB& a, const AABB& b);

    std::vector<Item> items;
    std::vector<Node> nodes;
    int rootIndex = -1;
};

#endif //RENDERER_BVH_H

#include "BVH.h"
#include <algorithm>
#include <limits>

AABB BVH::unionOf(const AABB& a, const AABB& b) {
    AABB result;
    for (int i = 0; i < 3; ++i) {
        result.min[i] = std::min(a.min[i], b.min[i]);
        result.max[i] = std::max(a.max[i], b.max[i]);
    }
    return result;
}

AABB BVH::boundsOf(const int first, const int count) const {
    AABB result;
    for (int i = first; i < first + count; ++i) {
        result = unionOf(result, items[i].box);
    }
    return result;
}

int BVH::buildRecursive(const int first, const int count) {
    Node node;
    node.bounds = boundsOf(first, count);

    constexpr int kLeafThreshold = 2;
    if (count <= kLeafThreshold) {
        node.firstItem = first;
        node.count = count;
        nodes.push_back(node);
        return static_cast<int>(nodes.size()) - 1;
    }

    const Vec3f extent = node.bounds.max - node.bounds.min;
    int axis = 0;
    if (extent.y() > extent[axis]) axis = 1;
    if (extent.z() > extent[axis]) axis = 2;

    std::sort(items.begin() + first, items.begin() + first + count,
              [axis](const Item& a, const Item& b) {
                  const float ca = a.box.min[axis] + a.box.max[axis];
                  const float cb = b.box.min[axis] + b.box.max[axis];
                  return ca < cb;
              });

    const int mid = first + count / 2;
    const int leftIdx = buildRecursive(first, mid - first);
    const int rightIdx = buildRecursive(mid, count - (mid - first));

    node.count = 0;
    node.left = leftIdx;
    node.right = rightIdx;
    nodes.push_back(node);
    return static_cast<int>(nodes.size()) - 1;
}

void BVH::build(const std::vector<AABB>& boxes, std::vector<int> indices) {
    items.clear();
    nodes.clear();
    rootIndex = -1;

    items.reserve(indices.size());
    for (const int idx : indices) {
        items.push_back({idx, boxes[idx]});
    }

    if (items.empty()) return;

    nodes.reserve(items.size() * 2); // a full binary tree over N leaves has < 2N nodes
    rootIndex = buildRecursive(0, static_cast<int>(items.size()));
}

int BVH::closestHit(const Vec3f& rayOrigin, const Vec3f& rayDir) const {
    if (rootIndex < 0) return -1;

    float bestT = std::numeric_limits<float>::max();
    int bestIndex = -1;

    std::vector<int> stack;
    stack.push_back(rootIndex);

    while (!stack.empty()) {
        const int nodeIdx = stack.back();
        stack.pop_back();
        const Node& node = nodes[nodeIdx];

        const float boxT = ModelInstance::RayBoxInterSection(rayOrigin, rayDir, node.bounds.min, node.bounds.max);
        if (boxT < 0 || boxT >= bestT) continue; // miss, or can't possibly beat what we already found

        if (node.count > 0) {
            for (int i = node.firstItem; i < node.firstItem + node.count; ++i) {
                const Item& item = items[i];
                const float t = ModelInstance::RayBoxInterSection(rayOrigin, rayDir, item.box.min, item.box.max);
                if (t >= 0 && t < bestT) {
                    bestT = t;
                    bestIndex = item.index;
                }
            }
        } else {
            stack.push_back(node.left);
            stack.push_back(node.right);
        }
    }

    return bestIndex;
}

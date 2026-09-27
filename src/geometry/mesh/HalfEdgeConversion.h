#pragma once

#include "raylib.h"
#include "HalfEdgeMesh.h"
#include <vector>

namespace core {

class HalfEdgeConversion {
public:
    static HalfEdgeMesh ToHalfEdge(const Mesh& raylibMesh);
    static Mesh FromHalfEdge(const HalfEdgeMesh& heMesh);
};

struct SubdividedMeshCache {
    std::vector<HalfEdgeMesh> heLevels;
    std::vector<Mesh> raylibLevels;

    void Unload();
};

}

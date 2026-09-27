#pragma once

#include "geometry/mesh/HalfEdgeMesh.h"

namespace core {

class LoopSubdivision {
public:
    static HalfEdgeMesh Subdivide(const HalfEdgeMesh& mesh);
};

}

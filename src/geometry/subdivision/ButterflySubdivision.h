#pragma once

#include "geometry/mesh/HalfEdgeMesh.h"

namespace core {

class ButterflySubdivision {
public:
    static HalfEdgeMesh Subdivide(const HalfEdgeMesh& mesh);
};

}

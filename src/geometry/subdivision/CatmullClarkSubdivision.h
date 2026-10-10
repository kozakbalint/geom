#pragma once

#include "geometry/mesh/HalfEdgeMesh.h"

namespace core {

class CatmullClarkSubdivision {
public:
    static HalfEdgeMesh Subdivide(const HalfEdgeMesh& mesh);
};

}

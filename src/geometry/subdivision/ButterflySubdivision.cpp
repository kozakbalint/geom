#include "ButterflySubdivision.h"
#include <unordered_map>

namespace core {

struct EdgeKey {
    Index v0, v1;
    bool operator==(const EdgeKey& other) const {
        return v0 == other.v0 && v1 == other.v1;
    }
};

struct EdgeHash {
    std::size_t operator()(const EdgeKey& key) const {
        return std::hash<Index>()(key.v0) ^ (std::hash<Index>()(key.v1) << 1);
    }
};

HalfEdgeMesh ButterflySubdivision::Subdivide(const HalfEdgeMesh& mesh) {
    HalfEdgeMesh outMesh;
    
    const auto& vertices = mesh.GetVertices();
    const auto& halfEdges = mesh.GetHalfEdges();
    const auto& faces = mesh.GetFaces();
    
    if (vertices.empty() || faces.empty()) return outMesh;
    
    // 1. Butterfly is interpolating, so existing vertices keep their positions
    for (Index v_idx = 0; v_idx < vertices.size(); ++v_idx) {
        outMesh.AddVertex(vertices[v_idx].position);
    }

    // 2. Compute edge vertices
    std::vector<Index> edge_vertex_idx(halfEdges.size(), kInvalidIndex);
    for (Index he_idx = 0; he_idx < halfEdges.size(); ++he_idx) {
        if (edge_vertex_idx[he_idx] != kInvalidIndex) continue;
        
        const auto& he = halfEdges[he_idx];
        Index v1 = he.vertex;
        Index v2 = halfEdges[he.next].vertex;
        
        glm::vec3 pos(0.0f);
        if (he.twin == kInvalidIndex) {
            // Boundary edge rule (linear)
            pos = 0.5f * (vertices[v1].position + vertices[v2].position);
            Index new_v = outMesh.AddVertex(pos);
            edge_vertex_idx[he_idx] = new_v;
        } else {
            // Interior edge rule (8-point stencil)
            Index twin_idx = he.twin;
            const auto& twin_he = halfEdges[twin_idx];
            
            Index he_next = he.next;
            Index he_next_next = halfEdges[he_next].next;
            Index twin_next = twin_he.next;
            Index twin_next_next = halfEdges[twin_next].next;

            Index v3 = halfEdges[he_next_next].vertex;
            Index v4 = halfEdges[twin_next_next].vertex;
            
            auto get_opp = [&](Index h) -> Index {
                Index t = halfEdges[h].twin;
                if (t == kInvalidIndex) return kInvalidIndex;
                Index t_next = halfEdges[t].next;
                Index t_next_next = halfEdges[t_next].next;
                return halfEdges[t_next_next].vertex;
            };

            Index v5 = get_opp(he_next);
            Index v6 = get_opp(he_next_next);
            Index v7 = get_opp(twin_next);
            Index v8 = get_opp(twin_next_next);

            glm::vec3 p1 = vertices[v1].position;
            glm::vec3 p2 = vertices[v2].position;
            
            if (v5 == kInvalidIndex || v6 == kInvalidIndex || v7 == kInvalidIndex || v8 == kInvalidIndex) {
                // Fallback to linear if full 8-point stencil is not available
                pos = 0.5f * (p1 + p2);
            } else {
                glm::vec3 p3 = vertices[v3].position;
                glm::vec3 p4 = vertices[v4].position;
                glm::vec3 p5 = vertices[v5].position;
                glm::vec3 p6 = vertices[v6].position;
                glm::vec3 p7 = vertices[v7].position;
                glm::vec3 p8 = vertices[v8].position;
                
                pos = 0.5f * (p1 + p2) + 0.125f * (p3 + p4) - 0.0625f * (p5 + p6 + p7 + p8);
            }
            
            Index new_v = outMesh.AddVertex(pos);
            edge_vertex_idx[he_idx] = new_v;
            edge_vertex_idx[twin_idx] = new_v;
        }
    }
    
    // 3. Build new faces
    std::unordered_map<EdgeKey, Index, EdgeHash> edgeMap;
    auto addTriangle = [&](Index v0, Index v1, Index v2) {
        Index faceIdx = outMesh.AddFace();
        Index he0 = outMesh.AddHalfEdge();
        Index he1 = outMesh.AddHalfEdge();
        Index he2 = outMesh.AddHalfEdge();

        outMesh.GetFacesMut()[faceIdx].halfedge = he0;

        outMesh.GetHalfEdgesMut()[he0].vertex = v0;
        outMesh.GetHalfEdgesMut()[he0].face = faceIdx;
        outMesh.GetHalfEdgesMut()[he0].next = he1;
        outMesh.GetHalfEdgesMut()[he0].prev = he2;
        if (outMesh.GetVerticesMut()[v0].halfedge == kInvalidIndex) outMesh.GetVerticesMut()[v0].halfedge = he0;

        outMesh.GetHalfEdgesMut()[he1].vertex = v1;
        outMesh.GetHalfEdgesMut()[he1].face = faceIdx;
        outMesh.GetHalfEdgesMut()[he1].next = he2;
        outMesh.GetHalfEdgesMut()[he1].prev = he0;
        if (outMesh.GetVerticesMut()[v1].halfedge == kInvalidIndex) outMesh.GetVerticesMut()[v1].halfedge = he1;

        outMesh.GetHalfEdgesMut()[he2].vertex = v2;
        outMesh.GetHalfEdgesMut()[he2].face = faceIdx;
        outMesh.GetHalfEdgesMut()[he2].next = he0;
        outMesh.GetHalfEdgesMut()[he2].prev = he1;
        if (outMesh.GetVerticesMut()[v2].halfedge == kInvalidIndex) outMesh.GetVerticesMut()[v2].halfedge = he2;

        auto linkTwin = [&](Index hIdx, Index startV, Index endV) {
            EdgeKey twinKey{endV, startV};
            auto it = edgeMap.find(twinKey);
            if (it != edgeMap.end()) {
                outMesh.GetHalfEdgesMut()[hIdx].twin = it->second;
                outMesh.GetHalfEdgesMut()[it->second].twin = hIdx;
            } else {
                edgeMap[{startV, endV}] = hIdx;
            }
        };

        linkTwin(he0, v0, v1);
        linkTwin(he1, v1, v2);
        linkTwin(he2, v2, v0);
    };
    
    for (const auto& face : faces) {
        if (face.halfedge == kInvalidIndex) continue;
        
        Index he0 = face.halfedge;
        Index he1 = halfEdges[he0].next;
        Index he2 = halfEdges[he1].next;
        
        Index v0 = halfEdges[he0].vertex;
        Index v1 = halfEdges[he1].vertex;
        Index v2 = halfEdges[he2].vertex;
        
        Index ev0 = edge_vertex_idx[he0];
        Index ev1 = edge_vertex_idx[he1];
        Index ev2 = edge_vertex_idx[he2];
        
        addTriangle(v0, ev0, ev2);
        addTriangle(v1, ev1, ev0);
        addTriangle(v2, ev2, ev1);
        addTriangle(ev0, ev1, ev2);
    }
    
    return outMesh;
}

}

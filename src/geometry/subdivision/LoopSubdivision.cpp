#include "LoopSubdivision.h"
#include <unordered_map>
#include <cmath>

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

struct VInfo {
    std::vector<Index> neighbors;
    std::vector<Index> boundary_neighbors;
};

HalfEdgeMesh LoopSubdivision::Subdivide(const HalfEdgeMesh& mesh) {
    HalfEdgeMesh outMesh;
    
    const auto& vertices = mesh.GetVertices();
    const auto& halfEdges = mesh.GetHalfEdges();
    const auto& faces = mesh.GetFaces();
    
    if (vertices.empty() || faces.empty()) return outMesh;
    
    // 1. Gather vertex connectivity
    std::vector<VInfo> vinfo(vertices.size());
    for (Index he_idx = 0; he_idx < halfEdges.size(); ++he_idx) {
        const auto& he = halfEdges[he_idx];
        Index v_src = he.vertex;
        Index v_dst = halfEdges[he.next].vertex;
        
        vinfo[v_src].neighbors.push_back(v_dst);
        
        if (he.twin == kInvalidIndex) {
            vinfo[v_src].boundary_neighbors.push_back(v_dst);
            vinfo[v_dst].boundary_neighbors.push_back(v_src);
        }
    }
    
    // 2. Compute new positions for existing vertices
    for (Index v_idx = 0; v_idx < vertices.size(); ++v_idx) {
        glm::vec3 v_pos = vertices[v_idx].position;
        glm::vec3 new_pos(0.0f);
        
        if (vinfo[v_idx].boundary_neighbors.size() >= 2) {
            // Boundary rule
            Index b1 = vinfo[v_idx].boundary_neighbors[0];
            Index b2 = vinfo[v_idx].boundary_neighbors[1];
            glm::vec3 p_b1 = vertices[b1].position;
            glm::vec3 p_b2 = vertices[b2].position;
            new_pos = 0.75f * v_pos + 0.125f * (p_b1 + p_b2);
        } else {
            // Interior rule
            int n = static_cast<int>(vinfo[v_idx].neighbors.size());
            float beta = 0.0f;
            if (n == 3) {
                beta = 3.0f / 16.0f;
            } else if (n > 0) {
                float pi = 3.14159265358979323846f;
                float center = 0.375f + 0.25f * std::cos(2.0f * pi / n);
                beta = (1.0f / n) * (0.625f - center * center);
            }
            
            glm::vec3 sum_neighbors(0.0f);
            for (Index neighbor : vinfo[v_idx].neighbors) {
                sum_neighbors += vertices[neighbor].position;
            }
            
            new_pos = (1.0f - n * beta) * v_pos + beta * sum_neighbors;
        }
        outMesh.AddVertex(new_pos);
    }
    
    // 3. Compute edge vertices
    std::vector<Index> edge_vertex_idx(halfEdges.size(), kInvalidIndex);
    for (Index he_idx = 0; he_idx < halfEdges.size(); ++he_idx) {
        if (edge_vertex_idx[he_idx] != kInvalidIndex) continue;
        
        const auto& he = halfEdges[he_idx];
        Index v1 = he.vertex;
        Index v2 = halfEdges[he.next].vertex;
        
        glm::vec3 pos(0.0f);
        if (he.twin == kInvalidIndex) {
            // Boundary edge rule
            pos = 0.5f * (vertices[v1].position + vertices[v2].position);
            Index new_v = outMesh.AddVertex(pos);
            edge_vertex_idx[he_idx] = new_v;
        } else {
            // Interior edge rule
            Index twin_idx = he.twin;
            const auto& twin_he = halfEdges[twin_idx];
            
            Index v3 = halfEdges[halfEdges[he.next].next].vertex;
            Index v4 = halfEdges[halfEdges[twin_he.next].next].vertex;
            
            glm::vec3 p1 = vertices[v1].position;
            glm::vec3 p2 = vertices[v2].position;
            glm::vec3 p3 = vertices[v3].position;
            glm::vec3 p4 = vertices[v4].position;
            
            pos = 0.375f * (p1 + p2) + 0.125f * (p3 + p4);
            
            Index new_v = outMesh.AddVertex(pos);
            edge_vertex_idx[he_idx] = new_v;
            edge_vertex_idx[twin_idx] = new_v;
        }
    }
    
    // 4. Build new faces
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

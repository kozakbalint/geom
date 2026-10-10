#include "CatmullClarkSubdivision.h"
#include <unordered_map>
#include <vector>

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

struct VertexConnectivity {
    std::vector<Index> incident_faces;
    std::vector<Index> neighbors;
    std::vector<Index> boundary_neighbors;
};

HalfEdgeMesh CatmullClarkSubdivision::Subdivide(const HalfEdgeMesh& mesh) {
    HalfEdgeMesh outMesh;

    const auto& vertices = mesh.GetVertices();
    const auto& halfEdges = mesh.GetHalfEdges();
    const auto& faces = mesh.GetFaces();

    if (vertices.empty() || faces.empty()) return outMesh;

    // 1. Compute Face Points
    std::vector<glm::vec3> face_points(faces.size(), glm::vec3(0.0f));
    for (Index f_idx = 0; f_idx < faces.size(); ++f_idx) {
        Index start_he = faces[f_idx].halfedge;
        if (start_he == kInvalidIndex) continue;

        Index curr_he = start_he;
        glm::vec3 sum(0.0f);
        int count = 0;
        do {
            sum += vertices[halfEdges[curr_he].vertex].position;
            count++;
            curr_he = halfEdges[curr_he].next;
        } while (curr_he != start_he && curr_he != kInvalidIndex);

        if (count > 0) {
            face_points[f_idx] = sum / static_cast<float>(count);
        }
    }

    // 2. Gather connectivity for original vertices
    std::vector<VertexConnectivity> vinfo(vertices.size());
    for (Index he_idx = 0; he_idx < halfEdges.size(); ++he_idx) {
        const auto& he = halfEdges[he_idx];
        Index v_src = he.vertex;
        Index v_dst = halfEdges[he.next].vertex;

        vinfo[v_src].neighbors.push_back(v_dst);
        if (he.face != kInvalidIndex) {
            vinfo[v_src].incident_faces.push_back(he.face);
        }

        if (he.twin == kInvalidIndex) {
            vinfo[v_src].boundary_neighbors.push_back(v_dst);
            vinfo[v_dst].boundary_neighbors.push_back(v_src);
        }
    }

    // 3. Compute updated positions for original vertices (added to outMesh with index 0 .. N_V-1)
    for (Index v_idx = 0; v_idx < vertices.size(); ++v_idx) {
        glm::vec3 v_pos = vertices[v_idx].position;
        glm::vec3 new_pos(0.0f);

        if (vinfo[v_idx].boundary_neighbors.size() >= 2) {
            // Cubic spline boundary rule: 1/8 b1 + 3/4 v + 1/8 b2
            Index b1 = vinfo[v_idx].boundary_neighbors[0];
            Index b2 = vinfo[v_idx].boundary_neighbors[1];
            new_pos = 0.75f * v_pos + 0.125f * (vertices[b1].position + vertices[b2].position);
        } else if (vinfo[v_idx].boundary_neighbors.size() == 1) {
            Index b1 = vinfo[v_idx].boundary_neighbors[0];
            new_pos = 0.75f * v_pos + 0.25f * vertices[b1].position;
        } else {
            // Interior vertex rule: (F_avg + 2 * R_avg + (k - 3) * v) / k
            int k = static_cast<int>(vinfo[v_idx].neighbors.size());
            if (k < 3) {
                new_pos = v_pos;
            } else {
                glm::vec3 f_sum(0.0f);
                for (Index f : vinfo[v_idx].incident_faces) {
                    f_sum += face_points[f];
                }
                glm::vec3 f_avg = f_sum / static_cast<float>(vinfo[v_idx].incident_faces.size());

                glm::vec3 r_sum(0.0f);
                for (Index neighbor : vinfo[v_idx].neighbors) {
                    r_sum += 0.5f * (v_pos + vertices[neighbor].position);
                }
                glm::vec3 r_avg = r_sum / static_cast<float>(k);

                new_pos = (f_avg + 2.0f * r_avg + static_cast<float>(k - 3) * v_pos) / static_cast<float>(k);
            }
        }
        outMesh.AddVertex(new_pos);
    }

    // 4. Add face points to outMesh (indices N_V .. N_V + N_F - 1)
    std::vector<Index> face_vertex_idx(faces.size(), kInvalidIndex);
    for (Index f_idx = 0; f_idx < faces.size(); ++f_idx) {
        face_vertex_idx[f_idx] = outMesh.AddVertex(face_points[f_idx]);
    }

    // 5. Compute and add Edge Points
    std::vector<Index> edge_vertex_idx(halfEdges.size(), kInvalidIndex);
    for (Index he_idx = 0; he_idx < halfEdges.size(); ++he_idx) {
        if (edge_vertex_idx[he_idx] != kInvalidIndex) continue;

        const auto& he = halfEdges[he_idx];
        Index v1 = he.vertex;
        Index v2 = halfEdges[he.next].vertex;

        glm::vec3 pos(0.0f);
        if (he.twin == kInvalidIndex) {
            // Boundary edge rule: midpoint
            pos = 0.5f * (vertices[v1].position + vertices[v2].position);
            Index new_v = outMesh.AddVertex(pos);
            edge_vertex_idx[he_idx] = new_v;
        } else {
            // Interior edge rule: average of 2 endpoints + 2 adjacent face points
            Index twin_idx = he.twin;
            Index f1 = he.face;
            Index f2 = halfEdges[twin_idx].face;

            if (f1 != kInvalidIndex && f2 != kInvalidIndex) {
                pos = 0.25f * (vertices[v1].position + vertices[v2].position + face_points[f1] + face_points[f2]);
            } else {
                pos = 0.5f * (vertices[v1].position + vertices[v2].position);
            }

            Index new_v = outMesh.AddVertex(pos);
            edge_vertex_idx[he_idx] = new_v;
            edge_vertex_idx[twin_idx] = new_v;
        }
    }

    // 6. Build new quadrilateral faces
    std::unordered_map<EdgeKey, Index, EdgeHash> edgeMap;

    auto addQuad = [&](Index v0, Index v1, Index v2, Index v3) {
        Index faceIdx = outMesh.AddFace();
        Index he0 = outMesh.AddHalfEdge();
        Index he1 = outMesh.AddHalfEdge();
        Index he2 = outMesh.AddHalfEdge();
        Index he3 = outMesh.AddHalfEdge();

        outMesh.GetFacesMut()[faceIdx].halfedge = he0;

        outMesh.GetHalfEdgesMut()[he0].vertex = v0;
        outMesh.GetHalfEdgesMut()[he0].face = faceIdx;
        outMesh.GetHalfEdgesMut()[he0].next = he1;
        outMesh.GetHalfEdgesMut()[he0].prev = he3;
        if (outMesh.GetVerticesMut()[v0].halfedge == kInvalidIndex)
            outMesh.GetVerticesMut()[v0].halfedge = he0;

        outMesh.GetHalfEdgesMut()[he1].vertex = v1;
        outMesh.GetHalfEdgesMut()[he1].face = faceIdx;
        outMesh.GetHalfEdgesMut()[he1].next = he2;
        outMesh.GetHalfEdgesMut()[he1].prev = he0;
        if (outMesh.GetVerticesMut()[v1].halfedge == kInvalidIndex)
            outMesh.GetVerticesMut()[v1].halfedge = he1;

        outMesh.GetHalfEdgesMut()[he2].vertex = v2;
        outMesh.GetHalfEdgesMut()[he2].face = faceIdx;
        outMesh.GetHalfEdgesMut()[he2].next = he3;
        outMesh.GetHalfEdgesMut()[he2].prev = he1;
        if (outMesh.GetVerticesMut()[v2].halfedge == kInvalidIndex)
            outMesh.GetVerticesMut()[v2].halfedge = he2;

        outMesh.GetHalfEdgesMut()[he3].vertex = v3;
        outMesh.GetHalfEdgesMut()[he3].face = faceIdx;
        outMesh.GetHalfEdgesMut()[he3].next = he0;
        outMesh.GetHalfEdgesMut()[he3].prev = he2;
        if (outMesh.GetVerticesMut()[v3].halfedge == kInvalidIndex)
            outMesh.GetVerticesMut()[v3].halfedge = he3;

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
        linkTwin(he2, v2, v3);
        linkTwin(he3, v3, v0);
    };

    for (Index f_idx = 0; f_idx < faces.size(); ++f_idx) {
        Index start_he = faces[f_idx].halfedge;
        if (start_he == kInvalidIndex) continue;

        std::vector<Index> face_hes;
        Index curr_he = start_he;
        do {
            face_hes.push_back(curr_he);
            curr_he = halfEdges[curr_he].next;
        } while (curr_he != start_he && curr_he != kInvalidIndex);

        size_t m = face_hes.size();
        if (m < 3) continue;

        Index f_pt = face_vertex_idx[f_idx];

        for (size_t i = 0; i < m; ++i) {
            Index h_curr = face_hes[i];
            Index h_prev = face_hes[(i + m - 1) % m];

            Index v_orig = halfEdges[h_curr].vertex;
            Index e_curr = edge_vertex_idx[h_curr];
            Index e_prev = edge_vertex_idx[h_prev];

            addQuad(v_orig, e_curr, f_pt, e_prev);
        }
    }

    return outMesh;
}

} // namespace core

#include "geometry/mesh/HalfEdgeMesh.h"
#include <unordered_map>

namespace core {

bool HalfEdgeMesh::IsBoundaryVertex(Index v) const noexcept {
    if (v >= m_Vertices.size() || m_Vertices[v].halfedge == kInvalidIndex) {
        return false;
    }
    Index startHe = m_Vertices[v].halfedge;
    Index curr = startHe;
    size_t guard = 0;
    do {
        if (m_HalfEdges[curr].twin == kInvalidIndex) {
            return true;
        }
        Index prevHe = m_HalfEdges[curr].prev;
        if (prevHe == kInvalidIndex) {
            return true;
        }
        curr = m_HalfEdges[prevHe].twin;
    } while (curr != kInvalidIndex && curr != startHe && ++guard < m_HalfEdges.size());

    return (curr == kInvalidIndex);
}

std::vector<Index> HalfEdgeMesh::GetVertexNeighbors(Index v) const {
    std::vector<Index> neighbors;
    if (v >= m_Vertices.size() || m_Vertices[v].halfedge == kInvalidIndex) {
        return neighbors;
    }

    Index startHe = m_Vertices[v].halfedge;
    Index curr = startHe;
    bool hitBoundary = false;
    size_t guard = 0;

    // Forward circulation (CCW: prev -> twin)
    do {
        Index nextHe = m_HalfEdges[curr].next;
        if (nextHe != kInvalidIndex) {
            neighbors.push_back(m_HalfEdges[nextHe].vertex);
        }

        Index prevHe = m_HalfEdges[curr].prev;
        if (prevHe == kInvalidIndex) {
            hitBoundary = true;
            break;
        }
        curr = m_HalfEdges[prevHe].twin;
    } while (curr != kInvalidIndex && curr != startHe && ++guard < m_HalfEdges.size());

    // If a boundary was encountered, circulate backwards from startHe (CW: twin -> next)
    if (hitBoundary || curr == kInvalidIndex) {
        Index backCurr = m_HalfEdges[startHe].twin;
        std::vector<Index> backNeighbors;
        guard = 0;
        while (backCurr != kInvalidIndex && backCurr != startHe && ++guard < m_HalfEdges.size()) {
            backCurr = m_HalfEdges[backCurr].next;
            if (backCurr == kInvalidIndex) break;
            backNeighbors.push_back(m_HalfEdges[backCurr].vertex);
            backCurr = m_HalfEdges[backCurr].twin;
        }
        neighbors.insert(neighbors.begin(), backNeighbors.rbegin(), backNeighbors.rend());
    }

    return neighbors;
}

std::vector<Index> HalfEdgeMesh::GetVertexFaces(Index v) const {
    std::vector<Index> faces;
    if (v >= m_Vertices.size() || m_Vertices[v].halfedge == kInvalidIndex) {
        return faces;
    }

    Index startHe = m_Vertices[v].halfedge;
    Index curr = startHe;
    bool hitBoundary = false;
    size_t guard = 0;

    // Forward walk
    do {
        if (m_HalfEdges[curr].face != kInvalidIndex) {
            faces.push_back(m_HalfEdges[curr].face);
        }
        Index prevHe = m_HalfEdges[curr].prev;
        if (prevHe == kInvalidIndex) {
            hitBoundary = true;
            break;
        }
        curr = m_HalfEdges[prevHe].twin;
    } while (curr != kInvalidIndex && curr != startHe && ++guard < m_HalfEdges.size());

    // Backward walk if boundary reached
    if (hitBoundary || curr == kInvalidIndex) {
        Index backCurr = m_HalfEdges[startHe].twin;
        std::vector<Index> backFaces;
        guard = 0;
        while (backCurr != kInvalidIndex && backCurr != startHe && ++guard < m_HalfEdges.size()) {
            if (m_HalfEdges[backCurr].face != kInvalidIndex) {
                backFaces.push_back(m_HalfEdges[backCurr].face);
            }
            backCurr = m_HalfEdges[backCurr].next;
            if (backCurr == kInvalidIndex) break;
            backCurr = m_HalfEdges[backCurr].twin;
        }
        faces.insert(faces.begin(), backFaces.rbegin(), backFaces.rend());
    }

    return faces;
}

std::vector<Index> HalfEdgeMesh::GetFaceVertices(Index f) const {
    std::vector<Index> verts;
    if (f >= m_Faces.size() || m_Faces[f].halfedge == kInvalidIndex) {
        return verts;
    }

    Index startHe = m_Faces[f].halfedge;
    Index curr = startHe;
    size_t guard = 0;
    do {
        verts.push_back(m_HalfEdges[curr].vertex);
        curr = m_HalfEdges[curr].next;
    } while (curr != startHe && curr != kInvalidIndex && ++guard < m_HalfEdges.size());

    return verts;
}

std::vector<Index> HalfEdgeMesh::GetFaceHalfEdges(Index f) const {
    std::vector<Index> hes;
    if (f >= m_Faces.size() || m_Faces[f].halfedge == kInvalidIndex) {
        return hes;
    }

    Index startHe = m_Faces[f].halfedge;
    Index curr = startHe;
    size_t guard = 0;
    do {
        hes.push_back(curr);
        curr = m_HalfEdges[curr].next;
    } while (curr != startHe && curr != kInvalidIndex && ++guard < m_HalfEdges.size());

    return hes;
}

void HalfEdgeMesh::LinkTwins() {
    std::unordered_map<EdgeKey, Index, EdgeHash> edgeMap;
    edgeMap.reserve(m_HalfEdges.size());

    for (Index hIdx = 0; hIdx < static_cast<Index>(m_HalfEdges.size()); ++hIdx) {
        Index nextHe = m_HalfEdges[hIdx].next;
        if (nextHe == kInvalidIndex) continue;

        Index startV = m_HalfEdges[hIdx].vertex;
        Index endV = m_HalfEdges[nextHe].vertex;

        EdgeKey twinKey{ endV, startV };
        auto it = edgeMap.find(twinKey);
        if (it != edgeMap.end()) {
            m_HalfEdges[hIdx].twin = it->second;
            m_HalfEdges[it->second].twin = hIdx;
        } else {
            edgeMap[{ startV, endV }] = hIdx;
        }
    }
}

} // namespace core

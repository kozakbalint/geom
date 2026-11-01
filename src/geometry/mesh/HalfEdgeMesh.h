#pragma once

#include "geometry/mesh/TopologyTypes.h"
#include <vector>
#include <glm/glm.hpp>

namespace core {

struct HalfEdge {
    Index twin = kInvalidIndex;
    Index next = kInvalidIndex;
    Index prev = kInvalidIndex;
    Index vertex = kInvalidIndex;
    Index face = kInvalidIndex;
};

struct Vertex {
    glm::vec3 position{0.0f};
    Index halfedge = kInvalidIndex;
};

struct Face {
    Index halfedge = kInvalidIndex;
};

class HalfEdgeMesh {
public:
    HalfEdgeMesh() = default;
    ~HalfEdgeMesh() = default;

    const std::vector<Vertex>& GetVertices() const { return m_Vertices; }
    const std::vector<HalfEdge>& GetHalfEdges() const { return m_HalfEdges; }
    const std::vector<Face>& GetFaces() const { return m_Faces; }

    std::vector<Vertex>& GetVerticesMut() { return m_Vertices; }
    std::vector<HalfEdge>& GetHalfEdgesMut() { return m_HalfEdges; }
    std::vector<Face>& GetFacesMut() { return m_Faces; }

    Index AddVertex(const glm::vec3& position) {
        Index idx = static_cast<Index>(m_Vertices.size());
        m_Vertices.push_back({position, kInvalidIndex});
        return idx;
    }

    Index AddFace() {
        Index idx = static_cast<Index>(m_Faces.size());
        m_Faces.push_back({kInvalidIndex});
        return idx;
    }

    Index AddHalfEdge() {
        Index idx = static_cast<Index>(m_HalfEdges.size());
        m_HalfEdges.push_back({});
        return idx;
    }

    void Clear() {
        m_Vertices.clear();
        m_HalfEdges.clear();
        m_Faces.clear();
    }

    // Direct accessors with bounds guards
    Index GetTwin(Index he) const noexcept {
        return (he < m_HalfEdges.size()) ? m_HalfEdges[he].twin : kInvalidIndex;
    }
    Index GetNext(Index he) const noexcept {
        return (he < m_HalfEdges.size()) ? m_HalfEdges[he].next : kInvalidIndex;
    }
    Index GetPrev(Index he) const noexcept {
        return (he < m_HalfEdges.size()) ? m_HalfEdges[he].prev : kInvalidIndex;
    }
    Index GetVertex(Index he) const noexcept {
        return (he < m_HalfEdges.size()) ? m_HalfEdges[he].vertex : kInvalidIndex;
    }
    Index GetFace(Index he) const noexcept {
        return (he < m_HalfEdges.size()) ? m_HalfEdges[he].face : kInvalidIndex;
    }

    bool IsBoundaryEdge(Index he) const noexcept {
        return GetTwin(he) == kInvalidIndex;
    }
    bool IsBoundaryVertex(Index v) const noexcept;

    // Circulation and traversal queries
    std::vector<Index> GetVertexNeighbors(Index v) const;
    std::vector<Index> GetVertexFaces(Index v) const;
    std::vector<Index> GetFaceVertices(Index f) const;
    std::vector<Index> GetFaceHalfEdges(Index f) const;

    // Twin linkage
    void LinkTwins(Index heA, Index heB) noexcept {
        if (heA < m_HalfEdges.size() && heB < m_HalfEdges.size()) {
            m_HalfEdges[heA].twin = heB;
            m_HalfEdges[heB].twin = heA;
        }
    }
    void LinkTwins();

private:
    std::vector<Vertex> m_Vertices;
    std::vector<HalfEdge> m_HalfEdges;
    std::vector<Face> m_Faces;
};

}


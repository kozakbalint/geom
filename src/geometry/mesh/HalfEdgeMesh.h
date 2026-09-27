#pragma once

#include <vector>
#include <cstdint>
#include <limits>
#include <glm/glm.hpp>

namespace core {

using Index = uint32_t;
constexpr Index kInvalidIndex = std::numeric_limits<Index>::max();

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

private:
    std::vector<Vertex> m_Vertices;
    std::vector<HalfEdge> m_HalfEdges;
    std::vector<Face> m_Faces;
};

}

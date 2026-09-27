#include "HalfEdgeConversion.h"
#include <unordered_map>
#include <utility>
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

struct Vec3Key {
    float x, y, z;
    bool operator==(const Vec3Key& other) const {
        return x == other.x && y == other.y && z == other.z;
    }
};

struct Vec3Hash {
    std::size_t operator()(const Vec3Key& key) const {
        std::size_t h1 = std::hash<float>()(key.x);
        std::size_t h2 = std::hash<float>()(key.y);
        std::size_t h3 = std::hash<float>()(key.z);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

HalfEdgeMesh HalfEdgeConversion::ToHalfEdge(const Mesh& raylibMesh) {
    HalfEdgeMesh heMesh;

    if (raylibMesh.vertexCount == 0 || raylibMesh.vertices == nullptr) {
        return heMesh;
    }

    std::vector<Index> oldToNewIndex(raylibMesh.vertexCount);
    std::unordered_map<Vec3Key, Index, Vec3Hash> vertexMap;

    for (int i = 0; i < raylibMesh.vertexCount; ++i) {
        Vec3Key key = {
            raylibMesh.vertices[i * 3 + 0],
            raylibMesh.vertices[i * 3 + 1],
            raylibMesh.vertices[i * 3 + 2]
        };
        
        auto it = vertexMap.find(key);
        if (it != vertexMap.end()) {
            oldToNewIndex[i] = it->second;
        } else {
            Index newIdx = heMesh.AddVertex(glm::vec3(key.x, key.y, key.z));
            vertexMap[key] = newIdx;
            oldToNewIndex[i] = newIdx;
        }
    }

    std::unordered_map<EdgeKey, Index, EdgeHash> edgeMap;

    int triangleCount = raylibMesh.triangleCount;
    auto getVertexIndex = [&](int index) -> Index {
        int oldIdx;
        if (raylibMesh.indices != nullptr) {
            oldIdx = raylibMesh.indices[index];
        } else {
            oldIdx = index;
        }
        return oldToNewIndex[oldIdx];
    };

    for (int i = 0; i < triangleCount; ++i) {
        Index v0 = getVertexIndex(i * 3 + 0);
        Index v1 = getVertexIndex(i * 3 + 1);
        Index v2 = getVertexIndex(i * 3 + 2);

        Index faceIdx = heMesh.AddFace();
        Index he0 = heMesh.AddHalfEdge();
        Index he1 = heMesh.AddHalfEdge();
        Index he2 = heMesh.AddHalfEdge();

        auto& heMeshFaces = heMesh.GetFacesMut();
        auto& heMeshHalfEdges = heMesh.GetHalfEdgesMut();
        auto& heMeshVertices = heMesh.GetVerticesMut();

        heMeshFaces[faceIdx].halfedge = he0;

        heMeshHalfEdges[he0].vertex = v0;
        heMeshHalfEdges[he0].face = faceIdx;
        heMeshHalfEdges[he0].next = he1;
        heMeshHalfEdges[he0].prev = he2;
        if (heMeshVertices[v0].halfedge == kInvalidIndex) heMeshVertices[v0].halfedge = he0;

        heMeshHalfEdges[he1].vertex = v1;
        heMeshHalfEdges[he1].face = faceIdx;
        heMeshHalfEdges[he1].next = he2;
        heMeshHalfEdges[he1].prev = he0;
        if (heMeshVertices[v1].halfedge == kInvalidIndex) heMeshVertices[v1].halfedge = he1;

        heMeshHalfEdges[he2].vertex = v2;
        heMeshHalfEdges[he2].face = faceIdx;
        heMeshHalfEdges[he2].next = he0;
        heMeshHalfEdges[he2].prev = he1;
        if (heMeshVertices[v2].halfedge == kInvalidIndex) heMeshVertices[v2].halfedge = he2;

        auto linkTwin = [&](Index hIdx, Index startV, Index endV) {
            EdgeKey twinKey{endV, startV};
            auto it = edgeMap.find(twinKey);
            if (it != edgeMap.end()) {
                heMeshHalfEdges[hIdx].twin = it->second;
                heMeshHalfEdges[it->second].twin = hIdx;
            } else {
                edgeMap[{startV, endV}] = hIdx;
            }
        };

        linkTwin(he0, v0, v1);
        linkTwin(he1, v1, v2);
        linkTwin(he2, v2, v0);
    }

    return heMesh;
}

Mesh HalfEdgeConversion::FromHalfEdge(const HalfEdgeMesh& heMesh) {
    Mesh mesh = { 0 };

    const auto& vertices = heMesh.GetVertices();
    const auto& faces = heMesh.GetFaces();
    const auto& halfEdges = heMesh.GetHalfEdges();

    if (vertices.empty()) return mesh;

    std::vector<Index> allFaceIndices;
    
    for (const auto& face : faces) {
        if (face.halfedge == kInvalidIndex) continue;
        
        Index startHe = face.halfedge;
        Index currHe = startHe;
        
        std::vector<Index> faceVertices;
        do {
            faceVertices.push_back(halfEdges[currHe].vertex);
            currHe = halfEdges[currHe].next;
        } while (currHe != startHe && currHe != kInvalidIndex);

        if (faceVertices.size() >= 3) {
            for (size_t i = 1; i < faceVertices.size() - 1; ++i) {
                allFaceIndices.push_back(faceVertices[0]);
                allFaceIndices.push_back(faceVertices[i]);
                allFaceIndices.push_back(faceVertices[i + 1]);
            }
        }
    }

    mesh.triangleCount = static_cast<int>(allFaceIndices.size() / 3);
    bool useIndices = vertices.size() <= 65535;

    if (useIndices) {
        mesh.vertexCount = static_cast<int>(vertices.size());
        mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
        mesh.indices = (unsigned short*)MemAlloc(mesh.triangleCount * 3 * sizeof(unsigned short));

        for (int i = 0; i < mesh.vertexCount; ++i) {
            mesh.vertices[i * 3 + 0] = vertices[i].position.x;
            mesh.vertices[i * 3 + 1] = vertices[i].position.y;
            mesh.vertices[i * 3 + 2] = vertices[i].position.z;
        }

        for (size_t i = 0; i < allFaceIndices.size(); ++i) {
            mesh.indices[i] = static_cast<unsigned short>(allFaceIndices[i]);
        }
    } else {
        // Unroll into unindexed mesh to avoid 16-bit index limits
        mesh.vertexCount = mesh.triangleCount * 3;
        mesh.vertices = (float*)MemAlloc(mesh.vertexCount * 3 * sizeof(float));
        mesh.indices = nullptr;

        for (size_t i = 0; i < allFaceIndices.size(); ++i) {
            Index vIdx = allFaceIndices[i];
            mesh.vertices[i * 3 + 0] = vertices[vIdx].position.x;
            mesh.vertices[i * 3 + 1] = vertices[vIdx].position.y;
            mesh.vertices[i * 3 + 2] = vertices[vIdx].position.z;
        }
    }

    return mesh;
}

void SubdividedMeshCache::Unload() {
    for (Mesh& m : raylibLevels) {
        UnloadMesh(m);
    }
    raylibLevels.clear();
    heLevels.clear();
}

}

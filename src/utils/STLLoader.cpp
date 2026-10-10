#include "utils/STLLoader.h"
#include <fstream>
#include <vector>
#include <cstring>
#include <cstdint>
#include <sstream>

namespace utils {

Model LoadModelSTL(const std::string& filePath) {
    Model model = { 0 };

    std::ifstream file(filePath, std::ios::binary);
    if (!file.is_open()) {
        return model;
    }

    file.seekg(0, std::ios::end);
    size_t fileSize = static_cast<size_t>(file.tellg());
    file.seekg(0, std::ios::beg);

    if (fileSize < 84) {
        return model;
    }

    // Check if it's binary STL (80 bytes header + 4 bytes count + count * 50 bytes)
    char header[80];
    file.read(header, 80);
    uint32_t numTriangles = 0;
    file.read(reinterpret_cast<char*>(&numTriangles), 4);

    bool isBinary = false;
    if (numTriangles > 0 && fileSize >= 84 + static_cast<size_t>(numTriangles) * 50) {
        if (fileSize - (84 + static_cast<size_t>(numTriangles) * 50) < 1024) {
            isBinary = true;
        }
    }

    std::vector<float> vertices;

    if (isBinary && numTriangles > 0) {
        vertices.resize(static_cast<size_t>(numTriangles) * 9);
        for (uint32_t i = 0; i < numTriangles; ++i) {
            float normal[3];
            file.read(reinterpret_cast<char*>(normal), 12);
            file.read(reinterpret_cast<char*>(&vertices[i * 9]), 36);
            uint16_t attr;
            file.read(reinterpret_cast<char*>(&attr), 2);
        }
    } else {
        // Fallback: parse ASCII STL
        file.seekg(0, std::ios::beg);
        std::string line, token;
        while (std::getline(file, line)) {
            std::stringstream ss(line);
            ss >> token;
            for (char& c : token) c = (char)tolower(c);
            if (token == "vertex") {
                float x, y, z;
                if (ss >> x >> y >> z) {
                    vertices.push_back(x);
                    vertices.push_back(y);
                    vertices.push_back(z);
                }
            }
        }
    }

    if (vertices.empty() || vertices.size() % 9 != 0) {
        return model;
    }

    Mesh mesh = { 0 };
    mesh.vertexCount = static_cast<int>(vertices.size() / 3);
    mesh.triangleCount = static_cast<int>(vertices.size() / 9);
    mesh.vertices = static_cast<float*>(MemAlloc(vertices.size() * sizeof(float)));
    std::memcpy(mesh.vertices, vertices.data(), vertices.size() * sizeof(float));

    UploadMesh(&mesh, false);
    model = LoadModelFromMesh(mesh);

    return model;
}

} // namespace utils

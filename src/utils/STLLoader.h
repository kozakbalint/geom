#pragma once
#include <raylib.h>
#include <string>

namespace utils {

// Loads a 3D model from an STL file (both binary and ASCII formats supported)
Model LoadModelSTL(const std::string& filePath);

} // namespace utils

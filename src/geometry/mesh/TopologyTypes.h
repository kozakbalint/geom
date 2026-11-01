#pragma once

#include <cstdint>
#include <limits>
#include <functional>

namespace core {

using Index = uint32_t;
constexpr Index kInvalidIndex = std::numeric_limits<Index>::max();

struct EdgeKey {
    Index v0{ kInvalidIndex };
    Index v1{ kInvalidIndex };

    constexpr EdgeKey() = default;
    constexpr EdgeKey(Index from, Index to) : v0(from), v1(to) {}

    constexpr bool operator==(const EdgeKey& other) const noexcept {
        return v0 == other.v0 && v1 == other.v1;
    }

    static constexpr EdgeKey Undirected(Index a, Index b) noexcept {
        return (a < b) ? EdgeKey{ a, b } : EdgeKey{ b, a };
    }
};

struct EdgeHash {
    std::size_t operator()(const EdgeKey& key) const noexcept {
        return std::hash<Index>()(key.v0) ^ (std::hash<Index>()(key.v1) << 1);
    }
};

} // namespace core

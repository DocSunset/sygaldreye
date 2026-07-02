#pragma once
#include <atomic>
#include <cstdint>
#include <vector>
#include <Eigen/Core>

struct TriVertex {
    Eigen::Vector3f position;
    Eigen::Vector3f normal;
    Eigen::Vector4f color;
};

// CPU triangle-mesh payload (GL-free; render_region owns the GPU copy).
// `version` is a globally unique stamp: assigned at construction and re-stamped
// by touch() after in-place mutation. render_region re-uploads when the version
// it cached differs — and because stamps are never reused, a fresh allocation
// landing at a freed TriMeshData's address can never alias a cache entry.
struct TriMeshData {
    std::vector<TriVertex> vertices;
    std::vector<uint32_t>  indices;
    uint64_t               version = next_version();

    void touch() { version = next_version(); }

    static uint64_t next_version() {
        static std::atomic<uint64_t> counter{0};
        return ++counter;
    }
};

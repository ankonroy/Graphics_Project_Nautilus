// ============================================================================
// cave.h - underwater tunnel leading to a chamber under the volcano.
// ============================================================================
#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Cave {
    struct Segment {
        glm::vec3 a, b;       // endpoints
        float     radius;
    };
    extern std::vector<Segment> segments;
    extern glm::vec3            endpointCenter;   // dome center
    extern float                endpointRadius;

    // Build the tunnel between entrancePos (set by Island) and the endpoint
    // chamber under the volcano. Deterministic.
    void build();

    // Render cave walls, jagged rock formations, and the sunbeam cone.
    void draw();

    // Helper for HUD: is the camera inside the cave region? Used to dim
    // ambient light.
    bool isInsideCave(const glm::vec3& p);
}

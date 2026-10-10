// ============================================================================
// island.h
// ============================================================================
#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Island {
    extern glm::vec3 entrancePos;   // underwater cave mouth position
    extern float     entranceAngle; // angle (rad) relative to +X

    void build();
    void draw();

    // Horizontal radius of the island solid at the given world Y.
    // Returns 0 if Y is outside the island's vertical range.
    float radiusAtY(float y);

    // True if the given world point is inside the island's solid volume.
    bool isInsideIsland(const glm::vec3& p);
}
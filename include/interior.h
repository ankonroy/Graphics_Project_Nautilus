// ============================================================================
// interior.h - submarine control room. Built only when the camera is inside.
// ============================================================================
#pragma once

#include <glm/glm.hpp>

namespace Interior {
    // Build the static room geometry and AABB colliders.
    void build();

    // Render the room. Should be called only when camera is in interior mode
    // (main loop checks camMode).
    void draw();

    // Total interior bounds (used by camera collision).
    void bounds(glm::vec3& mn, glm::vec3& mx);
}

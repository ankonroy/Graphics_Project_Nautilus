// ============================================================================
// camera.h
// ----------------------------------------------------------------------------
// Two cameras:
//   - Chase    : behind/above the sub, looking forward.
//   - Interior : a free-fly camera inside the control room with AABB
//                collision against walls and furniture.
// Camera shake is a small time-decaying position offset applied to either
// camera after the base view is built.
// ============================================================================
#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Camera {
    // AABB used for interior collision.
    struct AABB {
        glm::vec3 min;
        glm::vec3 max;
    };

    // Reset the interior camera to the captain's seat.
    void resetInterior();

    // Build view matrix for the current mode. Returns the world-space eye
    // position via `outEye` (so fog, sonar, etc. can use it).
    glm::mat4 buildView(const glm::vec3& subPos,
                        const glm::vec3& subForward,
                        const glm::vec3& subUp,
                        glm::vec3& outEye);

    // Per-frame update: advance interior state and decay shake.
    void update(float dt);

    // Push an AABB into the interior collision list.
    void addInteriorCollider(const AABB& b);
    void clearInteriorColliders();

    // Interior camera position (for HUD/debug).
    glm::vec3 interiorPos();
    glm::vec3 interiorForward();

    // Camera shake API: add a one-shot shake.
    void addShake(float magnitude);
    glm::vec3 currentShakeOffset();

    // Set interior collision bounds (room outer AABB). Camera is clamped
    // to the inside of this volume plus the inner colliders.
    void setInteriorBounds(const AABB& room);
}

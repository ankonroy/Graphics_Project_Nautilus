// ============================================================================
// submarine.h
// ----------------------------------------------------------------------------
// Modern nuclear-style submarine: capsule hull, sail/conning tower, planes,
// propeller, rudder. All procedural.
// ============================================================================
#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Sub {
    // World state of the sub.
    extern glm::vec3 pos;
    extern float yawDeg;
    extern float pitchDeg;
    extern float propellerAngle;
    extern float boundingRadius;

    // Per-frame update: reads input, updates pos/yaw/pitch, clamps to world,
    // animates propeller.
    void update(float dt);

    // Resets the sub to a random spawn position outside the island.
    void resetToSpawn();

    // Forward / up vectors derived from yaw + pitch (right-handed, Y up).
    glm::vec3 forward();
    glm::vec3 up();
    glm::mat4 rotationMatrix();

    // Torpedo launch point in world space (just ahead of the bow).
    glm::vec3 torpedoMuzzle();

    // Draws the sub.
    void draw();
}

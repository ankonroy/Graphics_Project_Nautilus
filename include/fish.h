// ============================================================================
// fish.h - realistic-looking fish swimming in circular orbits.
// ============================================================================
#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Fish {
    struct F {
        glm::vec3 center;
        float radius, angle, speed, depth, scale, bobPhase, bobAmp;
        glm::vec3 color;       // mid color (legacy)
        glm::vec3 topColor;    // dorsal (back) color
        glm::vec3 bellyColor;  // ventral (belly) color
        float yawWiggle;       // for tail wag
    };

    extern std::vector<F> fishes;

    void spawnSchool();
    void update(float dt);
    void draw(float time);
}

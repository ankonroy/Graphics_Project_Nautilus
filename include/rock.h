// ============================================================================
// rock.h - deformed low-poly rocks scattered on the sea floor.
// ============================================================================
#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Rock {
    struct R {
        glm::vec3 pos;
        glm::vec3 scale;
        float yawDeg;
        float colorTint;   // 0..1, used to vary brown/gray
    };

    extern std::vector<R> rocks;

    // Spawn the field (deterministic, given current RNG state).
    void spawnField();

    // Render the field.
    void draw();
}

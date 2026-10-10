// ============================================================================
// mine.h - spiked moored mines.
// ============================================================================
#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Mine {
    struct M {
        glm::vec3 pos;
        float scale;
        float fixedYaw;
        bool  alive = true;
    };
    extern std::vector<M> mines;

    // Spawn scattered mines.
    void spawnField();

    // Render only alive mines.
    void draw();

    // Per-frame slow bobbing.
    void update(float dt);
}

// ============================================================================
// game.h
// ============================================================================
#pragma once

#include <glm/glm.hpp>

namespace Game {
    enum class State { Playing, Won, Lost };
    extern State state;

    void update(float dt,
                const glm::vec3& subPos,
                float subBoundingR,
                const glm::vec3& subPrevPos);

    void lose();
    void resetAll();
    bool isPlaying();

    // ---- 3-strike hull integrity ----
    int  hullStrikes();            // 0, 1, 2, or 3
    int  hullStrikesRemaining();   // 3, 2, 1, or 0
    int  hullMax();                // always returns 3
}
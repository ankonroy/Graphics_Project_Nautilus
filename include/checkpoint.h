// ============================================================================
// checkpoint.h - glowing torus rings the player must fly through.
// ============================================================================
#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Checkpoint {
    struct CP {
        glm::vec3 pos;          // ring center
        glm::vec3 axis;         // ring normal (unit)
        bool      visited = false;
        int       orderIndex;   // 0..N-1
        float     flashTime;    // for visited flash animation
    };
    extern std::vector<CP> cps;
    extern int nextToVisit;     // index of next expected checkpoint (for HUD)

    // Build randomised checkpoints. The last (5th) is always at the cave
    // endpoint. Extra mines are placed around each checkpoint.
    void build();
    void draw();
    void update(float dt, const glm::vec3& subPos, const glm::vec3& subPrevPos);
    int  visitedCount();
    bool allVisited();

    // For HUD: 0..1, fraction of checkpoints visited.
    float progress();
}

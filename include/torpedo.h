// ============================================================================
// torpedo.h - player projectiles.
// ============================================================================
#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Torpedo {
    struct T {
        glm::vec3 pos;
        glm::vec3 prevPos;
        glm::vec3 vel;
        float     traveled;
        bool      alive;
    };
    extern std::vector<T> torps;

    extern int   ammo;
    extern float cooldown;     // seconds remaining

    // Reset to a full magazine and no cooldown.
    void reset();

    // Try to fire a torpedo at the given muzzle with the given direction.
    // Returns true if a torpedo was launched.
    bool fire(const glm::vec3& muzzle, const glm::vec3& dir);

    // Per-frame update: drag, range cutoff, mine collision, blast.
    void update(float dt);

    // Draw all alive torpedoes.
    void draw();

    // Total ammo state for HUD.
    int  ammoRemaining();
    float cooldownRemaining();
}

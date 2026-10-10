// ============================================================================
// particles.h
// ----------------------------------------------------------------------------
// Lightweight particle system: list of points with velocity, life, color,
// and a kind flag (bubble vs debris vs flash). Rendered as small spheres
// (for bubbles) or tiny cubes (debris). Capped by Constants::MAX_PARTICLES
// to avoid unbounded growth.
// ============================================================================
#pragma once

#include <glm/glm.hpp>
#include <vector>

namespace Particles {
    enum class Kind { Bubble, Debris, Spark };

    struct P {
        Kind  kind;
        glm::vec3 pos;
        glm::vec3 vel;
        float life;     // seconds remaining
        float maxLife;
        float size;     // base size (scaled by life/maxLife)
        glm::vec3 color;
    };

    void init();
    void shutdown();

    // Advance all particles and reap dead ones.
    void update(float dt);

    // Spawners.
    void spawnBubble(const glm::vec3& p, const glm::vec3& vel);
    void spawnDebris(const glm::vec3& p, const glm::vec3& baseVel,
                     const glm::vec3& color, int count = 8);
    void spawnSpark (const glm::vec3& p, const glm::vec3& color);

    // Render: chooses a tiny mesh per kind.
    void render();

    // For the screen flash effect.
    glm::vec4 currentFlash();   // RGBA, A is intensity
    void       triggerFlash(float intensity = 0.8f);
}

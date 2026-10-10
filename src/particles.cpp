// ============================================================================
// particles.cpp
// ============================================================================
#include "particles.h"
#include "mesh.h"
#include "draw.h"
#include "constants.h"
#include "shader.h"
#include "rng.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    std::vector<Particles::P> g_list;
    Mesh g_sphereSmall, g_cubeSmall;
    glm::vec4 g_flash{0,0,0,0};
    float     g_flashDecay = 2.0f;   // per second
    bool      g_inited = false;
}

namespace Particles {
    void init() {
        g_list.clear();
        g_sphereSmall = Meshes::sphere(8, 12);
        g_cubeSmall   = Meshes::cube();
        g_inited = true;
    }

    void shutdown() {
        g_list.clear();
        g_inited = false;
    }

    void update(float dt) {
        // Reap dead and update.
        for (auto& p : g_list) {
            p.life -= dt;
            // Bubbles float up
            if (p.kind == Kind::Bubble) {
                p.vel.y += 0.4f * dt;
                p.vel *= std::pow(0.92f, dt * 60.0f);
            } else {
                // Gravity for debris
                p.vel.y -= 4.0f * dt;
                p.vel *= std::pow(0.5f, dt);
            }
            p.pos += p.vel * dt;
        }
        g_list.erase(std::remove_if(g_list.begin(), g_list.end(),
                                    [](const P& p) { return p.life <= 0; }),
                     g_list.end());

        if (g_flash.a > 0) {
            g_flash.a = std::max(0.0f, g_flash.a - g_flashDecay * dt);
        }
    }

    void spawnBubble(const glm::vec3& p, const glm::vec3& vel) {
        if ((int)g_list.size() >= Constants::MAX_PARTICLES) return;
        P np;
        np.kind = Kind::Bubble;
        np.pos = p; np.vel = vel;
        np.life = 1.0f; np.maxLife = 1.0f;
        np.size = 0.10f;
        np.color = glm::vec3(0.85f, 0.95f, 1.0f);
        g_list.push_back(np);
    }
    void spawnDebris(const glm::vec3& p, const glm::vec3& baseVel,
                     const glm::vec3& color, int count) {
        for (int i = 0; i < count; i++) {
            if ((int)g_list.size() >= Constants::MAX_PARTICLES) return;
            glm::vec3 v = baseVel + glm::vec3(
                (Rng::randFloat() - 0.5f) * 8.0f,
                Rng::randFloat() * 6.0f,
                (Rng::randFloat() - 0.5f) * 8.0f);
            float life = 0.8f + Rng::randFloat() * 0.6f;
            P np;
            np.kind = Kind::Debris;
            np.pos = p; np.vel = v;
            np.life = life; np.maxLife = life;
            np.size = 0.15f + Rng::randFloat() * 0.15f;
            np.color = color;
            g_list.push_back(np);
        }
    }
    void spawnSpark(const glm::vec3& p, const glm::vec3& color) {
        if ((int)g_list.size() >= Constants::MAX_PARTICLES) return;
        P np;
        np.kind = Kind::Spark;
        np.pos = p; np.vel = glm::vec3(0);
        np.life = 0.4f; np.maxLife = 0.4f;
        np.size = 0.2f;
        np.color = color;
        g_list.push_back(np);
    }

    glm::vec4 currentFlash() { return g_flash; }
    void       triggerFlash(float intensity) {
        g_flash = glm::vec4(1.0f, 0.95f, 0.85f, intensity);
    }

    void render() {
        if (!g_inited) return;
        for (auto& p : g_list) {
            float t = std::clamp(p.life / p.maxLife, 0.0f, 1.0f);
            float scale = p.size * (0.4f + 0.6f * t);
            glm::mat4 m = glm::translate(glm::mat4(1.0f), p.pos);
            m = glm::scale(m, glm::vec3(scale));
            if (p.kind == Kind::Bubble) {
                Draw::modelMaterial(m, p.color, 32.0f, 0.3f);
                Draw::mesh(g_sphereSmall);
            } else {
                Draw::modelMaterial(m, p.color, 8.0f, 0.6f * t);
                Draw::mesh(g_cubeSmall);
            }
        }
    }
}

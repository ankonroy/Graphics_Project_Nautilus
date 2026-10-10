// ============================================================================
// torpedo.cpp
// ============================================================================
#include "torpedo.h"
#include "mine.h"
#include "mesh.h"
#include "draw.h"
#include "particles.h"
#include "constants.h"
#include "shader.h"
#include "rng.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    Mesh g_torp;
    bool inited = false;
    void build() {
        if (inited) return;
        // Elongated capsule for the body.
        std::vector<glm::vec2> prof = {
            {-0.6f, 0.10f}, {-0.4f, 0.18f}, {0.0f, 0.20f},
            { 0.4f, 0.18f}, { 0.6f, 0.10f}, { 0.6f, 0.0f},
        };
        g_torp = Meshes::latheBody(prof, 16);
        inited = true;
    }

    // Test if a segment from A to B hits a sphere at C with radius r.
    // If hit, returns true and the t parameter (along A->B).
    bool segmentSphere(const glm::vec3& A, const glm::vec3& B,
                       const glm::vec3& C, float r, float& tOut) {
        glm::vec3 m = B - A;
        glm::vec3 ac = A - C;
        float a = glm::dot(m, m);
        if (a < 1e-6f) {
            tOut = 0;
            return glm::dot(ac, ac) <= r * r;
        }
        float b = 2.0f * glm::dot(m, ac);
        float c = glm::dot(ac, ac) - r * r;
        float disc = b * b - 4.0f * a * c;
        if (disc < 0) return false;
        float sq = std::sqrt(disc);
        float t  = (-b - sq) / (2.0f * a);
        if (t < 0) t = (-b + sq) / (2.0f * a);
        if (t < 0 || t > 1) return false;
        tOut = t;
        return true;
    }

    void explodeAt(const glm::vec3& p) {
        Particles::triggerFlash(0.6f);
        Particles::spawnDebris(p, glm::vec3(0), glm::vec3(0.8f, 0.4f, 0.1f), 12);
        Particles::spawnSpark(p, glm::vec3(1.0f, 0.9f, 0.4f));
    }
}

namespace Torpedo {
    std::vector<T> torps;
    int   ammo   = Constants::TORPEDO_MAX;
    float cooldown = 0.0f;

    void reset() {
        torps.clear();
        ammo = Constants::TORPEDO_MAX;
        cooldown = 0;
    }

    bool fire(const glm::vec3& muzzle, const glm::vec3& dir) {
        if (ammo <= 0 || cooldown > 0) return false;
        T t;
        t.pos = muzzle;
        t.prevPos = muzzle;
        t.vel = glm::normalize(dir) * Constants::TORPEDO_SPEED;
        t.traveled = 0;
        t.alive = true;
        torps.push_back(t);
        ammo--;
        cooldown = Constants::TORPEDO_COOLDOWN;
        return true;
    }

    void update(float dt) {
        if (cooldown > 0) cooldown = std::max(0.0f, cooldown - dt);

        for (auto& t : torps) {
            if (!t.alive) continue;
            t.prevPos = t.pos;
            // Drag: velocity *= drag^dt
            t.vel *= std::pow(Constants::TORPEDO_DRAG, dt);
            t.pos += t.vel * dt;
            t.traveled += glm::length(t.vel * dt);

            // Bubble trail
            Particles::spawnBubble(t.pos, glm::vec3(
                (Rng::randFloat() - 0.5f) * 0.3f, 0.2f,
                (Rng::randFloat() - 0.5f) * 0.3f));

            // Range cutoff
            if (t.traveled > Constants::TORPEDO_RANGE) {
                t.alive = false;
            }
            // Mine collisions
            for (auto& m : Mine::mines) {
                if (!m.alive) continue;
                float hitT;
                if (segmentSphere(t.prevPos, t.pos, m.pos,
                                  Constants::MINE_HIT_R, hitT)) {
                    t.alive = false;
                    glm::vec3 impact = t.prevPos + (t.pos - t.prevPos) * hitT;
                    explodeAt(impact);
                    // Destroy the hit mine plus any in blast radius.
                    for (auto& m2 : Mine::mines) {
                        if (!m2.alive) continue;
                        if (glm::length(m2.pos - impact) < Constants::TORPEDO_BLAST) {
                            m2.alive = false;
                            explodeAt(m2.pos);
                        }
                    }
                    break;
                }
            }
        }
        torps.erase(std::remove_if(torps.begin(), torps.end(),
                                   [](const T& t){ return !t.alive; }),
                    torps.end());
    }

    void draw() {
        if (!inited) build();
        for (auto& t : torps) {
            glm::vec3 fwd = glm::normalize(t.vel);
            glm::mat4 m = glm::translate(glm::mat4(1.0f), t.pos);
            // Orient capsule along velocity.
            if (glm::length(fwd) > 0.001f) {
                m = m * Draw::alignLocalY(-fwd);
            }
            m = glm::scale(m, glm::vec3(1.4f));
            Draw::modelMaterial(m, glm::vec3(0.95f, 0.85f, 0.30f), 64.0f, 0.5f);
            Draw::mesh(g_torp);
        }
    }

    int   ammoRemaining()    { return ammo; }
    float cooldownRemaining() { return cooldown; }
}

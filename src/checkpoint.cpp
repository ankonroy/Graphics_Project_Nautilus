// ============================================================================
// checkpoint.cpp
// ----------------------------------------------------------------------------
// A flag on a pole for each checkpoint. Yellow when pending, green when
// touched. Any order. The 5th flag is always inside the cave.
// ============================================================================
#include "checkpoint.h"
#include "cave.h"
#include "mine.h"
#include "mesh.h"
#include "draw.h"
#include "constants.h"
#include "rng.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    Mesh g_pole, g_flag, g_base;
    bool inited = false;

    const float POLE_HEIGHT  = 6.0f;
    const float TOUCH_RADIUS = 4.5f;

    void buildMeshes() {
        if (inited) return;
        g_pole = Meshes::cylinder(12);
        g_flag = Meshes::cube();
        g_base = Meshes::sphere(10, 14);
        inited = true;
    }
}

namespace Checkpoint {
    std::vector<CP> cps;
    int nextToVisit = 0;

    void build() {
        buildMeshes();
        cps.clear();

        const int N = Constants::CHECKPOINT_COUNT;

        // World flags outside the island footprint.
        std::vector<glm::vec3> positions;
        for (int i = 0; i < N - 1; i++) {
            float a = Rng::randRange(0.0f, 2.0f * (float)M_PI);
            float r = Rng::randRange(Constants::ISLAND_FOOTPRINT + 10.0f,
                                     Constants::WORLD_RADIUS - 15.0f);
            positions.push_back(glm::vec3(r * cosf(a),
                                          Constants::FLOOR_Y,
                                          r * sinf(a)));
        }
        // Cave flag at the chamber.
        positions.push_back(Cave::endpointCenter);

        cps.resize(N);
        for (int i = 0; i < N; i++) {
            cps[i].orderIndex = i;
            cps[i].pos        = positions[i];
            cps[i].axis       = glm::vec3(0, 1, 0);
            cps[i].visited    = false;
            cps[i].flashTime  = 0.0f;
        }
        nextToVisit = 0;

        // Mines around each flag.
        for (auto& cp : cps) {
            int nMines = 3 + Rng::randInt(0, 2);
            for (int i = 0; i < nMines; i++) {
                float ang = (float)i / nMines * 2.0f * (float)M_PI
                            + Rng::randRange(0.0f, 0.5f);
                float r   = 6.5f + Rng::randRange(-1.0f, 1.5f);
                Mine::M m;
                m.pos      = cp.pos + glm::vec3(cosf(ang) * r, 2.0f,
                                                sinf(ang) * r);
                m.scale    = 0.9f + Rng::randFloat() * 0.4f;
                m.fixedYaw = Rng::randRange(0.0f, 360.0f);
                m.alive    = true;
                Mine::mines.push_back(m);
            }
        }
    }

    void draw() {
        if (!inited) buildMeshes();

        const glm::vec3 poleCol (0.35f, 0.35f, 0.38f);
        const glm::vec3 baseCol (0.20f, 0.20f, 0.22f);

        for (auto& cp : cps) {
            glm::vec3 flagCol = cp.visited
                ? glm::vec3(0.10f, 1.00f, 0.25f)
                : glm::vec3(1.00f, 0.85f, 0.10f);

            {
                glm::mat4 m = glm::translate(glm::mat4(1.0f), cp.pos);
                m = glm::scale(m, glm::vec3(0.6f, 0.30f, 0.6f));
                Draw::modelMaterial(m, baseCol, 16.0f);
                Draw::mesh(g_base);
            }
            {
                glm::mat4 m = glm::translate(
                    glm::mat4(1.0f),
                    cp.pos + glm::vec3(0.0f, POLE_HEIGHT * 0.5f, 0.0f));
                m = glm::scale(m, glm::vec3(0.08f, POLE_HEIGHT * 0.5f, 0.08f));
                Draw::modelMaterial(m, poleCol, 64.0f);
                Draw::mesh(g_pole);
            }
            {
                glm::mat4 m = glm::translate(
                    glm::mat4(1.0f),
                    cp.pos + glm::vec3(0.85f, POLE_HEIGHT - 0.8f, 0.0f));
                m = glm::scale(m, glm::vec3(1.7f, 1.2f, 0.04f));
                float em = cp.visited
                    ? 1.0f
                    : (0.40f + 0.20f * sinf(cp.flashTime * 5.0f));
                Draw::modelMaterial(m, flagCol, 16.0f, em);
                Draw::mesh(g_flag);
            }
        }
    }

    void update(float dt, const glm::vec3& subPos, const glm::vec3&) {
        for (auto& cp : cps) cp.flashTime += dt;

        for (auto& cp : cps) {
            if (cp.visited) continue;
            glm::vec3 a = cp.pos;
            glm::vec3 b = cp.pos + glm::vec3(0.0f, POLE_HEIGHT, 0.0f);
            glm::vec3 d = b - a;
            float len2 = glm::dot(d, d);
            float t = glm::clamp(glm::dot(subPos - a, d) / len2, 0.0f, 1.0f);
            glm::vec3 closest = a + d * t;
            float dist = glm::length(subPos - closest);
            if (dist < TOUCH_RADIUS) cp.visited = true;
        }
        nextToVisit = visitedCount();
    }

    int visitedCount() {
        int c = 0;
        for (auto& cp : cps) if (cp.visited) c++;
        return c;
    }
    bool  allVisited() { return visitedCount() == (int)cps.size(); }
    float progress()   { return (float)visitedCount() / (float)cps.size(); }
}
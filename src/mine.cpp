// ============================================================================
// mine.cpp
// ============================================================================
#include "mine.h"
#include "mesh.h"
#include "draw.h"
#include "constants.h"
#include "rng.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    Mesh g_sphere, g_spike;
    bool inited = false;
    void build() {
        if (inited) return;
        g_sphere = Meshes::sphere(12, 18);
        g_spike  = Meshes::cone(8);
        inited = true;
    }
}

namespace Mine {
    std::vector<M> mines;

    void spawnField() {
        build();
        mines.clear();
        const int COUNT = 40;
        for (int i = 0; i < COUNT; i++) {
            float a = Rng::randRange(0.0f, 2.0f * (float)M_PI);
            // Outside the island footprint.
            float r = Rng::randRange(Constants::ISLAND_FOOTPRINT + 5.0f,
                                     Constants::WORLD_RADIUS - 10.0f);
            float y = Rng::randRange(Constants::FLOOR_Y + 2.0f,
                                     Constants::WATER_LEVEL - 1.0f);
            M m;
            m.pos      = glm::vec3(r * cosf(a), y, r * sinf(a));
            m.scale    = 1.0f + Rng::randFloat() * 0.3f;
            m.fixedYaw = Rng::randRange(0.0f, 360.0f);
            m.alive    = true;
            mines.push_back(m);
        }
    }

    void update(float dt) { (void)dt; }

    void draw() {
        if (!inited) build();
        glm::vec3 bodyColor (0.90f, 0.15f, 0.15f);
        glm::vec3 spikeCol  (0.35f, 0.10f, 0.10f);
        for (auto& m : mines) {
            if (!m.alive) continue;
            glm::mat4 base = glm::translate(glm::mat4(1.0f), m.pos);
            base = glm::rotate(base, glm::radians(m.fixedYaw), glm::vec3(0, 1, 0));
            base = glm::scale(base, glm::vec3(m.scale));
            {
                glm::mat4 mm = glm::scale(base, glm::vec3(1.3f));
                Draw::modelMaterial(mm, bodyColor, 96.0f);
                Draw::mesh(g_sphere);
            }
            glm::vec3 dirs[] = {
                { 1, 0, 0}, {-1, 0, 0},
                { 0, 1, 0}, { 0,-1, 0},
                { 0, 0, 1}, { 0, 0,-1},
                { 0.707f,  0.707f, 0}, {-0.707f,  0.707f, 0},
                { 0.707f, -0.707f, 0}, {-0.707f, -0.707f, 0},
            };
            for (auto& d : dirs) {
                glm::vec3 nd = glm::normalize(d);
                glm::mat4 mm = base;
                mm = glm::translate(mm, nd * 0.55f);
                mm = mm * Draw::alignLocalY(nd);
                mm = glm::scale(mm, glm::vec3(0.18f, 0.75f, 0.18f));
                Draw::modelMaterial(mm, spikeCol, 64.0f);
                Draw::mesh(g_spike);
            }
        }
    }
}
// ============================================================================
// rock.cpp
// ============================================================================
#include "rock.h"
#include "mesh.h"
#include "draw.h"
#include "rng.h"
#include "texture.h"
#include "constants.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    std::vector<Mesh> g_rockVariants;
    bool inited = false;

    void build() {
        if (inited) return;
        g_rockVariants.push_back(Meshes::rock(1, 0.22f));
        g_rockVariants.push_back(Meshes::rock(1, 0.16f));
        g_rockVariants.push_back(Meshes::rock(2, 0.12f));
        g_rockVariants.push_back(Meshes::rock(1, 0.28f));
        inited = true;
    }
}

namespace Rock {
    std::vector<R> rocks;

    void spawnField() {
        build();
        rocks.clear();
        const int COUNT = 140;
        for (int i = 0; i < COUNT; i++) {
            float a = Rng::randRange(0.0f, 2.0f * (float)M_PI);
            // Spawn OUTSIDE the island footprint so nothing is buried.
            float r = Rng::randRange(Constants::ISLAND_FOOTPRINT,
                                     Constants::WORLD_RADIUS - 4.0f);
            R rk;
            rk.pos = glm::vec3(r * cosf(a),
                               Constants::FLOOR_Y + Rng::randRange(0.2f, 0.7f),
                               r * sinf(a));
            float s = Rng::randRange(0.6f, 2.4f);
            rk.scale = glm::vec3(s * Rng::randRange(0.7f, 1.3f),
                                 s * Rng::randRange(0.5f, 1.1f),
                                 s * Rng::randRange(0.7f, 1.3f));
            rk.yawDeg = Rng::randRange(0.0f, 360.0f);
            rk.colorTint = Rng::randFloat();
            rocks.push_back(rk);
        }
    }

    void draw() {
        if (!inited) build();
        GLuint rockTex = Textures::rockTex();
        for (size_t i = 0; i < rocks.size(); i++) {
            const R& r = rocks[i];
            glm::mat4 m = glm::translate(glm::mat4(1.0f), r.pos);
            m = glm::rotate(m, glm::radians(r.yawDeg), glm::vec3(0, 1, 0));
            m = glm::scale(m, r.scale);
            float k = r.colorTint;
            glm::vec3 col = glm::mix(glm::vec3(0.55f, 0.45f, 0.32f),
                                     glm::vec3(0.45f, 0.45f, 0.42f), k);
            Draw::modelMaterial(m, col, 8.0f, 0.0f, rockTex, true);
            Draw::mesh(g_rockVariants[i % g_rockVariants.size()]);
        }
    }
}
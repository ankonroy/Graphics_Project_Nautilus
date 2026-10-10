// ============================================================================
// fish.cpp
// ----------------------------------------------------------------------------
// A simple fish: elongated ellipsoid body, one tail fin (apex touching the
// body, edge pointing away), one dorsal fin, two side fins, two eyes.
//
// All fish swim in slow circles far from the island.
// ============================================================================
#include "fish.h"
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
    Mesh g_body;    // ellipsoid
    Mesh g_fin;     // flat triangle (apex at +Y, base at -Y in local space)
    Mesh g_eye;     // tiny sphere
    bool inited = false;

    void build() {
        if (inited) return;
        g_body = Meshes::ellipsoid(0.5f, 0.22f, 0.20f, 20, 14);
        g_fin  = Meshes::triangle();
        g_eye  = Meshes::sphere(6, 8);
        inited = true;
    }
}

namespace Fish {
    std::vector<F> fishes;

    void spawnSchool() {
        build();
        fishes.clear();

        struct Palette { glm::vec3 top, mid, belly; };
        Palette palettes[5] = {
            { glm::vec3(0.95f, 0.60f, 0.05f), glm::vec3(1.00f, 0.75f, 0.15f),
              glm::vec3(0.98f, 0.85f, 0.50f) },
            { glm::vec3(0.90f, 0.15f, 0.15f), glm::vec3(1.00f, 0.30f, 0.25f),
              glm::vec3(0.95f, 0.60f, 0.55f) },
            { glm::vec3(0.10f, 0.35f, 0.85f), glm::vec3(0.25f, 0.55f, 0.95f),
              glm::vec3(0.65f, 0.80f, 1.00f) },
            { glm::vec3(0.15f, 0.65f, 0.25f), glm::vec3(0.30f, 0.80f, 0.35f),
              glm::vec3(0.70f, 0.90f, 0.65f) },
            { glm::vec3(0.75f, 0.15f, 0.65f), glm::vec3(0.90f, 0.35f, 0.80f),
              glm::vec3(0.95f, 0.70f, 0.90f) },
        };

        const int N_SCHOOLS = 6;
        for (int s = 0; s < N_SCHOOLS; s++) {
            float angle = s * (2.0f * (float)M_PI / N_SCHOOLS);
            float ringR = Constants::ISLAND_FOOTPRINT + 25.0f
                        + (s % 3) * 12.0f;
            glm::vec3 center(ringR * cosf(angle), 0.0f,
                             ringR * sinf(angle));
            Palette p = palettes[s % 5];
            for (int i = 0; i < 5; i++) {
                F f;
                f.center   = center + glm::vec3(
                    Rng::randRange(-4.0f, 4.0f),
                    0.0f,
                    Rng::randRange(-4.0f, 4.0f));
                f.radius   = Rng::randRange(3.0f, 6.0f);
                f.angle    = Rng::randRange(0.0f, 6.28f);
                f.speed    = Rng::randRange(0.35f, 0.7f)
                             * (s % 2 ? 1 : -1);
                f.depth    = Rng::randRange(Constants::FLOOR_Y + 5.0f,
                                            Constants::WATER_LEVEL - 3.0f);
                f.scale    = Rng::randRange(0.9f, 1.3f);
                f.bobPhase = Rng::randRange(0.0f, 6.28f);
                f.bobAmp   = 0.3f;
                f.color      = p.mid;
                f.topColor   = p.top;
                f.bellyColor = p.belly;
                f.yawWiggle  = Rng::randRange(0.0f, 6.28f);
                fishes.push_back(f);
            }
        }
    }

    void update(float dt) {
        for (auto& f : fishes) {
            f.angle += f.speed * dt;
            if (f.angle >  2.0f * (float)M_PI) f.angle -= 2.0f * (float)M_PI;
            if (f.angle < -2.0f * (float)M_PI) f.angle += 2.0f * (float)M_PI;
        }
    }

    void draw(float time) {
        if (!inited) build();

        for (auto& f : fishes) {
            float cx = f.center.x + f.radius * cosf(f.angle);
            float cz = f.center.z + f.radius * sinf(f.angle);
            float cy = f.depth + f.bobAmp * sinf(time * 1.4f + f.bobPhase);

            float dx = -sinf(f.angle);
            float dz =  cosf(f.angle);
            float yaw = atan2f(dz, dx);

            glm::mat4 base = glm::translate(glm::mat4(1.0f),
                                            glm::vec3(cx, cy, cz));
            base = glm::rotate(base, -yaw, glm::vec3(0, 1, 0));
            base = glm::scale(base, glm::vec3(f.scale));

            // Body
            {
                glm::mat4 m = glm::scale(base, glm::vec3(1.4f, 1.0f, 1.0f));
                Draw::modelMaterial(m, f.color, 40.0f, 0.08f);
                Draw::mesh(g_body);
            }
            // Top stripe
            {
                glm::mat4 m = glm::translate(base,
                                             glm::vec3(0.0f, 0.10f, 0.0f));
                m = glm::scale(m, glm::vec3(1.2f, 0.35f, 0.5f));
                Draw::modelMaterial(m, f.topColor, 32.0f, 0.05f);
                Draw::mesh(g_body);
            }
            // Belly
            {
                glm::mat4 m = glm::translate(base,
                                             glm::vec3(0.0f, -0.08f, 0.0f));
                m = glm::scale(m, glm::vec3(1.2f, 0.35f, 0.5f));
                Draw::modelMaterial(m, f.bellyColor, 32.0f, 0.05f);
                Draw::mesh(g_body);
            }
            // TAIL FIN — apex (corner) touches the body, base (edge) faces away
            {
                float wag = sinf(time * 6.0f + f.yawWiggle) * 25.0f;
                glm::mat4 m = glm::translate(base,
                                             glm::vec3(-0.85f, 0.0f, 0.0f));
                // -90 deg rotation: apex (+Y in triangle-local) → +X
                // (toward body). The base edge points to -X (away).
                m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(0, 0, 1));
                m = glm::rotate(m, glm::radians(wag),      glm::vec3(0, 1, 0));
                m = glm::scale(m, glm::vec3(0.45f, 0.45f, 0.45f));
                Draw::modelMaterial(m, f.topColor, 16.0f, 0.10f);
                Draw::mesh(g_fin);
            }
            // Dorsal fin
            {
                glm::mat4 m = glm::translate(base,
                                             glm::vec3(-0.05f, 0.18f, 0.0f));
                m = glm::scale(m, glm::vec3(0.35f, 0.35f, 0.4f));
                Draw::modelMaterial(m, f.topColor, 16.0f, 0.10f);
                Draw::mesh(g_fin);
            }
            // Side fins
            for (int side : {-1, 1}) {
                float wobble = sinf(time * 4.0f + f.yawWiggle + side * 1.5f)
                               * 15.0f;
                glm::mat4 m = glm::translate(base,
                                             glm::vec3(-0.15f, -0.05f,
                                                       side * 0.18f));
                m = glm::rotate(m, glm::radians(90.0f), glm::vec3(1, 0, 0));
                m = glm::rotate(m, glm::radians(wobble), glm::vec3(0, 1, 0));
                m = glm::scale(m, glm::vec3(0.25f, 0.25f, 0.25f));
                Draw::modelMaterial(m, f.bellyColor, 12.0f, 0.10f);
                Draw::mesh(g_fin);
            }
            // Eyes
            for (int side : {-1, 1}) {
                glm::mat4 m = glm::translate(base,
                                             glm::vec3(0.55f, 0.05f,
                                                       side * 0.13f));
                m = glm::scale(m, glm::vec3(0.10f));
                Draw::modelMaterial(m, glm::vec3(0.02f), 64.0f, 0.0f);
                Draw::mesh(g_eye);
            }
        }
    }
}
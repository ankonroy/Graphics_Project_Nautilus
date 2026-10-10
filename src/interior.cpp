// ============================================================================
// interior.cpp
// ----------------------------------------------------------------------------
// A submarine control room: closed rectangular room with a front window,
// a sloped dashboard, a captain's chair, side consoles, an overhead panel,
// and a periscope. Everything is procedural and built from simple cubes,
// cylinders, and spheres.
//
// Interior volume:  X ∈ [-1.2, +1.2]
//                   Y ∈ [ 0.0, +1.8]
//                   Z ∈ [-2.0, +2.0]
// ============================================================================
#include "interior.h"
#include "mesh.h"
#include "draw.h"
#include "camera.h"
#include "submarine.h"
#include "constants.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    Mesh g_box;
    Mesh g_cylinder;
    Mesh g_sphere;
    bool inited = false;

    constexpr float IN_HALF_W = 1.2f;
    constexpr float IN_H      = 1.8f;
    constexpr float IN_HALF_D = 2.0f;

    constexpr float WALL_T = 0.3f;

    constexpr float OUT_HALF_W = IN_HALF_W + WALL_T;
    constexpr float OUT_HALF_D = IN_HALF_D + WALL_T;
    constexpr float OUT_TOP    = IN_H      + WALL_T;
    constexpr float OUT_BOT    =            -WALL_T;

    // ---- Window — raised by 0.15 to sit at eye level ----
    constexpr float WIN_HALF_X = 0.7f;
    constexpr float WIN_BOT    = 0.85f;   // was 0.70
    constexpr float WIN_TOP    = 1.45f;   // was 1.30

    const glm::vec3 COL_FLOOR    (0.28f, 0.30f, 0.34f);
    const glm::vec3 COL_CEIL     (0.62f, 0.65f, 0.70f);
    const glm::vec3 COL_WALL     (0.48f, 0.52f, 0.58f);
    const glm::vec3 COL_BACK     (0.40f, 0.44f, 0.50f);
    const glm::vec3 COL_CONSOLE  (0.14f, 0.16f, 0.19f);
    const glm::vec3 COL_CHAIR    (0.45f, 0.26f, 0.12f);
    const glm::vec3 COL_CHAIR_DK (0.32f, 0.18f, 0.08f);
    const glm::vec3 COL_METAL    (0.35f, 0.38f, 0.42f);
    const glm::vec3 COL_ACCENT   (0.55f, 0.58f, 0.62f);
    const glm::vec3 COL_DIAL_Y   (0.95f, 0.82f, 0.20f);
    const glm::vec3 COL_DIAL_R   (0.85f, 0.20f, 0.20f);
    const glm::vec3 COL_DIAL_G   (0.20f, 0.85f, 0.35f);
    const glm::vec3 COL_SCREEN   (0.15f, 0.50f, 0.75f);
    const glm::vec3 COL_LIGHT    (1.00f, 0.95f, 0.75f);

    void panel(const glm::mat4& base,
               glm::vec3 centre, glm::vec3 halfSize,
               const glm::vec3& color,
               float emissive = 0.35f,
               float shininess = 10.0f) {
        glm::mat4 m = base
                    * glm::translate(glm::mat4(1.0f), centre)
                    * glm::scale(glm::mat4(1.0f), halfSize * 2.0f);
        Draw::modelMaterial(m, color, shininess, emissive);
        Draw::mesh(g_box);
    }

    void cyl(const glm::mat4& base,
             glm::vec3 centre, glm::vec3 halfSize,
             const glm::vec3& color,
             float emissive = 0.35f,
             float shininess = 32.0f) {
        glm::mat4 m = base
                    * glm::translate(glm::mat4(1.0f), centre)
                    * glm::scale(glm::mat4(1.0f), halfSize * 2.0f);
        Draw::modelMaterial(m, color, shininess, emissive);
        Draw::mesh(g_cylinder);
    }

    void ball(const glm::mat4& base,
              glm::vec3 centre, float radius,
              const glm::vec3& color,
              float emissive = 0.5f) {
        glm::mat4 m = base
                    * glm::translate(glm::mat4(1.0f), centre)
                    * glm::scale(glm::mat4(1.0f), glm::vec3(radius * 2.0f));
        Draw::modelMaterial(m, color, 64.0f, emissive);
        Draw::mesh(g_sphere);
    }
}

namespace Interior {
    void build() {
        if (inited) return;
        g_box      = Meshes::cube();
        g_cylinder = Meshes::cylinder(16);
        g_sphere   = Meshes::sphere(10, 14);

        Camera::setInteriorBounds({
            glm::vec3(-IN_HALF_W, 0.0f, -IN_HALF_D),
            glm::vec3( IN_HALF_W, IN_H,  IN_HALF_D)
        });

        // Colliders
        Camera::addInteriorCollider({
            glm::vec3(-1.00f, 0.35f, -1.60f),
            glm::vec3( 1.00f, 0.90f, -1.10f)
        });
        Camera::addInteriorCollider({
            glm::vec3(-0.35f, 0.30f, 0.65f),
            glm::vec3( 0.35f, 0.95f, 1.35f)
        });
        Camera::addInteriorCollider({
            glm::vec3(-1.20f, 0.50f, -1.20f),
            glm::vec3(-0.90f, 0.90f,  1.20f)
        });
        Camera::addInteriorCollider({
            glm::vec3( 0.90f, 0.50f, -1.20f),
            glm::vec3( 1.20f, 0.90f,  1.20f)
        });

        std::cerr << "[Interior] built control room, volume = "
                  << "x[-" << IN_HALF_W << "," << IN_HALF_W << "] "
                  << "y[0," << IN_H      << "] "
                  << "z[-" << IN_HALF_D << "," << IN_HALF_D << "]\n";
        inited = true;
    }

    void bounds(glm::vec3& mn, glm::vec3& mx) {
        mn = glm::vec3(-IN_HALF_W, 0.0f, -IN_HALF_D);
        mx = glm::vec3( IN_HALF_W, IN_H,  IN_HALF_D);
    }

    void draw() {
        if (!inited) build();

        glm::mat4 base = glm::translate(glm::mat4(1.0f), Sub::pos);
        base = glm::rotate(base, glm::radians(Sub::yawDeg),   glm::vec3(0,1,0));
        base = glm::rotate(base, glm::radians(Sub::pitchDeg), glm::vec3(1,0,0));

        // Room shell
        panel(base,
              glm::vec3(0.0f, -WALL_T * 0.5f, 0.0f),
              glm::vec3(OUT_HALF_W, WALL_T * 0.5f, OUT_HALF_D),
              COL_FLOOR, 0.20f);
        panel(base,
              glm::vec3(0.0f, IN_H + WALL_T * 0.5f, 0.0f),
              glm::vec3(OUT_HALF_W, WALL_T * 0.5f, OUT_HALF_D),
              COL_CEIL, 0.30f);
        panel(base,
              glm::vec3(0.0f,
                        (OUT_TOP + OUT_BOT) * 0.5f,
                        IN_HALF_D + WALL_T * 0.5f),
              glm::vec3(OUT_HALF_W,
                        (OUT_TOP - OUT_BOT) * 0.5f,
                        WALL_T * 0.5f),
              COL_BACK);
        panel(base,
              glm::vec3(-IN_HALF_W - WALL_T * 0.5f,
                        (OUT_TOP + OUT_BOT) * 0.5f, 0.0f),
              glm::vec3(WALL_T * 0.5f,
                        (OUT_TOP - OUT_BOT) * 0.5f,
                        OUT_HALF_D),
              COL_WALL);
        panel(base,
              glm::vec3( IN_HALF_W + WALL_T * 0.5f,
                        (OUT_TOP + OUT_BOT) * 0.5f, 0.0f),
              glm::vec3(WALL_T * 0.5f,
                        (OUT_TOP - OUT_BOT) * 0.5f,
                        OUT_HALF_D),
              COL_WALL);

        // Front wall around window
        const float fz = -IN_HALF_D - WALL_T * 0.5f;
        panel(base,
              glm::vec3(0.0f, (OUT_BOT + WIN_BOT) * 0.5f, fz),
              glm::vec3(OUT_HALF_W,
                        (WIN_BOT - OUT_BOT) * 0.5f,
                        WALL_T * 0.5f),
              COL_WALL);
        panel(base,
              glm::vec3(0.0f, (WIN_TOP + OUT_TOP) * 0.5f, fz),
              glm::vec3(OUT_HALF_W,
                        (OUT_TOP - WIN_TOP) * 0.5f,
                        WALL_T * 0.5f),
              COL_WALL);
        panel(base,
              glm::vec3(-(OUT_HALF_W + WIN_HALF_X) * 0.5f,
                        (WIN_BOT + WIN_TOP) * 0.5f,
                        fz),
              glm::vec3((OUT_HALF_W - WIN_HALF_X) * 0.5f,
                        (WIN_TOP - WIN_BOT) * 0.5f,
                        WALL_T * 0.5f),
              COL_WALL);
        panel(base,
              glm::vec3( (OUT_HALF_W + WIN_HALF_X) * 0.5f,
                        (WIN_BOT + WIN_TOP) * 0.5f,
                        fz),
              glm::vec3((OUT_HALF_W - WIN_HALF_X) * 0.5f,
                        (WIN_TOP - WIN_BOT) * 0.5f,
                        WALL_T * 0.5f),
              COL_WALL);

        // Ceiling lights
        for (int iz = -1; iz <= 1; iz += 2) {
            for (int ix = -1; ix <= 1; ix += 2) {
                panel(base,
                      glm::vec3(ix * 0.6f, IN_H - 0.03f, iz * 1.1f),
                      glm::vec3(0.20f, 0.02f, 0.20f),
                      COL_LIGHT, 1.0f, 1.0f);
            }
        }

        // Front dashboard
        panel(base,
              glm::vec3(0.0f, 0.60f, -1.35f),
              glm::vec3(1.00f, 0.25f, 0.25f),
              COL_CONSOLE, 0.30f);

        // Sloped top
        {
            glm::mat4 m = base
                        * glm::translate(glm::mat4(1.0f),
                                         glm::vec3(0.0f, 0.88f, -1.32f))
                        * glm::rotate(glm::mat4(1.0f),
                                      glm::radians(20.0f),
                                      glm::vec3(1, 0, 0))
                        * glm::scale(glm::mat4(1.0f),
                                     glm::vec3(1.00f * 2.0f,
                                               0.02f * 2.0f,
                                               0.30f * 2.0f));
            Draw::modelMaterial(m, COL_CONSOLE, 40.0f, 0.30f);
            Draw::mesh(g_box);
        }

        // 5 gauges
        for (int i = 0; i < 5; i++) {
            float x = -0.6f + i * 0.3f;
            glm::vec3 col = (i == 0) ? COL_DIAL_R
                          : (i == 1) ? COL_DIAL_G
                          : (i == 2) ? COL_DIAL_Y
                          : (i == 3) ? COL_DIAL_G
                                     : COL_DIAL_R;
            cyl(base,
                glm::vec3(x, 0.95f, -1.25f),
                glm::vec3(0.06f, 0.02f, 0.06f),
                col, 0.8f);
        }

        // Sonar screen
        panel(base,
              glm::vec3(0.0f, 0.92f, -1.12f),
              glm::vec3(0.25f, 0.06f, 0.02f),
              COL_SCREEN, 1.0f, 64.0f);

        // Side consoles
        for (int side : {-1, 1}) {
            panel(base,
                  glm::vec3(side * 1.05f, 0.65f, 0.0f),
                  glm::vec3(0.15f, 0.15f, 1.20f),
                  COL_CONSOLE, 0.30f);
            panel(base,
                  glm::vec3(side * 1.05f, 0.80f, 0.0f),
                  glm::vec3(0.15f, 0.02f, 1.20f),
                  COL_ACCENT, 0.40f);
            for (int i = 0; i < 4; i++) {
                float z = -1.0f + i * 0.7f;
                glm::vec3 col = (i % 2 == 0) ? COL_DIAL_G : COL_DIAL_R;
                ball(base,
                     glm::vec3(side * 1.05f, 0.84f, z),
                     0.04f, col, 0.8f);
            }
        }

        // Overhead panel
        panel(base,
              glm::vec3(0.0f, 1.60f, -1.30f),
              glm::vec3(0.70f, 0.04f, 0.22f),
              COL_CONSOLE, 0.30f);
        for (int i = 0; i < 5; i++) {
            float x = -0.4f + i * 0.2f;
            glm::vec3 col = (i % 3 == 0) ? COL_DIAL_Y
                          : (i % 3 == 1) ? COL_DIAL_R
                                         : COL_DIAL_G;
            ball(base,
                 glm::vec3(x, 1.55f, -1.30f),
                 0.03f, col, 0.8f);
        }

        // Chair
        cyl(base,
            glm::vec3(0.0f, 0.20f, 1.0f),
            glm::vec3(0.06f, 0.20f, 0.06f),
            COL_METAL, 0.30f);
        panel(base,
              glm::vec3(0.0f, 0.45f, 1.0f),
              glm::vec3(0.35f, 0.08f, 0.35f),
              COL_CHAIR, 0.30f);
        panel(base,
              glm::vec3(0.0f, 0.85f, 1.30f),
              glm::vec3(0.35f, 0.35f, 0.06f),
              COL_CHAIR_DK, 0.30f);
        for (int side : {-1, 1}) {
            panel(base,
                  glm::vec3(side * 0.38f, 0.65f, 1.0f),
                  glm::vec3(0.04f, 0.03f, 0.30f),
                  COL_CHAIR_DK, 0.30f);
        }

        // Periscope
        cyl(base,
            glm::vec3(0.75f, 1.40f, 0.30f),
            glm::vec3(0.05f, 0.40f, 0.05f),
            COL_METAL, 0.35f, 64.0f);
        panel(base,
              glm::vec3(0.75f, 1.05f, 0.30f),
              glm::vec3(0.12f, 0.08f, 0.10f),
              COL_CONSOLE, 0.35f);
        ball(base,
             glm::vec3(0.75f, 1.05f, 0.19f),
             0.03f, COL_LIGHT, 1.0f);
    }
}
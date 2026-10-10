// ============================================================================
// submarine.cpp
// ----------------------------------------------------------------------------
// A realistic nuclear submarine, built entirely from procedural primitives.
//
// Layout (side view, local frame; -Z is forward):
//
//     sonar dome     bow planes      sail           stern cross     propeller
//         \            |  |        ___                |  |            //
//   ─────────────────────────────────────────────────────────────   )
//      nose ──── cylindrical midsection ──── tapered tail ──── hub
//
// Hull length 14, radius 1.4 (10:1 ratio, like a real SSN).
// ============================================================================
#include "submarine.h"
#include "mesh.h"
#include "draw.h"
#include "shader.h"
#include "input.h"
#include "constants.h"
#include "rng.h"
#include "island.h"
#include "cave.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    // ---- Mesh cache --------------------------------------------------------
    Mesh hullMesh;
    Mesh sailMesh;
    Mesh sailTopMesh;
    Mesh bowPlaneMesh;
    Mesh sternPlaneMesh;
    Mesh propHubMesh;
    Mesh propBladeMesh;
    Mesh windowMesh;
    Mesh hatchMesh;
    Mesh domeMesh;
    Mesh stripeMesh;
    Mesh antennaMesh;
    Mesh railMesh;
    bool inited = false;

    // ---- Hull lathe profile ------------------------------------------------
    // Values are (t, fraction-of-radius), t from 0 (stern) to 1 (nose).
    // Long cylindrical midsection; short stern taper; rounded blunt nose.
    std::vector<glm::vec2> hullProfile(float L, float R) {
        std::vector<glm::vec2> p;
        struct Pt { float t, f; };
        Pt pts[] = {
            {0.000f, 0.02f},   // stern tip (behind propeller)
            {0.008f, 0.10f},
            {0.022f, 0.22f},
            {0.048f, 0.38f},
            {0.085f, 0.54f},
            {0.130f, 0.70f},
            {0.185f, 0.82f},
            {0.250f, 0.91f},
            {0.320f, 0.97f},
            {0.400f, 1.00f},   // max radius begins
            {0.550f, 1.00f},   // cylindrical midsection
            {0.700f, 1.00f},
            {0.760f, 0.99f},
            {0.820f, 0.95f},
            {0.870f, 0.88f},
            {0.910f, 0.78f},
            {0.945f, 0.64f},
            {0.970f, 0.48f},
            {0.988f, 0.32f},
            {0.998f, 0.16f},
            {1.000f, 0.06f},   // blunt nose cap
        };
        for (auto& pt : pts) {
            float y = -L * 0.5f + pt.t * L;
            p.push_back({y, pt.f * R});
        }
        return p;
    }

    void buildAll() {
        float L = Constants::SUB_LENGTH;
        float R = Constants::SUB_RADIUS;

        hullMesh       = Meshes::latheBody(hullProfile(L, R), 48);
        sailMesh       = Meshes::cube();
        // Unit-size ellipsoid. Actual shape comes from draw-time scale.
        sailTopMesh    = Meshes::ellipsoid(0.5f, 0.5f, 0.5f, 24, 12);
        bowPlaneMesh   = Meshes::cube();
        sternPlaneMesh = Meshes::cube();
        propHubMesh    = Meshes::cylinder(20);
        propBladeMesh  = Meshes::cube();
        windowMesh     = Meshes::sphere(8, 10);
        hatchMesh      = Meshes::hemisphere(10, 14, 1.0f);
        domeMesh       = Meshes::hemisphere(16, 28, 1.0f);
        stripeMesh     = Meshes::cylinder(48);
        antennaMesh    = Meshes::cylinder(8);
        railMesh       = Meshes::cylinder(48);

        inited = true;
    }

    // ---- Palette -----------------------------------------------------------
    const glm::vec3 HULL_DARK   (0.14f, 0.16f, 0.19f);
    const glm::vec3 SAIL_COLOR  (0.10f, 0.11f, 0.14f);
    const glm::vec3 DECK_COLOR  (0.30f, 0.32f, 0.35f);
    const glm::vec3 DETAIL_COL  (0.45f, 0.47f, 0.50f);
    const glm::vec3 STRIPE_RED  (0.72f, 0.10f, 0.10f);
    const glm::vec3 PROP_BRONZE (0.72f, 0.52f, 0.20f);
    const glm::vec3 GLASS_DARK  (0.04f, 0.06f, 0.09f);
}

namespace Sub {
    glm::vec3 pos{0, -1.0f, 120.0f};
    float yawDeg = 180.0f;
    float pitchDeg = 0.0f;
    float propellerAngle = 0.0f;
    float boundingRadius = Constants::SUB_BOUNDING_R;

    void resetToSpawn() {
        float a = Rng::randRange(0.0f, 2.0f * (float)M_PI);
        float r = Rng::randRange(Constants::ISLAND_FOOTPRINT + 15.0f,
                                 Constants::WORLD_RADIUS - 15.0f);
        pos = glm::vec3(r * cosf(a), -1.0f, r * sinf(a));
        yawDeg = glm::degrees(atan2f(-pos.x, -pos.z));
        pitchDeg = 0.0f;
    }

    glm::mat4 rotationMatrix() {
        glm::mat4 r = glm::rotate(glm::mat4(1.0f),
                                  glm::radians(yawDeg), glm::vec3(0, 1, 0));
        r = glm::rotate(r, glm::radians(pitchDeg), glm::vec3(1, 0, 0));
        return r;
    }
    glm::vec3 forward() {
        glm::vec4 f = rotationMatrix() * glm::vec4(0, 0, -1, 0);
        return glm::normalize(glm::vec3(f));
    }
    glm::vec3 up() {
        glm::vec4 u = rotationMatrix() * glm::vec4(0, 1, 0, 0);
        return glm::normalize(glm::vec3(u));
    }
    glm::vec3 torpedoMuzzle() {
        return pos + forward() * Constants::SUB_TORPEDO_OFFSET;
    }

    void update(float dt) {
        if (!inited) buildAll();
        if (Input::paused()) return;
        if (Input::resetSub()) resetToSpawn();

        glm::vec3 fwd = forward();

        float boost = 1.0f;
        if (Input::down(GLFW_KEY_LEFT_CONTROL) ||
            Input::down(GLFW_KEY_RIGHT_CONTROL)) {
            boost = 4.0f;
        }

        float moveSpeed     = Constants::SUB_MOVE_SPEED * boost * dt;
        float turnSpeed     = Constants::SUB_TURN_SPEED * boost * dt;
        float verticalSpeed = Constants::SUB_VERT_SPEED * boost * dt;

        glm::vec3 delta(0.0f);
        if (Input::down(GLFW_KEY_W)) delta += fwd * moveSpeed;
        if (Input::down(GLFW_KEY_S)) delta -= fwd * moveSpeed;

        if (Input::down(GLFW_KEY_A)) yawDeg += turnSpeed;
        if (Input::down(GLFW_KEY_D)) yawDeg -= turnSpeed;

        bool ascending  = Input::down(GLFW_KEY_SPACE);
        bool descending = Input::down(GLFW_KEY_LEFT_SHIFT);
        if (ascending) {
            delta.y += verticalSpeed;
            pitchDeg = glm::mix(pitchDeg, Constants::PITCH_TARGET_UP,
                                std::min(1.0f, dt * 6.0f));
        } else if (descending) {
            delta.y -= verticalSpeed;
            pitchDeg = glm::mix(pitchDeg, Constants::PITCH_TARGET_DOWN,
                                std::min(1.0f, dt * 6.0f));
        } else {
            pitchDeg = glm::mix(pitchDeg, 0.0f, std::min(1.0f, dt * 4.0f));
            if (std::fabs(pitchDeg) < 0.3f) pitchDeg = 0.0f;
        }

        glm::vec3 desired = pos + delta;

        desired.x = std::clamp(desired.x, -Constants::WORLD_RADIUS,
                               Constants::WORLD_RADIUS);
        desired.z = std::clamp(desired.z, -Constants::WORLD_RADIUS,
                               Constants::WORLD_RADIUS);
        desired.y = std::clamp(desired.y,
                               Constants::FLOOR_Y + 0.6f,
                               Constants::WATER_LEVEL + 5.0f);

        // ---- Chamber Y-clamp ----
        {
            float dx = desired.x - Cave::endpointCenter.x;
            float dz = desired.z - Cave::endpointCenter.z;
            float rXZ = sqrtf(dx*dx + dz*dz);
            float R = Cave::endpointRadius;
            if (rXZ < R - 1.5f) {
                float ceilR = sqrtf(std::max(0.0f, R*R - rXZ*rXZ));
                float floorY = Cave::endpointCenter.y + 0.6f;
                float ceilY  = Cave::endpointCenter.y + ceilR - 1.2f;
                if (ceilY > floorY + 0.5f) {
                    if (desired.y < floorY) desired.y = floorY;
                    if (desired.y > ceilY)  desired.y = ceilY;
                }
            }
        }

        // ---- Island collision ----
        if (Island::isInsideIsland(desired) &&
            !Cave::isInsideCave(desired)) {
            float rXZ = std::sqrt(desired.x * desired.x +
                                  desired.z * desired.z);
            if (rXZ > 0.01f) {
                float required = Island::radiusAtY(desired.y)
                               + Constants::SUB_BOUNDING_R;
                if (rXZ < required) {
                    float s = required / rXZ;
                    desired.x *= s;
                    desired.z *= s;
                }
            }
        }

        pos = desired;

        propellerAngle += dt * 540.0f * boost;
        if (propellerAngle > 360.0f) propellerAngle -= 360.0f;
    }

    void draw() {
        if (!inited) buildAll();

        const float L = Constants::SUB_LENGTH;
        const float R = Constants::SUB_RADIUS;

        glm::mat4 base = glm::translate(glm::mat4(1.0f), pos)
                       * rotationMatrix();

        // ================================================================
        // 1. HULL — lathe body rotated so its +Y maps to sub-forward (-Z)
        // ================================================================
        {
            glm::mat4 m = base;
            m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            Draw::modelMaterial(m, HULL_DARK, 48.0f);
            Draw::mesh(hullMesh);
        }

        // ================================================================
        // 2. RED WATERLINE STRIPE — a thin cylinder wrapped around hull
        // ================================================================
        {
            glm::mat4 m = base;
            m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            m = glm::scale(m, glm::vec3(R * 1.015f, L * 0.42f, R * 1.015f));
            Draw::modelMaterial(m, STRIPE_RED, 12.0f);
            Draw::mesh(stripeMesh);
        }

        // ================================================================
        // 3. SONAR DOME at the bow (hemisphere, apex facing forward)
        //    The hemisphere's local apex is at +Y. Rotating -90 degrees
        //    about the X axis maps +Y to -Z, so the dome points forward.
        // ================================================================
        {
            glm::mat4 m = base;
            m = glm::translate(m, glm::vec3(0, 0, -L * 0.5f + 0.20f));
            m = glm::rotate(m, glm::radians(-90.0f), glm::vec3(1, 0, 0));
            m = glm::scale(m, glm::vec3(R * 0.85f, R * 0.60f, R * 0.85f));
            Draw::modelMaterial(m, DETAIL_COL, 96.0f);
            Draw::mesh(domeMesh);
        }

        // ================================================================
        // 4. SAIL (conning tower) — narrow, tall, forward of centre
        // ================================================================
        const float sailLen    = 2.6f;
        const float sailWid    = 0.75f;
        const float sailHgt    = 1.70f;
        const float sailZ      = -L * 0.20f;
        const float sailBaseY  = R * 0.88f;

        // Sail body
        {
            glm::mat4 m = base;
            m = glm::translate(m, glm::vec3(0,
                                            sailBaseY + sailHgt * 0.5f,
                                            sailZ));
            m = glm::scale(m, glm::vec3(sailWid, sailHgt, sailLen));
            Draw::modelMaterial(m, SAIL_COLOR, 32.0f);
            Draw::mesh(sailMesh);
        }
        // Sail rounded top — small cap on top of the sail
        {
            glm::mat4 m = base;
            m = glm::translate(m, glm::vec3(0,
                                            sailBaseY + sailHgt,
                                            sailZ));
            m = glm::scale(m, glm::vec3(sailWid * 1.02f,
                                        sailWid * 0.35f,
                                        sailLen * 1.02f));
            Draw::modelMaterial(m, SAIL_COLOR, 48.0f);
            Draw::mesh(sailTopMesh);
        }
        // Viewports (3 per side)
        for (int side : {-1, 1}) {
            for (int i = 0; i < 3; i++) {
                float z = sailZ - sailLen * 0.30f + i * sailLen * 0.30f;
                glm::mat4 m = base;
                m = glm::translate(m, glm::vec3(
                    side * (sailWid * 0.5f + 0.02f),
                    sailBaseY + sailHgt * 0.62f,
                    z));
                m = glm::scale(m, glm::vec3(0.10f));
                Draw::modelMaterial(m, GLASS_DARK, 128.0f);
                Draw::mesh(windowMesh);
            }
        }
        // Hatch on top of sail
        {
            glm::mat4 m = base;
            m = glm::translate(m, glm::vec3(0,
                                            sailBaseY + sailHgt + 0.06f,
                                            sailZ + 0.45f));
            m = glm::scale(m, glm::vec3(0.30f, 0.18f, 0.30f));
            Draw::modelMaterial(m, DETAIL_COL, 96.0f);
            Draw::mesh(hatchMesh);
        }
        // Three antennas / periscopes on top of sail
        for (int i = 0; i < 3; i++) {
            float dx = -0.18f + i * 0.18f;
            float h  = 0.65f + i * 0.28f;
            glm::mat4 m = base;
            m = glm::translate(m, glm::vec3(
                dx,
                sailBaseY + sailHgt + h * 0.5f + 0.05f,
                sailZ - 0.65f));
            m = glm::scale(m, glm::vec3(0.022f, h * 0.5f, 0.022f));
            Draw::modelMaterial(m, DETAIL_COL, 64.0f);
            Draw::mesh(antennaMesh);
        }

        // ================================================================
        // 5. BOW PLANES (on the hull, forward, slightly angled down)
        // ================================================================
        const float bowPlaneZ = -L * 0.32f;
        const float bowPlaneX = R * 0.55f;
        for (int side : {-1, 1}) {
            glm::mat4 m = base;
            m = glm::translate(m, glm::vec3(side * bowPlaneX * 1.5f,
                                            0.0f,
                                            bowPlaneZ));
            m = glm::rotate(m, glm::radians((float)side * -6.0f),
                            glm::vec3(0, 0, 1));
            m = glm::scale(m, glm::vec3(1.55f, 0.10f, 0.85f));
            Draw::modelMaterial(m, SAIL_COLOR, 32.0f);
            Draw::mesh(bowPlaneMesh);
        }

        // ================================================================
        // 6. STERN CROSS (2 horizontal + 2 vertical fins)
        // ================================================================
        const float sternZ = L * 0.34f;
        const float sternX = R * 0.60f;
        const float sternY = R * 0.60f;
        const float chord  = 0.90f;
        // Horizontal fins
        for (int side : {-1, 1}) {
            glm::mat4 m = base;
            m = glm::translate(m, glm::vec3(side * sternX * 1.5f,
                                            0.0f,
                                            sternZ));
            m = glm::scale(m, glm::vec3(1.15f, 0.10f, chord));
            Draw::modelMaterial(m, SAIL_COLOR, 32.0f);
            Draw::mesh(sternPlaneMesh);
        }
        // Vertical fins
        for (int dir : {-1, 1}) {
            glm::mat4 m = base;
            m = glm::translate(m, glm::vec3(0.0f,
                                            dir * sternY * 1.5f,
                                            sternZ));
            m = glm::scale(m, glm::vec3(0.10f, 1.15f, chord));
            Draw::modelMaterial(m, SAIL_COLOR, 32.0f);
            Draw::mesh(sternPlaneMesh);
        }

        // ================================================================
        // 7. PROPELLER (hub + 7 bronze blades)
        // ================================================================
        const float propZ = L * 0.5f + R * 0.20f;
        // Hub
        {
            glm::mat4 m = base;
            m = glm::translate(m, glm::vec3(0, 0, propZ));
            m = glm::rotate(m, glm::radians(90.0f), glm::vec3(1, 0, 0));
            m = glm::scale(m, glm::vec3(R * 0.32f, R * 0.32f, R * 0.55f));
            Draw::modelMaterial(m, PROP_BRONZE, 96.0f);
            Draw::mesh(propHubMesh);
        }
        // 7 blades
        for (int i = 0; i < 7; i++) {
            glm::mat4 m = base;
            m = glm::translate(m, glm::vec3(0, 0, propZ + R * 0.05f));
            m = glm::rotate(m,
                glm::radians(propellerAngle + i * (360.0f / 7.0f)),
                glm::vec3(0, 0, 1));
            m = glm::rotate(m, glm::radians(22.0f), glm::vec3(1, 0, 0));
            m = glm::translate(m, glm::vec3(0, R * 0.45f, 0));
            m = glm::scale(m, glm::vec3(R * 0.10f, R * 0.70f, R * 0.28f));
            Draw::modelMaterial(m, PROP_BRONZE, 64.0f);
            Draw::mesh(propBladeMesh);
        }

        // ================================================================
        // 8. RUB RAILS (thin lines running along both sides of the hull)
        // ================================================================
        for (int side : {-1, 1}) {
            glm::mat4 m = base;
            m = glm::translate(m, glm::vec3(side * R * 0.98f, 0.0f, 0.0f));
            m = glm::rotate(m, glm::radians(90.0f), glm::vec3(1, 0, 0));
            m = glm::scale(m, glm::vec3(0.03f, L * 0.42f, 0.03f));
            Draw::modelMaterial(m, DETAIL_COL, 32.0f);
            Draw::mesh(railMesh);
        }
    }
}
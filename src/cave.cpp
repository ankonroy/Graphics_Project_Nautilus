// ============================================================================
// cave.cpp
// ============================================================================
#include "cave.h"
#include "island.h"
#include "mesh.h"
#include "draw.h"
#include "shader.h"
#include "constants.h"
#include "texture.h"
#include "rng.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    Mesh g_tube;
    Mesh g_dome;
    Mesh g_disc;
    Mesh g_floorPatch;
    Mesh g_rock;
    Mesh g_cone;

    struct Inst { glm::vec3 pos; float scale; float yawDeg; };
    std::vector<Inst> g_chamberRocks;
    std::vector<Inst> g_stalactites;
    std::vector<Inst> g_stalagmites;

    bool inited = false;

    void buildSegments() {
        Cave::segments.clear();

        float a = Island::entranceAngle;
        glm::vec3 radial(cosf(a), 0.0f, sinf(a));

        glm::vec3 entrance = Island::entrancePos;

        // The chamber's surface at the tunnel's Y is a circle of radius
        // sqrt(R^2 - dy^2) around the chamber's vertical axis.
        float dy = entrance.y - Cave::endpointCenter.y;
        float chamberSurfaceR = sqrtf(std::max(0.0f,
            Cave::endpointRadius * Cave::endpointRadius - dy * dy));
        if (chamberSurfaceR < 1.0f) chamberSurfaceR = 1.0f;

        glm::vec3 chamberSurface(
            Cave::endpointCenter.x + radial.x * chamberSurfaceR,
            entrance.y,
            Cave::endpointCenter.z + radial.z * chamberSurfaceR);

        Cave::segments.push_back({
            entrance, chamberSurface, Constants::CAVE_TUNNEL_RADIUS
        });

        std::cerr << "[Cave] tunnel from (" << entrance.x << ","
                  << entrance.y << "," << entrance.z
                  << ") to (" << chamberSurface.x << ","
                  << chamberSurface.y << "," << chamberSurface.z << ")\n";
    }

    void placeDecoration() {
        g_chamberRocks.clear();
        g_stalactites.clear();
        g_stalagmites.clear();

        glm::vec3 C = Cave::endpointCenter;
        float R = Cave::endpointRadius;

        for (int i = 0; i < 16; i++) {
            float a = Rng::randRange(0.0f, 2.0f * (float)M_PI);
            float r = R * Rng::randRange(0.75f, 0.95f);
            glm::vec3 p(C.x + r * cosf(a),
                        C.y + Rng::randRange(0.4f, 1.4f),
                        C.z + r * sinf(a));
            Inst inst;
            inst.pos    = p;
            inst.scale  = Rng::randRange(0.6f, 1.5f);
            inst.yawDeg = Rng::randRange(0.0f, 360.0f);
            g_chamberRocks.push_back(inst);
        }
        for (int i = 0; i < 12; i++) {
            float a = Rng::randRange(0.0f, 2.0f * (float)M_PI);
            float r = R * Rng::randRange(0.4f, 0.9f);
            float ceilY = C.y + sqrtf(std::max(0.0f, R * R - r * r));
            glm::vec3 p(C.x + r * cosf(a), ceilY - 0.4f, C.z + r * sinf(a));
            Inst inst;
            inst.pos    = p;
            inst.scale  = Rng::randRange(1.0f, 2.4f);
            inst.yawDeg = Rng::randRange(0.0f, 360.0f);
            g_stalactites.push_back(inst);
        }
        for (int i = 0; i < 10; i++) {
            float a = Rng::randRange(0.0f, 2.0f * (float)M_PI);
            float r = R * Rng::randRange(0.35f, 0.85f);
            glm::vec3 p(C.x + r * cosf(a), C.y + 0.4f, C.z + r * sinf(a));
            Inst inst;
            inst.pos    = p;
            inst.scale  = Rng::randRange(0.8f, 2.0f);
            inst.yawDeg = Rng::randRange(0.0f, 360.0f);
            g_stalagmites.push_back(inst);
        }
    }

    void applyTunnelCut() {
        if (Cave::segments.empty()) return;
        const auto& s = Cave::segments[0];
        Shader::setBool("tunnelActive", true);
        Shader::setVec3("tunnelA", s.a);
        Shader::setVec3("tunnelB", s.b);
        Shader::setFloat("tunnelR", s.radius * 1.05f);
    }
    void clearTunnelCut() {
        Shader::setBool("tunnelActive", false);
    }
}

namespace Cave {
    std::vector<Segment> segments;
    glm::vec3 endpointCenter{0.0f, Constants::CAVE_ENDPOINT_Y, 0.0f};
    float     endpointRadius = Constants::CAVE_ENDPOINT_R;

    void build() {
        if (inited) return;
        buildSegments();

        g_tube       = Meshes::tube(28, 8, 1.0f, 1.0f);
        g_dome       = Meshes::hemisphere(24, 40, 1.0f);
        g_disc       = Meshes::cylinder(40);
        g_floorPatch = Meshes::ring(0.0f, 1.0f, 32);
        g_rock       = Meshes::rock(1, 0.28f);
        g_cone       = Meshes::cone(12);

        placeDecoration();
        inited = true;

        std::cerr << "[Cave] chamber at (" << endpointCenter.x << ","
                  << endpointCenter.y << "," << endpointCenter.z
                  << ") r=" << endpointRadius
                  << "  apex y=" << (endpointCenter.y + endpointRadius) << "\n";
    }

    namespace {
        glm::mat4 segmentMatrix(const Segment& s) {
            glm::vec3 d = s.b - s.a;
            float len = glm::length(d);
            glm::vec3 mid = (s.a + s.b) * 0.5f;
            glm::vec3 dir = d / len;
            glm::mat4 m = glm::translate(glm::mat4(1.0f), mid);
            m = m * Draw::alignLocalY(dir);
            m = glm::scale(m, glm::vec3(s.radius, len, s.radius));
            return m;
        }
    }

    void draw() {
        if (!inited) build();

        GLuint rockTex = Textures::rockTex();
        glm::vec3 caveCol(0.24f, 0.19f, 0.16f);

        clearTunnelCut();

        // ---- Chamber floor ----
        {
            glm::mat4 m = glm::translate(glm::mat4(1.0f), endpointCenter);
            m = glm::scale(m, glm::vec3(endpointRadius * 2.0f, 0.4f,
                                        endpointRadius * 2.0f));
            Draw::modelMaterial(m, glm::vec3(0.14f, 0.11f, 0.09f),
                                4.0f, 0.10f, rockTex, true);
            Draw::mesh(g_disc);
        }

        // ---- Warm glowing floor patch ----
        {
            glm::mat4 m = glm::translate(glm::mat4(1.0f),
                endpointCenter + glm::vec3(0.0f, 0.5f, 0.0f));
            m = glm::scale(m, glm::vec3(Constants::CAVE_SUNBEAM_RADIUS,
                                        1.0f,
                                        Constants::CAVE_SUNBEAM_RADIUS));
            Draw::modelMaterial(m, glm::vec3(1.0f, 0.88f, 0.55f),
                                16.0f, 0.9f);
            Draw::mesh(g_floorPatch);
        }

        // ---- Chamber dome — WITH tunnel cut ----
        applyTunnelCut();
        {
            glm::mat4 m = glm::translate(glm::mat4(1.0f), endpointCenter);
            m = glm::scale(m, glm::vec3(endpointRadius));
            Draw::modelMaterial(m, caveCol, 6.0f, 0.12f, rockTex, true);
            Draw::mesh(g_dome);
        }
        clearTunnelCut();

        // ---- Tunnel tube ----
        for (auto& s : segments) {
            glm::mat4 m = segmentMatrix(s);
            Draw::modelMaterial(m, caveCol, 6.0f, 0.10f, rockTex, true);
            Draw::mesh(g_tube);
        }

        // ---- Chamber wall rocks ----
        for (auto& r : g_chamberRocks) {
            glm::mat4 m = glm::translate(glm::mat4(1.0f), r.pos);
            m = glm::rotate(m, glm::radians(r.yawDeg), glm::vec3(0, 1, 0));
            m = glm::scale(m, glm::vec3(r.scale));
            Draw::modelMaterial(m, glm::vec3(0.30f, 0.24f, 0.20f),
                                6.0f, 0.10f, rockTex, true);
            Draw::mesh(g_rock);
        }

        // ---- Stalactites ----
        for (auto& st : g_stalactites) {
            glm::mat4 m = glm::translate(glm::mat4(1.0f), st.pos);
            m = glm::rotate(m, glm::radians(st.yawDeg), glm::vec3(0, 1, 0));
            m = glm::rotate(m, glm::radians(180.0f), glm::vec3(1, 0, 0));
            m = glm::scale(m, glm::vec3(0.4f, st.scale, 0.4f));
            Draw::modelMaterial(m, glm::vec3(0.28f, 0.24f, 0.20f),
                                6.0f, 0.10f, rockTex, true);
            Draw::mesh(g_cone);
        }

        // ---- Stalagmites ----
        for (auto& sg : g_stalagmites) {
            glm::mat4 m = glm::translate(glm::mat4(1.0f), sg.pos);
            m = glm::rotate(m, glm::radians(sg.yawDeg), glm::vec3(0, 1, 0));
            m = glm::scale(m, glm::vec3(0.4f, sg.scale, 0.4f));
            Draw::modelMaterial(m, glm::vec3(0.28f, 0.24f, 0.20f),
                                6.0f, 0.10f, rockTex, true);
            Draw::mesh(g_cone);
        }
    }

    bool isInsideCave(const glm::vec3& p) {
        if (!inited) build();

        glm::vec3 d = p - endpointCenter;
        if (d.y >= -1.0f && d.y <= endpointRadius) {
            float y2 = std::max(d.y, 0.0f);
            float radAtY = sqrtf(endpointRadius * endpointRadius - y2 * y2);
            float rXZ = sqrtf(d.x * d.x + d.z * d.z);
            if (rXZ <= radAtY + 1.0f) return true;
        }

        for (auto& s : segments) {
            glm::vec3 td = s.b - s.a;
            float len2 = glm::dot(td, td);
            if (len2 < 1e-4f) continue;
            float t = glm::clamp(glm::dot(p - s.a, td) / len2, 0.0f, 1.0f);
            glm::vec3 closest = s.a + td * t;
            if (glm::length(p - closest)
                < s.radius + Constants::SUB_BOUNDING_R) return true;
        }
        return false;
    }
}
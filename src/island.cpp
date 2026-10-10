// ============================================================================
// island.cpp
// ============================================================================
#include "island.h"
#include "cave.h"
#include "mesh.h"
#include "draw.h"
#include "shader.h"
#include "constants.h"
#include "texture.h"
#include "rng.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    Mesh g_sand, g_grass, g_volcano;
    Mesh g_craterFloor, g_lavaPatch;
    Mesh g_grassBlade;
    Mesh g_shoreRock;
    bool inited = false;

    struct GrassInst { glm::vec3 pos; float yawDeg, scale; glm::vec3 color; };
    std::vector<GrassInst> g_grassInst;
    struct ShoreRock { glm::vec3 pos; float scale, yawDeg; glm::vec3 color; };
    std::vector<ShoreRock> g_shoreRocks;

    const glm::vec3 sandCol   (0.90f, 0.80f, 0.55f);
    const glm::vec3 grassCol  (0.30f, 0.60f, 0.22f);
    const glm::vec3 rockCol   (0.30f, 0.26f, 0.22f);
    const glm::vec3 darkRock  (0.18f, 0.15f, 0.12f);
    const glm::vec3 lavaCol   (1.00f, 0.45f, 0.10f);

    const float SAND_Y[] = { -55, -45, -35, -30, -25, -20, -15, -8, -4, 0, 2, 3, 4, 5, 6 };
    const float SAND_R[] = {  90,  82,  76,  72,  66,  60,  54, 48, 45, 43, 41, 39, 37, 34, 32 };
    const int   SAND_N   = 15;

    std::vector<glm::vec2> sandProfile() {
        std::vector<glm::vec2> p;
        for (int i = 0; i < SAND_N; i++) p.push_back({ SAND_Y[i], SAND_R[i] });
        return p;
    }
    std::vector<glm::vec2> grassProfile() {
        return {
            { 6.0f, 32.0f }, { 7.0f, 29.0f }, { 8.0f, 27.0f },
            { 9.0f, 25.0f }, { 10.0f, 24.0f }
        };
    }
    std::vector<glm::vec2> volcanoProfile() {
        return {
            { 10.0f, 24.0f }, { 12.0f, 21.0f }, { 14.0f, 17.0f },
            { 16.0f, 13.0f }, { 18.0f, 9.0f }, { 20.0f, 5.0f }, { 21.0f, 4.0f }
        };
    }

    float sandRadiusAt(float y) {
        if (y <= SAND_Y[0])          return SAND_R[0];
        if (y >= SAND_Y[SAND_N - 1]) return SAND_R[SAND_N - 1];
        for (int i = 0; i < SAND_N - 1; i++) {
            if (y >= SAND_Y[i] && y <= SAND_Y[i + 1]) {
                float t = (y - SAND_Y[i]) / (SAND_Y[i + 1] - SAND_Y[i]);
                return SAND_R[i] + t * (SAND_R[i + 1] - SAND_R[i]);
            }
        }
        return SAND_R[SAND_N - 1];
    }

    float sandYAt(float r) {
        if (r >= SAND_R[0])          return SAND_Y[0];
        if (r <= SAND_R[SAND_N - 1]) return SAND_Y[SAND_N - 1];
        for (int i = 0; i < SAND_N - 1; i++) {
            if (r <= SAND_R[i] && r >= SAND_R[i + 1]) {
                float t = (SAND_R[i] - r) / (SAND_R[i] - SAND_R[i + 1]);
                return SAND_Y[i] + t * (SAND_Y[i + 1] - SAND_Y[i]);
            }
        }
        return SAND_Y[SAND_N - 1];
    }

    void scatter() {
        g_grassInst.clear();
        g_shoreRocks.clear();

        for (int i = 0; i < 140; i++) {
            float a = Rng::randRange(0.0f, 2.0f * (float)M_PI);
            float r = Rng::randRange(24.0f, 31.5f);
            float t = (32.0f - r) / (32.0f - 24.0f);
            float y = 6.0f + t * (10.0f - 6.0f);
            GrassInst g;
            g.pos    = glm::vec3(r * cosf(a), y - 0.05f, r * sinf(a));
            g.yawDeg = Rng::randRange(0.0f, 360.0f);
            g.scale  = Rng::randRange(0.6f, 1.4f);
            g.color  = grassCol * Rng::randRange(0.8f, 1.2f);
            g_grassInst.push_back(g);
        }
        for (int i = 0; i < 30; i++) {
            float a = Rng::randRange(0.0f, 2.0f * (float)M_PI);
            float r = Rng::randRange(38.0f, 46.0f);
            float y = sandYAt(r);
            ShoreRock sr;
            sr.pos    = glm::vec3(r * cosf(a), y - 0.2f, r * sinf(a));
            sr.scale  = Rng::randRange(0.5f, 1.3f);
            sr.yawDeg = Rng::randRange(0.0f, 360.0f);
            sr.color  = glm::mix(darkRock, rockCol, Rng::randFloat());
            g_shoreRocks.push_back(sr);
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

namespace Island {
    glm::vec3 entrancePos{0, 0, 0};
    float     entranceAngle = 0.0f;

    void build() {
        if (inited) return;

        g_sand        = Meshes::latheBody(sandProfile(),    64);
        g_grass       = Meshes::latheBody(grassProfile(),   64);
        g_volcano     = Meshes::latheBody(volcanoProfile(), 64);
        g_craterFloor = Meshes::ring(0.0f, 1.0f, 32);
        g_lavaPatch   = Meshes::ring(0.0f, 1.0f, 32);
        g_grassBlade  = Meshes::triangle();
        g_shoreRock   = Meshes::rock(1, 0.18f);

        // Entrance at the tunnel's Y (above the chamber floor).
        entranceAngle = Rng::randRange(0.0f, 2.0f * (float)M_PI);
        float entY = Constants::CAVE_ENTRANCE_Y;
        float entR = sandRadiusAt(entY);
        entrancePos = glm::vec3(entR * cosf(entranceAngle),
                                entY,
                                entR * sinf(entranceAngle));

        std::cerr << "[Island] entrance at r=" << entR
                  << ", y=" << entY << "\n";

        scatter();
        inited = true;
    }

    float radiusAtY(float y) {
        if (y < -55.0f) return 0.0f;
        if (y >  21.0f) return 0.0f;
        if (y <= 6.0f)  return sandRadiusAt(y);
        if (y <= 10.0f) {
            float t = (y - 6.0f) / 4.0f;
            return 32.0f + t * (24.0f - 32.0f);
        }
        float t = (y - 10.0f) / 11.0f;
        return 24.0f + t * (4.0f - 24.0f);
    }

    bool isInsideIsland(const glm::vec3& p) {
        float r = radiusAtY(p.y);
        if (r < 0.1f) return false;
        float rXZ = std::sqrt(p.x * p.x + p.z * p.z);
        return rXZ < r;
    }

    void draw() {
        if (!inited) build();

        GLuint sandTex = Textures::sandTex();
        GLuint rockTex = Textures::rockTex();
        GLuint lavaTex = Textures::lava();

        applyTunnelCut();
        { glm::mat4 m = glm::mat4(1.0f);
          Draw::modelMaterial(m, sandCol, 6.0f, 0.10f, sandTex, true);
          Draw::mesh(g_sand); }
        clearTunnelCut();

        { glm::mat4 m = glm::mat4(1.0f);
          Draw::modelMaterial(m, grassCol, 4.0f, 0.15f);
          Draw::mesh(g_grass); }

        { glm::mat4 m = glm::mat4(1.0f);
          Draw::modelMaterial(m, rockCol, 4.0f, 0.0f, rockTex, true);
          Draw::mesh(g_volcano); }

        { glm::mat4 m = glm::translate(glm::mat4(1.0f),
            glm::vec3(0.0f,
                      Constants::VOLCANO_BASE_Y + Constants::VOLCANO_HEIGHT - 0.5f,
                      0.0f));
          m = glm::scale(m, glm::vec3(Constants::CRATER_RADIUS, 1.0f,
                                      Constants::CRATER_RADIUS));
          Draw::modelMaterial(m, darkRock, 4.0f, 0.15f);
          Draw::mesh(g_craterFloor); }

        { glm::mat4 m = glm::translate(glm::mat4(1.0f),
            glm::vec3(0.0f,
                      Constants::VOLCANO_BASE_Y + Constants::VOLCANO_HEIGHT - 0.4f,
                      0.0f));
          m = glm::scale(m, glm::vec3(Constants::CRATER_RADIUS - 0.8f, 1.0f,
                                      Constants::CRATER_RADIUS - 0.8f));
          Draw::modelMaterial(m, lavaCol, 16.0f, 0.9f, lavaTex, true);
          Draw::mesh(g_lavaPatch); }

        for (auto& sr : g_shoreRocks) {
            glm::mat4 m = glm::translate(glm::mat4(1.0f), sr.pos);
            m = glm::rotate(m, glm::radians(sr.yawDeg), glm::vec3(0, 1, 0));
            m = glm::scale(m, glm::vec3(sr.scale));
            Draw::modelMaterial(m, sr.color, 4.0f, 0.0f, rockTex, true);
            Draw::mesh(g_shoreRock);
        }
        for (auto& gr : g_grassInst) {
            glm::mat4 m = glm::translate(glm::mat4(1.0f), gr.pos);
            m = glm::rotate(m, glm::radians(gr.yawDeg), glm::vec3(0, 1, 0));
            m = glm::scale(m, glm::vec3(0.07f * gr.scale,
                                        0.35f * gr.scale,
                                        0.025f * gr.scale));
            Draw::modelMaterial(m, gr.color, 4.0f, 0.0f);
            Draw::mesh(g_grassBlade);
        }
    }
}
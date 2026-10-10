// ============================================================================
// hud.cpp
// ----------------------------------------------------------------------------
// Text drawn as small filled quads (one per lit pixel of a 5x7 font).
// No texture, no atlas, no UVs.
// ============================================================================
#include "hud.h"
#include "shader.h"
#include "particles.h"
#include "mesh.h"
#include "draw.h"
#include "input.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <vector>
#include <string>
#include <cstdio>
#include <cstring>

namespace {

struct Glyph5x7 { const char* r[7]; };

Glyph5x7 glyphFor(char c) {
    switch (c) {
    case 'A': return {".###.","#...#","#...#","#####","#...#","#...#","#...#"};
    case 'B': return {"####.","#...#","#...#","####.","#...#","#...#","####."};
    case 'C': return {".###.","#...#","#....","#....","#....","#...#",".###."};
    case 'D': return {"####.","#...#","#...#","#...#","#...#","#...#","####."};
    case 'E': return {"#####","#....","#....","####.","#....","#....","#####"};
    case 'F': return {"#####","#....","#....","####.","#....","#....","#...."};
    case 'G': return {".###.","#...#","#....","#.###","#...#","#...#",".###."};
    case 'H': return {"#...#","#...#","#...#","#####","#...#","#...#","#...#"};
    case 'I': return {".###.","..#..","..#..","..#..","..#..","..#..",".###."};
    case 'J': return {"..###","...#.","...#.","...#.","...#.","#..#.",".##.."};
    case 'K': return {"#...#","#..#.","#.#..","##...","#.#..","#..#.","#...#"};
    case 'L': return {"#....","#....","#....","#....","#....","#....","#####"};
    case 'M': return {"#...#","##.##","#.#.#","#...#","#...#","#...#","#...#"};
    case 'N': return {"#...#","##..#","#.#.#","#..##","#...#","#...#","#...#"};
    case 'O': return {".###.","#...#","#...#","#...#","#...#","#...#",".###."};
    case 'P': return {"####.","#...#","#...#","####.","#....","#....","#...."};
    case 'Q': return {".###.","#...#","#...#","#...#","#.#.#","#..#.",".##.#"};
    case 'R': return {"####.","#...#","#...#","####.","#.#..","#..#.","#...#"};
    case 'S': return {".####","#....","#....",".###.","....#","....#","####."};
    case 'T': return {"#####","..#..","..#..","..#..","..#..","..#..","..#.."};
    case 'U': return {"#...#","#...#","#...#","#...#","#...#","#...#",".###."};
    case 'V': return {"#...#","#...#","#...#","#...#","#...#",".#.#.","..#.."};
    case 'W': return {"#...#","#...#","#...#","#...#","#.#.#","##.##","#...#"};
    case 'X': return {"#...#","#...#",".#.#.","..#..",".#.#.","#...#","#...#"};
    case 'Y': return {"#...#","#...#",".#.#.","..#..","..#..","..#..","..#.."};
    case 'Z': return {"#####","....#","...#.","..#..",".#...","#....","#####"};
    case '0': return {".###.","#...#","#..##","#.#.#","##..#","#...#",".###."};
    case '1': return {"..#..",".##..","..#..","..#..","..#..","..#..",".###."};
    case '2': return {".###.","#...#","....#","...#.","..#..",".#...","#####"};
    case '3': return {"####.","....#","....#",".###.","....#","....#","####."};
    case '4': return {"#...#","#...#","#...#","#####","....#","....#","....#"};
    case '5': return {"#####","#....","#....","####.","....#","....#","####."};
    case '6': return {".###.","#....","#....","####.","#...#","#...#",".###."};
    case '7': return {"#####","....#","...#.","..#..",".#...",".#...",".#..."};
    case '8': return {".###.","#...#","#...#",".###.","#...#","#...#",".###."};
    case '9': return {".###.","#...#","#...#",".####","....#","....#",".###."};
    case ' ': return {".....",".....",".....",".....",".....",".....","....."};
    case ':': return {".....","..#..","..#..",".....","..#..","..#..","....."};
    case '/': return {"....#","....#","...#.","..#..",".#...","#....","#...."};
    case '.': return {".....",".....",".....",".....",".....",".##..",".##.."};
    case ',': return {".....",".....",".....",".....",".##..",".##..",".#..."};
    case '-': return {".....",".....",".....","#####",".....",".....","....."};
    case '+': return {".....","..#..","..#..","#####","..#..","..#..","....."};
    case '>': return {"#....",".#...","..#..","...#.","..#..",".#...","#...."};
    case '<': return {"....#","...#.","..#..",".#...","..#..","...#.","....#"};
    case '!': return {"..#..","..#..","..#..","..#..","..#..",".....","..#.."};
    case '?': return {".###.","#...#","....#","...#.","..#..",".....","..#.."};
    case '=': return {".....",".....","#####",".....","#####",".....","....."};
    case '(': return {"...#.","..#..",".#...",".#...",".#...","..#..","...#."};
    case ')': return {".#...","..#..","...#.","...#.","...#.","..#..",".#..."};
    case '[': return {".###.",".#...",".#...",".#...",".#...",".#...",".###."};
    case ']': return {".###.","...#.","...#.","...#.","...#.","...#.",".###."};
    case '_': return {".....",".....",".....",".....",".....",".....","#####"};
    case '|': return {"..#..","..#..","..#..","..#..","..#..","..#..","..#.."};
    case '*': return {".....","#.#.#",".###.","#####",".###.","#.#.#","....."};
    default:  return {"#####","#...#","#...#","#...#","#...#","#...#","#####"};
    }
}

std::string g_status = "READY";
int   g_cpVisited = 0, g_cpTotal = 0;
int   g_torRem = 0, g_torMax = 0;
float g_torCooldownFrac = 0.0f;
int   g_hullRemaining = 3, g_hullMax = 3;

struct Overlay {
    std::string id, title;
    std::vector<std::string> buttons;
    int selected = 0;
};
Overlay g_overlay;
bool    g_hasOverlay = false;

struct HudVertex { float px, py, pz; float nx, ny, nz; float u, v; };
std::vector<HudVertex>    g_verts;
std::vector<unsigned int> g_idx;

struct Segment { size_t start; size_t count; glm::vec3 color; };
std::vector<Segment> g_segments;

GLuint g_vao = 0, g_vbo = 0, g_ebo = 0;
bool   g_glInit = false;

void ensureGl() {
    if (g_glInit) return;
    glGenVertexArrays(1, &g_vao);
    glGenBuffers(1, &g_vbo);
    glGenBuffers(1, &g_ebo);
    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ebo);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(HudVertex), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(HudVertex), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(HudVertex), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);
    glBindVertexArray(0);
    g_glInit = true;
}

void pushQuad(float x, float y, float w, float h, const glm::vec3& c) {
    if (g_segments.empty() ||
        std::memcmp(&g_segments.back().color, &c, sizeof(glm::vec3)) != 0) {
        Segment s;
        s.start = g_idx.size();
        s.count = 0;
        s.color = c;
        g_segments.push_back(s);
    }

    unsigned int base = (unsigned int)g_verts.size();
    HudVertex v{};
    v.nx = 0; v.ny = 0; v.nz = 1;
    v.u  = 0; v.v  = 0;
    v.px = x;     v.py = y;     v.pz = 0; g_verts.push_back(v);
    v.px = x + w; v.py = y;     v.pz = 0; g_verts.push_back(v);
    v.px = x + w; v.py = y + h; v.pz = 0; g_verts.push_back(v);
    v.px = x;     v.py = y + h; v.pz = 0; g_verts.push_back(v);

    g_idx.push_back(base + 0); g_idx.push_back(base + 1); g_idx.push_back(base + 2);
    g_idx.push_back(base + 0); g_idx.push_back(base + 2); g_idx.push_back(base + 3);

    g_segments.back().count = g_idx.size() - g_segments.back().start;
}

int pushChar(char c, float x, float y, float px, const glm::vec3& col) {
    if (c >= 'a' && c <= 'z') c = (char)(c - 'a' + 'A');
    Glyph5x7 g = glyphFor(c);
    for (int row = 0; row < 7; row++) {
        for (int col2 = 0; col2 < 5; col2++) {
            if (g.r[row][col2] == '#') {
                float py  = y + (6 - row) * px;
                float pxx = x + col2 * px;
                pushQuad(pxx, py, px, px, col);
            }
        }
    }
    return 6;
}

int pushText(const std::string& s, float x, float y, float px,
             const glm::vec3& col) {
    float cx = x;
    for (char c : s) cx += pushChar(c, cx, y, px, col) * px;
    return (int)(cx - x);
}

int textWidth(const std::string& s, float px) {
    return (int)(s.size() * 6.0f * px);
}

void flushAndDraw(int fbW, int fbH) {
    if (g_verts.empty()) return;
    ensureGl();

    glBindVertexArray(g_vao);
    glBindBuffer(GL_ARRAY_BUFFER, g_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 (GLsizeiptr)(g_verts.size() * sizeof(HudVertex)),
                 g_verts.data(), GL_STREAM_DRAW);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, g_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 (GLsizeiptr)(g_idx.size() * sizeof(unsigned int)),
                 g_idx.data(), GL_STREAM_DRAW);

    Shader::use();
    glm::mat4 ortho = glm::ortho(0.0f, (float)fbW, 0.0f, (float)fbH,
                                 -1.0f, 1.0f);
    Shader::setMat4("projection", ortho);
    Shader::setMat4("view", glm::mat4(1.0f));
    Shader::setMat4("model", glm::mat4(1.0f));
    Shader::setMat3("normalMatrix", glm::mat3(1));
    Shader::setVec3("viewPos", glm::vec3(0, 0, 1));
    Shader::setVec3("ambientColor", glm::vec3(1));
    Shader::setVec3("dirLightDir", glm::vec3(0, -1, 0));
    Shader::setVec3("dirLightColor", glm::vec3(0));
    Shader::setBool("headlightOn", false);
    Shader::setInt("shadingMode", 1);
    Shader::setVec3("fogColor", glm::vec3(0));
    Shader::setFloat("fogDensity", 0);
    Shader::setBool("useTexture", false);
    Shader::setFloat("emissiveBoost", 0.0f);
    Shader::setVec4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, 10000.0f));

    for (auto& s : g_segments) {
        Shader::setVec3("objectColor", s.color);
        glDrawElements(GL_TRIANGLES, (GLsizei)s.count,
                       GL_UNSIGNED_INT,
                       (void*)(s.start * sizeof(unsigned int)));
    }

    glBindVertexArray(0);
    g_verts.clear();
    g_idx.clear();
    g_segments.clear();
}

} // namespace

namespace HUD {
    void setStatus(const std::string& s) { g_status = s; }
    void setCheckpoints(int v, int t)    { g_cpVisited = v; g_cpTotal = t; }
    void setTorpedoInfo(int rem, int max, float frac) {
        g_torRem = rem; g_torMax = max; g_torCooldownFrac = frac;
    }
    void setHullIntegrity(int remaining, int max) {
        g_hullRemaining = remaining;
        g_hullMax       = max;
    }

    void pushOverlay(const std::string& id, const std::string& title,
                     const std::vector<std::string>& buttons, int sel) {
        g_overlay.id = id;
        g_overlay.title = title;
        g_overlay.buttons = buttons;
        g_overlay.selected = sel;
        g_hasOverlay = true;
    }
    void clearOverlay() { g_hasOverlay = false; }
    bool hasOverlay()   { return g_hasOverlay; }

    void render(int fbW, int fbH) {
        g_verts.clear(); g_idx.clear(); g_segments.clear();

        glDisable(GL_DEPTH_TEST);
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        const float PX         = 4.0f;
        const float glyphH     = 7.0f * PX;
        const float lineH      = 9.0f * PX;
        const float topMargin  = 20.0f;
        const float leftMargin = 20.0f;

        float y = (float)fbH - topMargin - glyphH;
        char buf[128];

        pushText(g_status, leftMargin, y, PX, glm::vec3(1.0f, 0.90f, 0.30f));
        y -= lineH;

        std::snprintf(buf, sizeof(buf),
                      "CHECKPOINTS: %d / %d", g_cpVisited, g_cpTotal);
        pushText(buf, leftMargin, y, PX, glm::vec3(0.95f));
        y -= lineH;

        std::snprintf(buf, sizeof(buf),
                      "TORPEDOES: %d / %d", g_torRem, g_torMax);
        pushText(buf, leftMargin, y, PX, glm::vec3(0.95f));
        y -= lineH;

        glm::vec3 hullCol = (g_hullRemaining <= 1)
            ? glm::vec3(1.0f, 0.30f, 0.30f)
            : glm::vec3(0.95f);
        std::snprintf(buf, sizeof(buf),
                      "HULL: %d / %d", g_hullRemaining, g_hullMax);
        pushText(buf, leftMargin, y, PX, hullCol);
        y -= lineH * 0.9f;

        if (g_torCooldownFrac > 0.0f) {
            float barW = 60.0f * PX;
            float barH = 3.0f * PX;
            float filled = barW * (1.0f - g_torCooldownFrac);
            pushQuad(leftMargin, y, barW, barH, glm::vec3(0.15f, 0.15f, 0.20f));
            pushQuad(leftMargin, y, filled, barH, glm::vec3(1.0f, 0.85f, 0.20f));
        }

        if (g_hasOverlay) {
            pushQuad(0, 0, (float)fbW, (float)fbH,
                     glm::vec3(0.05f, 0.06f, 0.10f));
            float titlePx = 7.0f;
            int titleW = textWidth(g_overlay.title, titlePx);
            float tx = ((float)fbW - titleW) * 0.5f;
            float ty = (float)fbH * 0.60f;
            pushText(g_overlay.title, tx, ty, titlePx,
                     glm::vec3(1.0f, 0.88f, 0.25f));
            float btnPx = 5.0f;
            float btnH  = 9.0f * btnPx;
            float btnY  = (float)fbH * 0.35f;
            for (size_t i = 0; i < g_overlay.buttons.size(); i++) {
                std::string label = g_overlay.buttons[i];
                if ((int)i == g_overlay.selected) label = "> " + label;
                else                                label = "  " + label;
                int w = textWidth(label, btnPx);
                float bx = ((float)fbW - w) * 0.5f;
                glm::vec3 col = ((int)i == g_overlay.selected)
                    ? glm::vec3(1.0f, 0.95f, 0.40f)
                    : glm::vec3(0.85f, 0.85f, 0.85f);
                pushText(label, bx, btnY - i * btnH * 1.2f, btnPx, col);
            }
        }

        glm::vec4 f = Particles::currentFlash();
        if (f.a > 0.0f) {
            pushQuad(0, 0, (float)fbW, (float)fbH,
                     glm::vec3(f.r, f.g, f.b));
        }

        flushAndDraw(fbW, fbH);

        glDisable(GL_BLEND);
        glEnable(GL_DEPTH_TEST);
    }

    void drawFlash(float, int, int) {}
}
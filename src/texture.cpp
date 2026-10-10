// ============================================================================
// texture.cpp - generate simple textures at startup using only stdlib + GL.
// ============================================================================
#include "texture.h"
#include "rng.h"
#include <cmath>
#include <vector>
#include <cstring>
#include <iostream>
#include <algorithm>

namespace {
    constexpr int N = 256;

    GLuint makeRGBA(const std::vector<unsigned char>& px, int w, int h) {
        GLuint id = 0;
        glGenTextures(1, &id);
        glBindTexture(GL_TEXTURE_2D, id);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0, GL_RGBA,
                     GL_UNSIGNED_BYTE, px.data());
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
        glGenerateMipmap(GL_TEXTURE_2D);
        return id;
    }

    // Simple value noise: bilinear over a small random grid. Deterministic.
    float vnoise(float x, float y) {
        int ix = (int)std::floor(x);
        int iy = (int)std::floor(y);
        float fx = x - ix;
        float fy = y - iy;
        auto h = [&](int i, int j) {
            // Hash into 0..1
            unsigned int s = (unsigned int)(i * 374761393 + j * 668265263);
            s = (s ^ (s >> 13)) * 1274126177u;
            return ((s & 0xFFFF) / 65535.0f);
        };
        float a = h(ix, iy);
        float b = h(ix+1, iy);
        float c = h(ix, iy+1);
        float d = h(ix+1, iy+1);
        // Smoothstep weights for organic look
        float u = fx * fx * (3 - 2 * fx);
        float v = fy * fy * (3 - 2 * fy);
        return ((1-u)*(1-v)*a + u*(1-v)*b + (1-u)*v*c + u*v*d);
    }

    float fbm(float x, float y, int octaves = 5) {
        float v = 0, amp = 0.5f;
        for (int i = 0; i < octaves; i++) {
            v += amp * vnoise(x, y);
            x *= 2.0f; y *= 2.0f; amp *= 0.5f;
        }
        return v;
    }

    GLuint g_checker = 0, g_noise = 0, g_sand = 0, g_rock = 0;
    GLuint g_stripe = 0, g_metal = 0, g_wood = 0, g_lava = 0;
}

namespace Textures {
    bool initAll() {
        // Checkerboard
        {
            std::vector<unsigned char> px(N * N * 4);
            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    int i = (y * N + x) * 4;
                    bool c = ((x / 32) + (y / 32)) & 1;
                    unsigned char v = c ? 220 : 40;
                    px[i] = px[i+1] = px[i+2] = v; px[i+3] = 255;
                }
            }
            g_checker = makeRGBA(px, N, N);
        }
        // Noise
        {
            std::vector<unsigned char> px(N * N * 4);
            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    int i = (y * N + x) * 4;
                    float n = fbm(x * 0.04f, y * 0.04f, 5);
                    unsigned char v = (unsigned char)(std::clamp(n, 0.0f, 1.0f) * 255);
                    px[i] = px[i+1] = px[i+2] = v; px[i+3] = 255;
                }
            }
            g_noise = makeRGBA(px, N, N);
        }
        // Sand (tan)
        {
            std::vector<unsigned char> px(N * N * 4);
            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    int i = (y * N + x) * 4;
                    float n = fbm(x * 0.05f, y * 0.05f, 5);
                    float s = 0.6f + 0.4f * n;
                    px[i]   = (unsigned char)(220 * s);
                    px[i+1] = (unsigned char)(190 * s);
                    px[i+2] = (unsigned char)(120 * s);
                    px[i+3] = 255;
                }
            }
            g_sand = makeRGBA(px, N, N);
        }
        // Rock (gray with brown tint)
        {
            std::vector<unsigned char> px(N * N * 4);
            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    int i = (y * N + x) * 4;
                    float n = fbm(x * 0.10f, y * 0.10f, 6);
                    float s = 0.4f + 0.6f * n;
                    px[i]   = (unsigned char)(120 * s);
                    px[i+1] = (unsigned char)(105 * s);
                    px[i+2] = (unsigned char)( 85 * s);
                    px[i+3] = 255;
                }
            }
            g_rock = makeRGBA(px, N, N);
        }
        // Stripes (diagonal)
        {
            int w = 256, h = 64;
            std::vector<unsigned char> px(w * h * 4);
            for (int y = 0; y < h; y++) {
                for (int x = 0; x < w; x++) {
                    int i = (y * w + x) * 4;
                    float u = (x + y) / 16.0f;
                    float f = 0.5f + 0.5f * std::cos(u);
                    px[i]   = (unsigned char)(200 * f + 30);
                    px[i+1] = (unsigned char)(200 * f + 30);
                    px[i+2] = (unsigned char)(200 * f + 30);
                    px[i+3] = 255;
                }
            }
            g_stripe = makeRGBA(px, w, h);
        }
        // Metal panel
        {
            std::vector<unsigned char> px(N * N * 4);
            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    int i = (y * N + x) * 4;
                    float n = fbm(x * 0.02f, y * 0.02f, 3);
                    float s = 0.45f + 0.25f * n;
                    px[i]   = (unsigned char)(90 * s);
                    px[i+1] = (unsigned char)(95 * s);
                    px[i+2] = (unsigned char)(105 * s);
                    px[i+3] = 255;
                }
            }
            g_metal = makeRGBA(px, N, N);
        }
        // Wood
        {
            std::vector<unsigned char> px(N * N * 4);
            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    int i = (y * N + x) * 4;
                    float n = fbm(x * 0.05f, y * 0.05f, 4);
                    float rings = 0.5f + 0.5f * std::sin((x + n * 40) * 0.08f);
                    px[i]   = (unsigned char)(110 + 50 * rings);
                    px[i+1] = (unsigned char)( 75 + 35 * rings);
                    px[i+2] = (unsigned char)( 45 + 20 * rings);
                    px[i+3] = 255;
                }
            }
            g_wood = makeRGBA(px, N, N);
        }
        // Lava
        {
            std::vector<unsigned char> px(N * N * 4);
            for (int y = 0; y < N; y++) {
                for (int x = 0; x < N; x++) {
                    int i = (y * N + x) * 4;
                    float n = fbm(x * 0.04f + 13, y * 0.04f + 27, 5);
                    float r = std::clamp(0.6f + 0.8f * n, 0.0f, 1.5f);
                    float g = std::clamp(0.2f + 0.6f * n * n, 0.0f, 1.0f);
                    float b = 0.05f * n;
                    px[i]   = (unsigned char)(std::min(r, 1.0f) * 255);
                    px[i+1] = (unsigned char)(g * 255);
                    px[i+2] = (unsigned char)(b * 255);
                    px[i+3] = 255;
                }
            }
            g_lava = makeRGBA(px, N, N);
        }
        std::cerr << "[Texture] generated 8 procedural textures\n";
        return true;
    }

    void shutdownAll() {
        auto del = [](GLuint& t) { if (t) glDeleteTextures(1, &t); t = 0; };
        del(g_checker); del(g_noise); del(g_sand); del(g_rock);
        del(g_stripe); del(g_metal); del(g_wood); del(g_lava);
    }

    GLuint checker()    { return g_checker; }
    GLuint noiseTex()   { return g_noise; }
    GLuint sandTex()    { return g_sand; }
    GLuint rockTex()    { return g_rock; }
    GLuint stripe()     { return g_stripe; }
    GLuint metalPanel() { return g_metal; }
    GLuint woodPanel()  { return g_wood; }
    GLuint lava()       { return g_lava; }

    void bind(GLuint tex) {
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, tex);
    }
}

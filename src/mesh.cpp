// ============================================================================
// mesh.cpp
// ----------------------------------------------------------------------------
// All procedural geometry lives here. New mesh types should be added to this
// file so the rest of the engine only consumes Mesh handles.
//
// Vertex layout (stride 8 floats):
//   [px py pz  nx ny nz  u v]
// ============================================================================
#include "mesh.h"
#include "rng.h"
#include <cmath>
#include <iostream>
#include <unordered_map>
#include <functional>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// ---------------------------------------------------------------------------
// Mesh RAII
// ---------------------------------------------------------------------------
Mesh::Mesh(Mesh&& o) noexcept
    : vao(o.vao), vbo(o.vbo), ebo(o.ebo),
      indexCount(o.indexCount), center(o.center), radius(o.radius) {
    o.vao = o.vbo = o.ebo = 0;
}

Mesh& Mesh::operator=(Mesh&& o) noexcept {
    if (this != &o) {
        if (vao) glDeleteVertexArrays(1, &vao);
        if (vbo) glDeleteBuffers(1, &vbo);
        if (ebo) glDeleteBuffers(1, &ebo);
        vao = o.vao; vbo = o.vbo; ebo = o.ebo;
        indexCount = o.indexCount;
        center = o.center; radius = o.radius;
        o.vao = o.vbo = o.ebo = 0;
    }
    return *this;
}

Mesh::~Mesh() {
    if (vao) glDeleteVertexArrays(1, &vao);
    if (vbo) glDeleteBuffers(1, &vbo);
    if (ebo) glDeleteBuffers(1, &ebo);
}

namespace {

// Compute bounding sphere of a position array (first 3 floats per vertex).
void computeBounds(const std::vector<float>& v, glm::vec3& c, float& r) {
    if (v.empty()) { c = glm::vec3(0); r = 0; return; }
    glm::vec3 mn( 1e9f), mx(-1e9f);
    for (size_t i = 0; i < v.size(); i += 8) {
        glm::vec3 p(v[i], v[i+1], v[i+2]);
        mn = glm::min(mn, p);
        mx = glm::max(mx, p);
    }
    c = (mn + mx) * 0.5f;
    r = 0.0f;
    for (size_t i = 0; i < v.size(); i += 8) {
        glm::vec3 p(v[i], v[i+1], v[i+2]);
        r = std::max(r, glm::length(p - c));
    }
}

// Cache so the same primitive is built only once (the engine re-uses
// cubeMesh, sphereMesh, etc. across all systems).
std::unordered_map<std::string, Mesh>& cache() {
    static std::unordered_map<std::string, Mesh> c;
    return c;
}

} // namespace

namespace Meshes {
    void clearCache() {
        auto& c = cache();
        c.clear();
    }

    Mesh createMesh(const std::vector<float>& v,
                    const std::vector<unsigned int>& idx) {
        Mesh m;
        m.indexCount = (GLuint)idx.size();
        computeBounds(v, m.center, m.radius);

        glGenVertexArrays(1, &m.vao);
        glGenBuffers(1, &m.vbo);
        glGenBuffers(1, &m.ebo);
        glBindVertexArray(m.vao);

        glBindBuffer(GL_ARRAY_BUFFER, m.vbo);
        glBufferData(GL_ARRAY_BUFFER,
                     (GLsizeiptr)(v.size() * sizeof(float)),
                     v.data(), GL_STATIC_DRAW);

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m.ebo);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                     (GLsizeiptr)(idx.size() * sizeof(unsigned int)),
                     idx.data(), GL_STATIC_DRAW);

        // aPos
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                              (void*)0);
        glEnableVertexAttribArray(0);
        // aNormal
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                              (void*)(3 * sizeof(float)));
        glEnableVertexAttribArray(1);
        // aUV
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float),
                              (void*)(6 * sizeof(float)));
        glEnableVertexAttribArray(2);

        glBindVertexArray(0);
        return m;
    }

    // ---- Cube --------------------------------------------------------------
    Mesh cube() {
        // Returns immediately a copy of the cached mesh, but since Mesh is
        // move-only and we want to keep the cached one intact, callers below
        // use Meshes::cube() to fetch a fresh handle via the global move.
        std::vector<float> v;
        std::vector<unsigned int> idx;
        float s = 0.5f;
        // Each face: 4 verts. UVs in [0,1].
        struct Face { glm::vec3 n; glm::vec3 v[4]; };
        Face faces[6] = {
            { {0,0,1}, {{-s,-s, s},{ s,-s, s},{ s, s, s},{-s, s, s}} },
            { {0,0,-1},{{ s,-s,-s},{-s,-s,-s},{-s, s,-s},{ s, s,-s}} },
            { {-1,0,0},{{-s,-s,-s},{-s,-s, s},{-s, s, s},{-s, s,-s}} },
            { {1,0,0}, {{ s,-s, s},{ s,-s,-s},{ s, s,-s},{ s, s, s}} },
            { {0,1,0}, {{-s, s, s},{ s, s, s},{ s, s,-s},{-s, s,-s}} },
            { {0,-1,0},{{-s,-s,-s},{ s,-s,-s},{ s,-s, s},{-s,-s, s}} },
        };
        glm::vec2 uvs[4] = {{0,0},{1,0},{1,1},{0,1}};
        for (auto& f : faces) {
            unsigned int base = (unsigned int)(v.size() / 8);
            for (int i = 0; i < 4; i++) {
                v.insert(v.end(), {f.v[i].x, f.v[i].y, f.v[i].z,
                                   f.n.x, f.n.y, f.n.z,
                                   uvs[i].x, uvs[i].y});
            }
            idx.insert(idx.end(), {base, base+1, base+2,
                                   base, base+2, base+3});
        }
        return createMesh(v, idx);
    }

    // ---- Cylinder (side + 2 caps) ------------------------------------------
    Mesh cylinder(int seg) {
        std::vector<float> v;
        std::vector<unsigned int> idx;
        float r = 0.5f, h = 0.5f;
        // Side
        for (int i = 0; i <= seg; i++) {
            float a = 2.0f * (float)M_PI * i / seg;
            float x = r * cosf(a), z = r * sinf(a);
            float nx = cosf(a), nz = sinf(a);
            v.insert(v.end(), {x, -h, z, nx, 0, nz, (float)i / seg, 0.0f});
            v.insert(v.end(), {x,  h, z, nx, 0, nz, (float)i / seg, 1.0f});
        }
        for (int i = 0; i < seg; i++) {
            unsigned b = (unsigned)(i * 2);
            idx.insert(idx.end(), {b, b+1, b+2,
                                   b+2, b+1, b+3});
        }
        // Bottom cap
        unsigned bc = (unsigned)(v.size() / 8);
        v.insert(v.end(), {0, -h, 0, 0, -1, 0, 0.5f, 0.5f});
        for (int i = 0; i <= seg; i++) {
            float a = 2.0f * (float)M_PI * i / seg;
            v.insert(v.end(), {r*cosf(a), -h, r*sinf(a), 0, -1, 0,
                               0.5f + 0.5f * cosf(a), 0.5f + 0.5f * sinf(a)});
        }
        for (int i = 0; i < seg; i++) idx.insert(idx.end(), {bc, bc+i+2, bc+i+1});
        // Top cap
        unsigned tc = (unsigned)(v.size() / 8);
        v.insert(v.end(), {0, h, 0, 0, 1, 0, 0.5f, 0.5f});
        for (int i = 0; i <= seg; i++) {
            float a = 2.0f * (float)M_PI * i / seg;
            v.insert(v.end(), {r*cosf(a), h, r*sinf(a), 0, 1, 0,
                               0.5f + 0.5f * cosf(a), 0.5f + 0.5f * sinf(a)});
        }
        for (int i = 0; i < seg; i++) idx.insert(idx.end(), {tc, tc+i+1, tc+i+2});
        return createMesh(v, idx);
    }

    // ---- Cone --------------------------------------------------------------
    Mesh cone(int seg) {
        std::vector<float> v;
        std::vector<unsigned int> idx;
        float r = 0.5f, h = 0.5f;
        // Side
        v.insert(v.end(), {0, h, 0, 0, 1, 0, 0.5f, 1.0f});
        for (int i = 0; i <= seg; i++) {
            float a = 2.0f * (float)M_PI * i / seg;
            float x = r * cosf(a), z = r * sinf(a);
            float nx = cosf(a), nz = sinf(a), ny = r / h;
            float len = sqrtf(nx*nx + ny*ny + nz*nz);
            v.insert(v.end(), {x, -h, z, nx/len, ny/len, nz/len,
                               (float)i / seg, 0.0f});
        }
        for (int i = 0; i < seg; i++) idx.insert(idx.end(), {0, (unsigned)(i+1), (unsigned)(i+2)});
        // Base
        unsigned bc = (unsigned)(v.size() / 8);
        v.insert(v.end(), {0, -h, 0, 0, -1, 0, 0.5f, 0.5f});
        for (int i = 0; i <= seg; i++) {
            float a = 2.0f * (float)M_PI * i / seg;
            v.insert(v.end(), {r*cosf(a), -h, r*sinf(a), 0, -1, 0,
                               0.5f + 0.5f * cosf(a), 0.5f + 0.5f * sinf(a)});
        }
        for (int i = 0; i < seg; i++) idx.insert(idx.end(), {bc, bc+i+2, bc+i+1});
        return createMesh(v, idx);
    }

    // ---- Sphere (UV-sphere) ------------------------------------------------
    Mesh sphere(int stacks, int slices) {
        std::vector<float> v;
        std::vector<unsigned int> idx;
        float r = 0.5f;
        for (int i = 0; i <= stacks; i++) {
            float phi = (float)M_PI * i / stacks;
            for (int j = 0; j <= slices; j++) {
                float th = 2.0f * (float)M_PI * j / slices;
                float x = r * sinf(phi) * cosf(th);
                float y = r * cosf(phi);
                float z = r * sinf(phi) * sinf(th);
                v.insert(v.end(), {x, y, z, x/r, y/r, z/r,
                                   (float)j / slices, (float)i / stacks});
            }
        }
        for (int i = 0; i < stacks; i++) {
            for (int j = 0; j < slices; j++) {
                unsigned a = (unsigned)(i * (slices + 1) + j);
                unsigned b = a + (unsigned)(slices + 1);
                idx.insert(idx.end(), {a, b, a+1, a+1, b, b+1});
            }
        }
        return createMesh(v, idx);
    }

    // ---- Plane (single quad) -----------------------------------------------
    Mesh plane(float y, float size) {
        std::vector<float> v = {
            -size, y, -size, 0, 1, 0,  0, 0,
             size, y, -size, 0, 1, 0,  1, 0,
             size, y,  size, 0, 1, 0,  1, 1,
            -size, y,  size, 0, 1, 0,  0, 1,
        };
        std::vector<unsigned int> idx = {0, 1, 2, 0, 2, 3};
        return createMesh(v, idx);
    }

    // ---- Triangle (single face) --------------------------------------------
    Mesh triangle() {
        std::vector<float> v = {
             0.0f,  0.5f, 0.0f,  0, 0, 1,  0.5f, 1.0f,
            -0.5f, -0.5f, 0.0f,  0, 0, 1,  0.0f, 0.0f,
             0.5f, -0.5f, 0.0f,  0, 0, 1,  1.0f, 0.0f,
        };
        std::vector<unsigned int> idx = {0, 1, 2};
        return createMesh(v, idx);
    }

    // ---- Torus -------------------------------------------------------------
    Mesh torus(float majorR, float minorR, int majorSeg, int minorSeg) {
        std::vector<float> v;
        std::vector<unsigned int> idx;
        for (int i = 0; i <= majorSeg; i++) {
            float u = (float)i / majorSeg * 2.0f * (float)M_PI;
            glm::vec3 center(cosf(u) * majorR, 0, sinf(u) * majorR);
            for (int j = 0; j <= minorSeg; j++) {
                float w = (float)j / minorSeg * 2.0f * (float)M_PI;
                glm::vec3 normal(cosf(u) * cosf(w), sinf(w), sinf(u) * cosf(w));
                glm::vec3 p = center + normal * minorR;
                v.insert(v.end(), {p.x, p.y, p.z, normal.x, normal.y, normal.z,
                                   (float)i / majorSeg, (float)j / minorSeg});
            }
        }
        int ringStride = minorSeg + 1;
        for (int i = 0; i < majorSeg; i++) {
            for (int j = 0; j < minorSeg; j++) {
                unsigned a = (unsigned)(i * ringStride + j);
                unsigned b = a + (unsigned)ringStride;
                idx.insert(idx.end(), {a, b, a+1, a+1, b, b+1});
            }
        }
        return createMesh(v, idx);
    }

    // ---- Capsule (lathed profile) ------------------------------------------
    // profile: list of (y, r) pairs from y=-h to y=+h.
    Mesh capsule(const std::vector<glm::vec2>& profile, int seg) {
        std::vector<float> v;
        std::vector<unsigned int> idx;
        for (int i = 0; i <= seg; i++) {
            float a = 2.0f * (float)M_PI * i / seg;
            float cx = cosf(a), cz = sinf(a);
            for (auto& pr : profile) {
                float y = pr.x, r = pr.y;
                float px = r * cx, pz = r * cz;
                // Approx normal: radial. OK for capsule-like shapes.
                float nlen = sqrtf(cx * cx + cz * cz);
                std::vector<float> row = {px, y, pz, cx / nlen, 0.0f, cz / nlen,
                                          (float)i / seg, (y + 0.5f)};
                for (float f : row) v.push_back(f);
            }
        }
        int profileSize = (int)profile.size();
        for (int i = 0; i < seg; i++) {
            for (int j = 0; j < profileSize - 1; j++) {
                unsigned a = (unsigned)(i * profileSize + j);
                unsigned b = a + (unsigned)profileSize;
                idx.push_back(a); idx.push_back(b); idx.push_back(a+1);
                idx.push_back(a+1); idx.push_back(b); idx.push_back(b+1);
            }
        }
        return createMesh(v, idx);
    }

    // ---- Lathe body from (y, r) profile ------------------------------------
    Mesh latheBody(const std::vector<glm::vec2>& profile, int seg) {
        std::vector<float> v;
        std::vector<unsigned int> idx;
        for (int i = 0; i <= seg; i++) {
            float a = 2.0f * (float)M_PI * i / seg;
            float cx = cosf(a), cz = sinf(a);
            for (auto& p : profile) {
                float y = p.x, r = p.y;
                // Numerical derivative for normal in (r,y) space.
                v.insert(v.end(), {r * cx, y, r * cz, cx, 0.0f, cz,
                                   (float)i / seg, (y + 1.0f) * 0.5f});
            }
        }
        int P = (int)profile.size();
        for (int i = 0; i < seg; i++) {
            for (int j = 0; j < P - 1; j++) {
                unsigned a = (unsigned)(i * P + j);
                unsigned b = a + (unsigned)P;
                idx.insert(idx.end(), {a, b, a+1, a+1, b, b+1});
            }
        }
        return createMesh(v, idx);
    }

    // ---- Ellipsoid ---------------------------------------------------------
    Mesh ellipsoid(float rx, float ry, float rz, int seg, int stacks) {
        std::vector<float> v;
        std::vector<unsigned int> idx;
        for (int i = 0; i <= stacks; i++) {
            float phi = (float)M_PI * i / stacks;
            for (int j = 0; j <= seg; j++) {
                float th = 2.0f * (float)M_PI * j / seg;
                float sx = sinf(phi) * cosf(th);
                float sy = cosf(phi);
                float sz = sinf(phi) * sinf(th);
                v.insert(v.end(), {sx * rx, sy * ry, sz * rz,
                                   sx, sy, sz,
                                   (float)j / seg, (float)i / stacks});
            }
        }
        for (int i = 0; i < stacks; i++) {
            for (int j = 0; j < seg; j++) {
                unsigned a = (unsigned)(i * (seg + 1) + j);
                unsigned b = a + (unsigned)(seg + 1);
                idx.insert(idx.end(), {a, b, a+1, a+1, b, b+1});
            }
        }
        return createMesh(v, idx);
    }

    // ---- Ring (annulus) ----------------------------------------------------
    Mesh ring(float innerR, float outerR, int seg) {
        std::vector<float> v;
        std::vector<unsigned int> idx;
        for (int i = 0; i <= seg; i++) {
            float a = 2.0f * (float)M_PI * i / seg;
            float cx = cosf(a), cz = sinf(a);
            v.insert(v.end(), {innerR * cx, 0, innerR * cz, 0, 1, 0,
                               0.5f + 0.5f * cx, 0.5f + 0.5f * cz});
            v.insert(v.end(), {outerR * cx, 0, outerR * cz, 0, 1, 0,
                               0.5f + 0.5f * outerR * cx / outerR,
                               0.5f + 0.5f * outerR * cz / outerR});
        }
        for (int i = 0; i < seg; i++) {
            unsigned b = (unsigned)(i * 2);
            idx.insert(idx.end(), {b, b+1, b+2, b+2, b+1, b+3});
        }
        return createMesh(v, idx);
    }

    // ---- Tube (open cylinder) ---------------------------------------------
    Mesh tube(int seg, int rings, float radius, float length) {
        std::vector<float> v;
        std::vector<unsigned int> idx;
        for (int i = 0; i <= rings; i++) {
            float t = (float)i / rings;
            float y = -length * 0.5f + t * length;
            for (int j = 0; j <= seg; j++) {
                float a = 2.0f * (float)M_PI * j / seg;
                float cx = cosf(a), cz = sinf(a);
                v.insert(v.end(), {radius * cx, y, radius * cz,
                                   cx, 0, cz,
                                   (float)j / seg, t});
            }
        }
        int ringStride = seg + 1;
        for (int i = 0; i < rings; i++) {
            for (int j = 0; j < seg; j++) {
                unsigned a = (unsigned)(i * ringStride + j);
                unsigned b = a + (unsigned)ringStride;
                idx.insert(idx.end(), {a, b, a+1, a+1, b, b+1});
            }
        }
        return createMesh(v, idx);
    }

    // ---- Hemisphere (for cave ceiling) -------------------------------------
    Mesh hemisphere(int stacks, int slices, float r) {
        std::vector<float> v;
        std::vector<unsigned int> idx;
        for (int i = 0; i <= stacks; i++) {
            float phi = (float)M_PI * 0.5f * i / stacks; // 0..pi/2
            for (int j = 0; j <= slices; j++) {
                float th = 2.0f * (float)M_PI * j / slices;
                float sx = sinf(phi) * cosf(th);
                float sy = cosf(phi);
                float sz = sinf(phi) * sinf(th);
                v.insert(v.end(), {sx * r, sy * r, sz * r,
                                   sx, sy, sz,
                                   (float)j / slices, (float)i / stacks});
            }
        }
        for (int i = 0; i < stacks; i++) {
            for (int j = 0; j < slices; j++) {
                unsigned a = (unsigned)(i * (slices + 1) + j);
                unsigned b = a + (unsigned)(slices + 1);
                idx.insert(idx.end(), {a, b, a+1, a+1, b, b+1});
            }
        }
        return createMesh(v, idx);
    }

    // ---- Rock (deformed low-poly chunk) -----------------------------------
    // Start from a subdivided icosphere-like shape, displace each vertex
    // outward by a small amount, and emit triangles. We use a cube sphere
    // parametrization so UVs and normals are well-defined.
    Mesh rock(int subdivisions, float displace) {
        const int n = 1 << subdivisions;            // subdivisions per axis
        const int N = n + 1;
        // Build (n+1)^3 points on a [-1, 1]^3 grid, then push them onto a
        // sphere of radius 0.5 and add per-vertex noise.
        std::vector<glm::vec3> pts;
        pts.reserve(N * N * N);
        for (int i = 0; i < N; i++) {
            for (int j = 0; j < N; j++) {
                for (int k = 0; k < N; k++) {
                    float x = ((float)i / n) * 2.0f - 1.0f;
                    float y = ((float)j / n) * 2.0f - 1.0f;
                    float z = ((float)k / n) * 2.0f - 1.0f;
                    glm::vec3 dir = glm::normalize(glm::vec3(x, y, z));
                    float d = displace * (Rng::randFloat() * 2.0f - 1.0f);
                    pts.push_back(dir * (0.5f + d));
                }
            }
        }
        auto at = [&](int i, int j, int k) -> glm::vec3& {
            return pts[(i * N + j) * N + k];
        };

        std::vector<float> v;
        std::vector<unsigned int> idx;
        auto push = [&](glm::vec3 p) {
            glm::vec3 nrm = glm::normalize(p);
            v.insert(v.end(), {p.x, p.y, p.z, nrm.x, nrm.y, nrm.z,
                               0.5f + p.x, 0.5f + p.z});
        };
        auto quad = [&](glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
            unsigned base = (unsigned)(v.size() / 8);
            push(a); push(b); push(c); push(d);
            idx.insert(idx.end(), {base, base+1, base+2,
                                   base, base+2, base+3});
        };

        // 6 faces of the cube, each (n x n) quads
        // +X
        for (int j = 0; j < n; j++) for (int k = 0; k < n; k++) {
            quad(at(n, j,   k),  at(n, j,   k+1),
                 at(n, j+1, k+1), at(n, j+1, k));
        }
        // -X
        for (int j = 0; j < n; j++) for (int k = 0; k < n; k++) {
            quad(at(0, j,   k),   at(0, j+1, k),
                 at(0, j+1, k+1), at(0, j,   k+1));
        }
        // +Y
        for (int i = 0; i < n; i++) for (int k = 0; k < n; k++) {
            quad(at(i,   n, k), at(i,   n, k+1),
                 at(i+1, n, k+1), at(i+1, n, k));
        }
        // -Y
        for (int i = 0; i < n; i++) for (int k = 0; k < n; k++) {
            quad(at(i,   0, k),   at(i+1, 0, k),
                 at(i+1, 0, k+1), at(i,   0, k+1));
        }
        // +Z
        for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) {
            quad(at(i,   j+1, n), at(i+1, j+1, n),
                 at(i+1, j,   n), at(i,   j,   n));
        }
        // -Z
        for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) {
            quad(at(i,   j+1, 0), at(i,   j,   0),
                 at(i+1, j,   0), at(i+1, j+1, 0));
        }

        return createMesh(v, idx);
    }
}

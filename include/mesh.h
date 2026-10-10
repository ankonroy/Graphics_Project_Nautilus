// ============================================================================
// mesh.h
// ----------------------------------------------------------------------------
// Mesh struct + procedural generators. All geometry is interleaved
//   [px py pz  nx ny nz  u v]
// with attribute locations 0=position, 1=normal, 2=uv. The same vertex
// layout is used everywhere so a single VAO format covers every object.
// ============================================================================
#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <vector>

struct Mesh {
    GLuint vao = 0, vbo = 0, ebo = 0;
    GLuint indexCount = 0;
    // Bounding sphere (computed at build time) - useful for ray casts.
    glm::vec3  center{0.0f};
    float      radius = 0.0f;

    ~Mesh();
    Mesh() = default;
    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;
};

namespace Meshes {
    // Internally: position(3) + normal(3) + uv(2) = 8 floats per vertex.
    Mesh createMesh(const std::vector<float>& v,
                    const std::vector<unsigned int>& idx);

    // Primitive generators. All centered at origin, unit-ish size.
    Mesh cube();
    Mesh cylinder(int seg = 32);
    Mesh cone(int seg = 32);
    Mesh sphere(int stacks = 16, int slices = 32);
    Mesh plane(float y, float size);
    Mesh triangle();
    Mesh torus(float majorR, float minorR, int majorSeg = 48, int minorSeg = 16);
    // Capsule (lathe profile) used for hull-shaped objects. profile is a
    // list of glm::vec2 where x = y-coord and y = radius at that height.
    Mesh capsule(const std::vector<glm::vec2>& profile, int seg = 32);
    // Low-poly icosphere-like rock. `displace` adds per-vertex noise.
    Mesh rock(int subdivisions = 1, float displace = 0.18f);
    // Lathe body for fish: profile is (y, r) pairs.
    Mesh latheBody(const std::vector<glm::vec2>& profile, int seg = 24);
    // Elongated ellipsoid from a lathe.
    Mesh ellipsoid(float rx, float ry, float rz, int seg = 24, int stacks = 16);
    // Ring (annulus) on a horizontal plane.
    Mesh ring(float innerR, float outerR, int seg = 64);
    // Open cylindrical tube (no caps) - used for cave tunnel walls.
    Mesh tube(int seg = 32, int rings = 32, float radius = 1.0f,
              float length = 1.0f);
    // Half-sphere dome (for cave ceiling).
    Mesh hemisphere(int stacks = 12, int slices = 24, float r = 1.0f);

    // Free GPU resources for all loaded meshes.
    void clearCache();
}

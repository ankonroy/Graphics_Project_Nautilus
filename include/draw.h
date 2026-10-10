// ============================================================================
// draw.h - small shared rendering helpers (model+material, draw mesh, align).
// ============================================================================
#pragma once

#include "mesh.h"
#include "shader.h"
#include <glm/glm.hpp>

namespace Draw {
    // Issues glDrawElements for the supplied mesh.
    void mesh(const Mesh& m);

    // Builds normal matrix and uploads model/color/shininess/emissive/
    // texture flags in one call. texture=0 means no texture.
    void modelMaterial(const glm::mat4& model,
                       const glm::vec3& color,
                       float shininess,
                       float emissive = 0.0f,
                       GLuint texture = 0,
                       bool  useTex   = false);

    // Align local +Y axis to the supplied world direction.
    glm::mat4 alignLocalY(const glm::vec3& dir);

    // Draw a mesh N times with per-instance transform callbacks.
    // cb receives (instance index) and returns a model matrix.
    template <typename Cb>
    void instanced(const Mesh& m, int count, Cb cb) {
        for (int i = 0; i < count; i++) cb(i);
        // (Already drawn inside cb via Draw::mesh; this template is
        //  just sugar so call sites read cleanly.)
    }
}

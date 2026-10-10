// ============================================================================
// shader.h
// ----------------------------------------------------------------------------
// Owns the single GLSL program used for the entire scene, plus uniform
// setters. Kept header-only in interface so the rest of the engine does not
// have to juggle a global pointer for every call.
// ============================================================================
#pragma once

#include <GL/glew.h>
#include <glm/glm.hpp>
#include <string>

namespace Shader {
    // Lifecycle.
    bool   init();            // compile + link. Returns true on success.
    void   shutdown();
    GLuint program();         // the linked program (0 if init failed)
    void   use();             // glUseProgram wrapper

    // Uniform setters (location lookup cached lazily inside).
    void setMat4  (const std::string& n, const glm::mat4& m);
    void setMat3  (const std::string& n, const glm::mat3& m);
    void setVec3  (const std::string& n, const glm::vec3& v);
    void setVec4  (const std::string& n, const glm::vec4& v);
    void setFloat (const std::string& n, float v);
    void setInt   (const std::string& n, int v);
    void setBool  (const std::string& n, bool v);

    // Convenience: model + normal matrix + material.
    void setModelAndMaterial(const glm::mat4& model,
                             const glm::vec3& color,
                             float shininess);

    // For uniform location caching.
    GLint location(const std::string& n);
}

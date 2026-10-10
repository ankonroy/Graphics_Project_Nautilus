// ============================================================================
// draw.cpp
// ============================================================================
#include "draw.h"
#include "texture.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Draw {
    void mesh(const Mesh& m) {
        glBindVertexArray(m.vao);
        glDrawElements(GL_TRIANGLES, m.indexCount, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);
    }

    void modelMaterial(const glm::mat4& model,
                       const glm::vec3& color,
                       float shininess,
                       float emissive,
                       GLuint texture,
                       bool  useTex) {
        glm::mat3 nMat = glm::mat3(glm::transpose(glm::inverse(model)));
        Shader::setMat4("model", model);
        Shader::setMat3("normalMatrix", nMat);
        Shader::setVec3("objectColor", color);
        Shader::setFloat("objectShininess", shininess);
        Shader::setFloat("emissiveBoost", emissive);
        if (useTex && texture) {
            Textures::bind(texture);
            Shader::setInt("tex", 0);
            Shader::setBool("useTexture", true);
        } else {
            Shader::setBool("useTexture", false);
        }
    }

    glm::mat4 alignLocalY(const glm::vec3& dir) {
        glm::vec3 n = glm::normalize(dir);
        glm::vec3 y(0, 1, 0);
        float d = glm::dot(y, n);
        if (d > 0.99999f) return glm::mat4(1.0f);
        if (d < -0.99999f) return glm::rotate(glm::mat4(1.0f), (float)M_PI, glm::vec3(1, 0, 0));
        glm::vec3 axis = glm::normalize(glm::cross(y, n));
        float angle = acosf(std::clamp(d, -1.0f, 1.0f));
        return glm::rotate(glm::mat4(1.0f), angle, axis);
    }
}

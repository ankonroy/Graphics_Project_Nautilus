// ============================================================================
// shader.cpp
// ============================================================================
#include "shader.h"
#include <glm/gtc/type_ptr.hpp>
#include <unordered_map>
#include <iostream>

namespace {
    GLuint gProgram = 0;
    std::unordered_map<std::string, GLint> gLocations;

    const char* VERT_SRC = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aUV;

uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;
uniform mat3 normalMatrix;

uniform vec3 viewPos;
uniform vec3 ambientColor;
uniform vec3 dirLightDir;
uniform vec3 dirLightColor;
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform vec3 spotLightColor;
uniform bool headlightOn;
uniform int  shadingMode;
uniform vec3 objectColor;
uniform float objectShininess;
uniform float emissiveBoost;

uniform vec4 clipPlane;

out vec3 FragPos;
out vec3 Normal;
out vec3 GouraudColor;
out vec2 UV;

vec3 computeLighting(vec3 N, vec3 V) {
    vec3 ambient = ambientColor * objectColor;

    vec3 L = normalize(-dirLightDir);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * dirLightColor * objectColor;
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), objectShininess);
    vec3 specular = 0.45 * spec * dirLightColor;

    vec3 spotResult = vec3(0.0);
    if (headlightOn) {
        vec3 Ls = normalize(spotLightPos - FragPos);
        float dist = length(spotLightPos - FragPos);
        float att = 1.0 / (1.0 + 0.004 * dist + 0.0003 * dist * dist);

        float theta = dot(Ls, normalize(-spotLightDir));
        float innerC = cos(radians(28.0));
        float outerC = cos(radians(45.0));
        float intensity = clamp((theta - outerC) / (innerC - outerC), 0.0, 1.0);

        float diffS = max(dot(N, Ls), 0.0);
        vec3 Rs = reflect(-Ls, N);
        float specS = pow(max(dot(V, Rs), 0.0), objectShininess);

        vec3 ambS = 0.30 * spotLightColor * objectColor;
        vec3 difS = 1.40 * diffS * spotLightColor * objectColor;
        vec3 speS = 1.00 * specS * spotLightColor;
        spotResult = (ambS + difS + speS) * att * intensity;
    }

    return ambient + diffuse + specular + spotResult
         + emissiveBoost * objectColor;
}

void main() {
    FragPos = vec3(model * vec4(aPos, 1.0));
    Normal  = normalMatrix * aNormal;
    vec3 V = normalize(viewPos - FragPos);
    UV      = aUV;

    if (shadingMode == 1)
        GouraudColor = computeLighting(normalize(Normal), V);
    else
        GouraudColor = vec3(0.0);

    gl_Position = projection * view * vec4(FragPos, 1.0);

    gl_ClipDistance[0] = dot(vec4(FragPos, 1.0), clipPlane);
}
)";

    const char* FRAG_SRC = R"(
#version 330 core
in vec3 FragPos;
in vec3 Normal;
in vec3 GouraudColor;
in vec2 UV;

uniform vec3 viewPos;
uniform vec3 ambientColor;
uniform vec3 dirLightDir;
uniform vec3 dirLightColor;
uniform vec3 spotLightPos;
uniform vec3 spotLightDir;
uniform vec3 spotLightColor;
uniform bool headlightOn;
uniform int  shadingMode;
uniform vec3 objectColor;
uniform float objectShininess;
uniform float emissiveBoost;
uniform vec3  fogColor;
uniform float fogDensity;

uniform bool  useTexture;
uniform sampler2D tex;

// ---- Tunnel cut-out ----
// When tunnelActive is true, any fragment within `tunnelR` of the segment
// [tunnelA, tunnelB] is discarded. Used to carve a hole in the island's
// sand and the chamber dome where the cave tunnel passes through.
uniform bool  tunnelActive;
uniform vec3  tunnelA;
uniform vec3  tunnelB;
uniform float tunnelR;

out vec4 FragColor;

vec3 computeLighting(vec3 N, vec3 V) {
    vec3 ambient = ambientColor * objectColor;

    vec3 L = normalize(-dirLightDir);
    float diff = max(dot(N, L), 0.0);
    vec3 diffuse = diff * dirLightColor * objectColor;
    vec3 R = reflect(-L, N);
    float spec = pow(max(dot(V, R), 0.0), objectShininess);
    vec3 specular = 0.45 * spec * dirLightColor;

    vec3 spotResult = vec3(0.0);
    if (headlightOn) {
        vec3 Ls = normalize(spotLightPos - FragPos);
        float dist = length(spotLightPos - FragPos);
        float att = 1.0 / (1.0 + 0.004 * dist + 0.0003 * dist * dist);

        float theta = dot(Ls, normalize(-spotLightDir));
        float innerC = cos(radians(28.0));
        float outerC = cos(radians(45.0));
        float intensity = clamp((theta - outerC) / (innerC - outerC), 0.0, 1.0);

        float diffS = max(dot(N, Ls), 0.0);
        vec3 Rs = reflect(-Ls, N);
        float specS = pow(max(dot(V, Rs), 0.0), objectShininess);

        vec3 ambS = 0.30 * spotLightColor * objectColor;
        vec3 difS = 1.40 * diffS * spotLightColor * objectColor;
        vec3 speS = 1.00 * specS * spotLightColor;
        spotResult = (ambS + difS + speS) * att * intensity;
    }

    return ambient + diffuse + specular + spotResult
         + emissiveBoost * objectColor;
}

void main() {
    // ---- Tunnel cut-out test ----
    if (tunnelActive) {
        vec3 d = tunnelB - tunnelA;
        float len2 = dot(d, d);
        if (len2 > 1e-4) {
            float t = clamp(dot(FragPos - tunnelA, d) / len2, 0.0, 1.0);
            vec3 closest = tunnelA + d * t;
            if (length(FragPos - closest) < tunnelR) discard;
        }
    }

    vec3 baseColor = objectColor;
    if (useTexture) {
        baseColor = baseColor * texture(tex, UV).rgb;
    }

    vec3 N = normalize(Normal);

    if (shadingMode == 0) {
        N = normalize(cross(dFdx(FragPos), dFdy(FragPos)));
    }
    if (!gl_FrontFacing) N = -N;

    vec3 V = normalize(viewPos - FragPos);
    vec3 color = (shadingMode == 1) ? GouraudColor : computeLighting(N, V);

    if (useTexture && shadingMode == 1) {
        color = color * texture(tex, UV).rgb;
    }

    float dist = length(viewPos - FragPos);
    float fogFactor = clamp(exp(-fogDensity * dist), 0.0, 1.0);
    color = mix(fogColor, color, fogFactor);

    FragColor = vec4(color, useTexture ? texture(tex, UV).a : 1.0);
}
)";

    GLuint compileShader(GLenum type, const char* src) {
        GLuint s = glCreateShader(type);
        glShaderSource(s, 1, &src, nullptr);
        glCompileShader(s);
        GLint ok = 0;
        glGetShaderiv(s, GL_COMPILE_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetShaderInfoLog(s, 1024, nullptr, log);
            std::cerr << "[Shader] compile error: " << log << std::endl;
            glDeleteShader(s);
            return 0;
        }
        return s;
    }
}

namespace Shader {
    bool init() {
        gProgram = 0;
        gLocations.clear();

        GLuint v = compileShader(GL_VERTEX_SHADER,   VERT_SRC);
        GLuint f = compileShader(GL_FRAGMENT_SHADER, FRAG_SRC);
        if (!v || !f) return false;

        gProgram = glCreateProgram();
        glAttachShader(gProgram, v);
        glAttachShader(gProgram, f);
        glLinkProgram(gProgram);
        GLint ok = 0;
        glGetProgramiv(gProgram, GL_LINK_STATUS, &ok);
        if (!ok) {
            char log[1024];
            glGetProgramInfoLog(gProgram, 1024, nullptr, log);
            std::cerr << "[Shader] link error: " << log << std::endl;
            glDeleteProgram(gProgram);
            gProgram = 0;
            return false;
        }
        glDeleteShader(v);
        glDeleteShader(f);
        return true;
    }

    void shutdown() {
        if (gProgram) glDeleteProgram(gProgram);
        gProgram = 0;
        gLocations.clear();
    }

    GLuint program() { return gProgram; }
    void   use()     { glUseProgram(gProgram); }

    GLint location(const std::string& n) {
        auto it = gLocations.find(n);
        if (it != gLocations.end()) return it->second;
        GLint loc = glGetUniformLocation(gProgram, n.c_str());
        gLocations[n] = loc;
        return loc;
    }

    void setMat4 (const std::string& n, const glm::mat4& m) { glUniformMatrix4fv(location(n), 1, GL_FALSE, glm::value_ptr(m)); }
    void setMat3 (const std::string& n, const glm::mat3& m) { glUniformMatrix3fv(location(n), 1, GL_FALSE, glm::value_ptr(m)); }
    void setVec3 (const std::string& n, const glm::vec3& v) { glUniform3fv(location(n), 1, glm::value_ptr(v)); }
    void setVec4 (const std::string& n, const glm::vec4& v) { glUniform4fv(location(n), 1, glm::value_ptr(v)); }
    void setFloat(const std::string& n, float v) { glUniform1f(location(n), v); }
    void setInt  (const std::string& n, int v)   { glUniform1i(location(n), v); }
    void setBool (const std::string& n, bool v)  { glUniform1i(location(n), v ? 1 : 0); }

    void setModelAndMaterial(const glm::mat4& model,
                             const glm::vec3& color,
                             float shininess) {
        glm::mat3 nMat = glm::mat3(glm::transpose(glm::inverse(model)));
        setMat4("model", model);
        setMat3("normalMatrix", nMat);
        setVec3("objectColor", color);
        setFloat("objectShininess", shininess);
        setFloat("emissiveBoost", 0.0f);
        setBool("useTexture", false);
        setInt("shadingMode", 0);
    }
}
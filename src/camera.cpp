// ============================================================================
// camera.cpp
// ----------------------------------------------------------------------------
// Four cameras, cycled with C:
//   0 = Chase   : third-person, behind the sub
//   1 = Bow     : first-person at the nose
//   2 = Interior: free-fly inside the cabin
//   3 = Side    : level with the sub, off to one side, showing the full profile
// ============================================================================
#include "camera.h"
#include "input.h"
#include "submarine.h"
#include "cave.h"
#include "constants.h"
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <cmath>
#include <algorithm>
#include <iostream>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace {
    glm::vec3 intPos      = glm::vec3(0.0f, 1.0f, 1.0f);
    glm::vec3 intYawPitch = glm::vec3(0.0f, 0.0f, 0.0f);

    float g_shakeMag  = 0.0f;
    float g_shakeTime = 0.0f;
    glm::vec3 g_shakeOffset{0};

    std::vector<Camera::AABB> g_colliders;
    Camera::AABB g_room{ glm::vec3(-1.2f, 0.0f, -2.0f),
                         glm::vec3( 1.2f, 1.8f,  2.0f) };

    glm::mat4 subTransform() {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), Sub::pos);
        m = glm::rotate(m, glm::radians(Sub::yawDeg),   glm::vec3(0, 1, 0));
        m = glm::rotate(m, glm::radians(Sub::pitchDeg), glm::vec3(1, 0, 0));
        return m;
    }
}

namespace Camera {
    void resetInterior() {
        intPos      = glm::vec3(0.0f, 1.0f, 1.0f);
        intYawPitch = glm::vec3(0.0f, 0.0f, 0.0f);
    }

    void addInteriorCollider(const AABB& b) { g_colliders.push_back(b); }
    void clearInteriorColliders()           { g_colliders.clear(); }
    void setInteriorBounds(const AABB& b)   { g_room = b; }

    glm::vec3 interiorPos() { return intPos; }

    glm::vec3 interiorForward() {
        float yaw = glm::radians(intYawPitch.x);
        float pit = glm::radians(intYawPitch.y);
        glm::vec3 f;
        f.x = -sinf(yaw) * cosf(pit);
        f.y =  sinf(pit);
        f.z = -cosf(yaw) * cosf(pit);
        return glm::normalize(f);
    }

    void addShake(float m) {
        g_shakeMag  = std::max(g_shakeMag, m);
        g_shakeTime = 0.0f;
    }
    glm::vec3 currentShakeOffset() { return g_shakeOffset; }

    void update(float dt) {
        // ---- Interior camera movement (only when camMode == 2) ----
        if (Input::camMode() == 2) {
            glm::vec3 fwd   = interiorForward();
            glm::vec3 right = glm::normalize(
                glm::cross(fwd, glm::vec3(0,1,0)));
            glm::vec3 up    = glm::vec3(0,1,0);
            float speed = 2.5f * dt;

            if (Input::down(GLFW_KEY_UP))    intPos += fwd   * speed;
            if (Input::down(GLFW_KEY_DOWN))  intPos -= fwd   * speed;
            if (Input::down(GLFW_KEY_LEFT))  intPos -= right * speed;
            if (Input::down(GLFW_KEY_RIGHT)) intPos += right * speed;
            if (Input::down(GLFW_KEY_PAGE_UP))   intPos += up * speed;
            if (Input::down(GLFW_KEY_PAGE_DOWN)) intPos -= up * speed;

            float lookSpeed = 80.0f * dt;
            if (Input::down(GLFW_KEY_I)) intYawPitch.y += lookSpeed;
            if (Input::down(GLFW_KEY_K)) intYawPitch.y -= lookSpeed;
            if (Input::down(GLFW_KEY_J)) intYawPitch.x += lookSpeed;
            if (Input::down(GLFW_KEY_L)) intYawPitch.x -= lookSpeed;
            intYawPitch.y = std::clamp(intYawPitch.y, -80.0f, 80.0f);

            // Clamp to room AABB.
            intPos = glm::clamp(intPos,
                                g_room.min + glm::vec3(0.20f),
                                g_room.max - glm::vec3(0.20f));

            // Sphere-vs-AABB collision against interior furniture.
            const float r = 0.25f;
            for (auto& b : g_colliders) {
                glm::vec3 closest;
                closest.x = std::clamp(intPos.x, b.min.x, b.max.x);
                closest.y = std::clamp(intPos.y, b.min.y, b.max.y);
                closest.z = std::clamp(intPos.z, b.min.z, b.max.z);
                glm::vec3 d = intPos - closest;
                float dist2 = glm::dot(d, d);
                if (dist2 < r * r) {
                    if (dist2 < 1e-6f) {
                        float dxMin = intPos.x - b.min.x;
                        float dxMax = b.max.x - intPos.x;
                        float dyMin = intPos.y - b.min.y;
                        float dyMax = b.max.y - intPos.y;
                        float dzMin = intPos.z - b.min.z;
                        float dzMax = b.max.z - intPos.z;
                        float m = std::min({dxMin, dxMax, dyMin, dyMax,
                                            dzMin, dzMax});
                        if      (m == dxMin) intPos.x = b.min.x - r;
                        else if (m == dxMax) intPos.x = b.max.x + r;
                        else if (m == dyMin) intPos.y = b.min.y - r;
                        else if (m == dyMax) intPos.y = b.max.y + r;
                        else if (m == dzMin) intPos.z = b.min.z - r;
                        else                 intPos.z = b.max.z + r;
                    } else {
                        glm::vec3 n = d / std::sqrt(dist2);
                        intPos = closest + n * r;
                    }
                }
            }
        }

        // ---- Shake decay ----
        if (g_shakeMag > 0.001f) {
            g_shakeTime += dt;
            float t = g_shakeTime;
            g_shakeOffset = glm::vec3(
                std::sin(t * 47.0f)          * g_shakeMag,
                std::sin(t * 71.0f + 1.7f)   * g_shakeMag,
                std::sin(t * 31.0f + 3.3f)   * g_shakeMag);
            g_shakeMag *= std::pow(0.05f, dt);
        } else {
            g_shakeOffset = glm::vec3(0);
        }
    }

    glm::mat4 buildView(const glm::vec3& subPos,
                        const glm::vec3& subForward,
                        const glm::vec3& subUp,
                        glm::vec3& outEye) {
        glm::mat4 view;
        int mode = Input::camMode();

        if (mode == 0) {
            // ================================================================
            // CHASE CAMERA
            // ================================================================
            glm::vec3 camPos = subPos
                - subForward * 14.0f
                + subUp      *  5.0f;
            glm::vec3 target = subPos + subForward * 3.0f;
            camPos += g_shakeOffset;
            outEye = camPos;
            view = glm::lookAt(camPos, target, subUp);
        }
        else if (mode == 1) {
            // ================================================================
            // BOW CAMERA — first-person from the nose
            // ================================================================
            float forwardOffset = Constants::SUB_LENGTH * 0.5f + 0.6f;
            glm::vec3 camPos = subPos
                + subForward * forwardOffset
                + subUp      * 0.2f;
            glm::vec3 target = camPos + subForward;
            camPos += g_shakeOffset;
            outEye = camPos;
            view = glm::lookAt(camPos, target, subUp);
        }
        else if (mode == 3) {
            // ================================================================
            // SIDE CAMERA — full profile view of the submarine.
            //
            // The camera sits off the sub's right side, level with the
            // hull, looking straight at the sub's center. Distance scales
            // with sub length so the whole silhouette fits regardless of
            // how long the hull is.
            // ================================================================
            glm::vec3 right = glm::normalize(glm::cross(subForward, subUp));
            float d = Constants::SUB_LENGTH * 1.05f;
            glm::vec3 camPos = subPos + right * d + subUp * 1.5f;
            glm::vec3 target = subPos;
            camPos += g_shakeOffset;
            outEye = camPos;
            view = glm::lookAt(camPos, target, subUp);
        }
        else {
            // ================================================================
            // INTERIOR CAMERA — free-fly inside the cabin
            // ================================================================
            glm::mat4 subBase = subTransform();
            glm::vec3 eyeLocal = intPos + g_shakeOffset;
            glm::vec3 fwdLocal = interiorForward();
            glm::vec3 eyeWorld = glm::vec3(
                subBase * glm::vec4(eyeLocal, 1.0f));
            glm::vec3 fwdWorld = glm::normalize(
                glm::vec3(subBase * glm::vec4(fwdLocal, 0.0f)));
            glm::vec3 upWorld  = glm::normalize(
                glm::vec3(subBase * glm::vec4(0.0f, 1.0f, 0.0f, 0.0f)));
            outEye = eyeWorld;
            view = glm::lookAt(eyeWorld, eyeWorld + fwdWorld, upWorld);
        }
        return view;
    }
}
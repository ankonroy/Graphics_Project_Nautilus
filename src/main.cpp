// ============================================================================
// Nautilus: Interactive 3D Submarine Explorer - Game Edition
// ============================================================================
#include <GL/glew.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <cstdio>

#include "constants.h"
#include "rng.h"
#include "shader.h"
#include "mesh.h"
#include "texture.h"
#include "draw.h"
#include "input.h"
#include "camera.h"
#include "submarine.h"
#include "torpedo.h"
#include "mine.h"
#include "fish.h"
#include "rock.h"
#include "checkpoint.h"
#include "island.h"
#include "cave.h"
#include "interior.h"
#include "particles.h"
#include "hud.h"
#include "game.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static void drawFloor();
static void drawWater();
static void drawSonar(int fbW, int fbH, const glm::vec3& subPos, float yaw);
static void renderFrame(float dt, float now,
                        const glm::vec3& subPos,
                        const glm::vec3& subForward,
                        const glm::vec3& subUp,
                        const glm::mat4& view,
                        const glm::vec3& eye);
static void updateLightingUniforms(const glm::vec3& subPos,
                                   const glm::vec3& subForward,
                                   const glm::vec3& eye);

int main() {
    if (!glfwInit()) { std::cerr << "[Main] GLFW init failed\n"; return -1; }
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_SAMPLES, 4);

    GLFWwindow* window = glfwCreateWindow(Constants::WINDOW_W,
                                          Constants::WINDOW_H,
                                          "Nautilus - Submarine Explorer",
                                          nullptr, nullptr);
    if (!window) {
        std::cerr << "[Main] window creation failed\n";
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    if (glewInit() != GLEW_OK) {
        std::cerr << "[Main] GLEW init failed\n";
        return -1;
    }

    glEnable(GL_DEPTH_TEST);
    glEnable(GL_MULTISAMPLE);
    glDisable(GL_CULL_FACE);
    glEnable(GL_CLIP_DISTANCE0);

    if (!Shader::init())      { std::cerr << "[Main] shader init failed\n";   return -1; }
    if (!Textures::initAll()) { std::cerr << "[Main] texture init failed\n";  return -1; }

    Input::install(window);
    Particles::init();

    Camera::clearInteriorColliders();
    Interior::build();

    Game::resetAll();

    float lastFrame = (float)glfwGetTime();
    glm::vec3 subPrevPos = Sub::pos;

    // ---- Cave camera state ----
    bool wasInCave   = false;
    int  savedCamMode = 0;
    int  prevCamMode  = Input::camMode();

    while (!glfwWindowShouldClose(window)) {
        if (Input::quit()) break;

        float now = (float)glfwGetTime();
        float dt  = now - lastFrame;
        lastFrame = now;
        if (dt > 0.1f) dt = 0.1f;

        Input::update();

        // ============================================================
        // CAVE CAMERA AUTO-SWITCH
        //   - On entering the cave: save current mode, force nose cam
        //   - While inside: if user toggles to chase (0), push back to 1
        //   - On exit: restore the saved mode
        // ============================================================
        bool inCave = Cave::isInsideCave(Sub::pos);
        if (inCave && !wasInCave) {
            savedCamMode = Input::camMode();
            Input::camMode() = 1;
        }
        if (inCave && Input::camMode() == 0) {
            Input::camMode() = 1;
        }
        if (!inCave && wasInCave) {
            Input::camMode() = savedCamMode;
        }
        wasInCave = inCave;

        // Interior camera reset on transition to interior mode.
        int curCamMode = Input::camMode();
        if (curCamMode != prevCamMode) {
            if (curCamMode == 2) Camera::resetInterior();
            prevCamMode = curCamMode;
        }

        Sub::update(dt);
        Camera::update(dt);
        Fish::update(dt);
        Mine::update(dt);
        Particles::update(dt);
        Torpedo::update(dt);
        Checkpoint::update(dt, Sub::pos, subPrevPos);
        Game::update(dt, Sub::pos, Sub::boundingRadius, subPrevPos);

        if (Input::fireTorpedo() && Game::isPlaying()) {
            Torpedo::fire(Sub::torpedoMuzzle(), Sub::forward());
        }

        glm::vec3 eye;
        glm::mat4 view = Camera::buildView(Sub::pos, Sub::forward(),
                                           Sub::up(), eye);

        char title[256];
        const char* shadingLabel = (Input::shadingMode() == 0) ? "Flat"
                                  : (Input::shadingMode() == 1) ? "Gouraud"
                                  : "Phong";
        const char* camLabel = (Input::camMode() == 0) ? "Chase"
                      : (Input::camMode() == 1) ? "Bow"
                      : (Input::camMode() == 2) ? "Interior"
                      : "Side";
        const char* depthLabel = (Sub::pos.y > Constants::WATER_LEVEL) ? "ABOVE" : "UNDER";
        std::snprintf(title, sizeof(title),
                      "Nautilus | %s | Shading: %s | Headlight: %s | Cam: %s | CP %d/%d | Hull %d/%d | Tor %d | Pos(%.0f,%.1f,%.0f)%s",
                      depthLabel, shadingLabel,
                      Input::headlight() ? "ON" : "OFF",
                      camLabel,
                      Checkpoint::visitedCount(), (int)Checkpoint::cps.size(),
                      Game::hullStrikesRemaining(), Game::hullMax(),
                      Torpedo::ammoRemaining(),
                      Sub::pos.x, Sub::pos.y, Sub::pos.z,
                      Input::paused() ? " | PAUSED" : "");
        glfwSetWindowTitle(window, title);

        renderFrame(dt, now, Sub::pos, Sub::forward(), Sub::up(), view, eye);

        subPrevPos = Sub::pos;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    Particles::shutdown();
    Textures::shutdownAll();
    Shader::shutdown();
    glfwTerminate();
    return 0;
}

static void updateLightingUniforms(const glm::vec3& subPos,
                                   const glm::vec3& subForward,
                                   const glm::vec3& eye) {
    Shader::setVec3("viewPos", eye);

    glm::vec3 baseAmb(0.18f, 0.22f, 0.28f);
    bool insideCave = Cave::isInsideCave(eye) && eye.y < Constants::WATER_LEVEL;
    if (insideCave) {
        baseAmb *= 0.35f;

        float dCenter = glm::length(eye - Cave::endpointCenter);
        if (dCenter < Cave::endpointRadius * 1.2f) {
            float t = 1.0f - dCenter / (Cave::endpointRadius * 1.2f);
            t = std::max(0.0f, std::min(1.0f, t));
            glm::vec3 warm(0.55f, 0.45f, 0.28f);
            baseAmb += warm * (t * t) * 1.4f;
        }
    }
    Shader::setVec3("ambientColor", baseAmb);

    Shader::setVec3("dirLightDir",
                    glm::normalize(glm::vec3(0.3f, -1.0f, 0.2f)));
    Shader::setVec3("dirLightColor", glm::vec3(0.55f, 0.60f, 0.70f));
    Shader::setInt("shadingMode", Input::shadingMode());
    Shader::setBool("headlightOn", Input::headlight());

    glm::vec3 headPos = subPos + subForward * 3.0f;
    Shader::setVec3("spotLightPos", headPos);
    Shader::setVec3("spotLightDir", subForward);
    Shader::setVec3("spotLightColor", glm::vec3(1.4f, 1.30f, 1.05f));
}

static Mesh g_floorMesh;
static void drawFloor() {
    if (!g_floorMesh.vao) g_floorMesh = Meshes::plane(Constants::FLOOR_Y, 200.0f);
    GLuint sand = Textures::sandTex();
    Draw::modelMaterial(glm::mat4(1.0f),
                        glm::vec3(0.55f, 0.50f, 0.40f), 4.0f, 0.0f,
                        sand, true);
    Draw::mesh(g_floorMesh);
}

static Mesh g_waterMesh;
static void drawWater() {
    if (!g_waterMesh.vao) g_waterMesh = Meshes::plane(Constants::WATER_LEVEL, 200.0f);
    Draw::modelMaterial(glm::mat4(1.0f),
                        glm::vec3(0.35f, 0.65f, 0.85f), 96.0f, 0.05f);
    Draw::mesh(g_waterMesh);
}

static Mesh g_sonarFloor;
static void drawSonar(int fbW, int fbH, const glm::vec3& subPos, float yaw) {
    int sonarSize = std::min(fbW, fbH) / 4;
    int margin = 15;
    int sx = fbW - sonarSize - margin;
    int sy = margin;

    glEnable(GL_SCISSOR_TEST);
    glScissor(sx, sy, sonarSize, sonarSize);
    glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_SCISSOR_TEST);
    glViewport(sx, sy, sonarSize, sonarSize);

    float range = 50.0f;
    glm::mat4 ortho = glm::ortho(-range, range, -range, range, 0.1f, 200.0f);
    glm::mat4 sonarView = glm::lookAt(
        glm::vec3(subPos.x, 100.0f, subPos.z),
        glm::vec3(subPos.x, 0.0f, subPos.z),
        glm::vec3(0.0f, 0.0f, -1.0f));
    Shader::setMat4("view", sonarView);
    Shader::setMat4("projection", ortho);

    Shader::setVec3("fogColor", glm::vec3(1.0f));
    Shader::setFloat("fogDensity", 0.0f);
    Shader::setVec3("ambientColor", glm::vec3(1.0f));
    Shader::setVec3("dirLightColor", glm::vec3(0.0f));
    Shader::setBool("headlightOn", false);
    Shader::setInt("shadingMode", 2);

    if (!g_sonarFloor.vao) g_sonarFloor = Meshes::plane(Constants::FLOOR_Y, range);
    Draw::modelMaterial(glm::mat4(1.0f), glm::vec3(0.90f), 1.0f);
    Draw::mesh(g_sonarFloor);

    for (auto& r : Rock::rocks) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f),
                                     glm::vec3(r.pos.x, 0.0f, r.pos.z));
        m = glm::scale(m, glm::vec3(r.scale.x, 1.0f, r.scale.z));
        Draw::modelMaterial(m, glm::vec3(0.45f, 0.32f, 0.20f), 1.0f);
        Draw::mesh(Meshes::sphere(8, 12));
    }
    if (Input::showMines()) {
        for (auto& mi : Mine::mines) {
            if (!mi.alive) continue;
            glm::mat4 m = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(mi.pos.x, 0.0f, mi.pos.z));
            m = glm::scale(m, glm::vec3(mi.scale * 0.6f, 1.0f, mi.scale * 0.6f));
            Draw::modelMaterial(m, glm::vec3(0.90f, 0.15f, 0.15f), 1.0f);
            Draw::mesh(Meshes::sphere(8, 12));
        }
    }
    if (Input::showFish()) {
        for (auto& f : Fish::fishes) {
            float fx = f.center.x + f.radius * cosf(f.angle);
            float fz = f.center.z + f.radius * sinf(f.angle);
            glm::mat4 m = glm::translate(glm::mat4(1.0f),
                                         glm::vec3(fx, 0.0f, fz));
            m = glm::scale(m, glm::vec3(0.4f, 1.0f, 0.4f));
            Draw::modelMaterial(m, f.color, 1.0f);
            Draw::mesh(Meshes::sphere(8, 12));
        }
    }
    for (auto& cp : Checkpoint::cps) {
        glm::vec3 col = cp.visited
            ? glm::vec3(0.20f, 0.95f, 0.30f)
            : glm::vec3(1.00f, 0.85f, 0.10f);
        glm::mat4 m = glm::translate(glm::mat4(1.0f),
                                     glm::vec3(cp.pos.x, 0.0f, cp.pos.z));
        m = glm::scale(m, glm::vec3(0.6f, 1.0f, 0.6f));
        Draw::modelMaterial(m, col, 1.0f);
        Draw::mesh(Meshes::sphere(8, 12));
    }
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f),
                                     glm::vec3(subPos.x, 0.0f, subPos.z));
        m = glm::rotate(m, glm::radians(yaw), glm::vec3(0,1,0));
        m = glm::scale(m, glm::vec3(1.5f, 1.0f, 3.0f));
        Draw::modelMaterial(m, glm::vec3(0.10f, 0.85f, 0.20f), 1.0f);
        Draw::mesh(Meshes::cube());
    }
}

static void renderFrame(float dt, float now,
                        const glm::vec3& subPos,
                        const glm::vec3& subForward,
                        const glm::vec3& subUp,
                        const glm::mat4& view,
                        const glm::vec3& eye) {
    GLFWwindow* w = glfwGetCurrentContext();
    int fbW, fbH;
    glfwGetFramebufferSize(w, &fbW, &fbH);
    if (fbH == 0) {
        glfwSwapBuffers(w);
        glfwPollEvents();
        return;
    }
    float aspect = (float)fbW / (float)fbH;
    glm::mat4 projection = glm::perspective(glm::radians(60.0f),
                                            aspect, 0.1f, 500.0f);

    float blend = glm::clamp((eye.y - Constants::WATER_LEVEL + 0.5f) / 1.5f,
                             0.0f, 1.0f);
    glm::vec3 underwaterBg(0.05f, 0.20f, 0.38f);
    glm::vec3 skyBg      (0.65f, 0.85f, 1.00f);
    glm::vec3 bgColor = glm::mix(underwaterBg, skyBg, blend);
    glClearColor(bgColor.r, bgColor.g, bgColor.b, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    Shader::use();
    Shader::setMat4("view", view);
    Shader::setMat4("projection", projection);
    updateLightingUniforms(subPos, subForward, eye);

    float aboveFactor = glm::clamp(
        (eye.y - (Constants::WATER_LEVEL - 6.0f)) / 6.0f, 0.0f, 1.0f);
    glm::vec3 fogColor = glm::mix(glm::vec3(0.03f, 0.15f, 0.30f),
                                  bgColor, aboveFactor);
    float fogDensity   = glm::mix(0.045f, 0.004f, aboveFactor);
    Shader::setVec3("fogColor", fogColor);
    Shader::setFloat("fogDensity", fogDensity);

    bool inInterior = (Input::camMode() == 2);
    bool cameraAboveWater = (eye.y > Constants::WATER_LEVEL);
    bool insideCave = Cave::isInsideCave(eye);

    if (inInterior || insideCave) {
        Shader::setVec4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, 10000.0f));
    } else if (cameraAboveWater) {
        Shader::setVec4("clipPlane",
            glm::vec4(0.0f, 1.0f, 0.0f, -(Constants::WATER_LEVEL - 0.02f)));
    } else {
        Shader::setVec4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, 10000.0f));
    }

    glViewport(0, 0, fbW, fbH);

    if (!insideCave) {
        drawFloor();
    }

    Rock::draw();
    Island::draw();
    Cave::draw();
    Checkpoint::draw();
    if (Input::showFish()) Fish::draw(now);
    if (Input::showMines()) Mine::draw();

    if (!inInterior) {
        Sub::draw();
    }

    Torpedo::draw();
    if (!insideCave) drawWater();
    Particles::render();

    if (inInterior) {
        Interior::draw();
    }

    if (Input::showSonar()) {
        Shader::setVec4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, 10000.0f));
        drawSonar(fbW, fbH, subPos, Sub::yawDeg);
        if (inInterior || insideCave) {
            Shader::setVec4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, 10000.0f));
        } else if (cameraAboveWater) {
            Shader::setVec4("clipPlane",
                glm::vec4(0.0f, 1.0f, 0.0f,
                          -(Constants::WATER_LEVEL - 0.02f)));
        } else {
            Shader::setVec4("clipPlane", glm::vec4(0.0f, 1.0f, 0.0f, 10000.0f));
        }
    }

    HUD::setCheckpoints(Checkpoint::visitedCount(),
                        (int)Checkpoint::cps.size());
    float cdFrac = Torpedo::cooldownRemaining() / 1.0f;
    if (cdFrac < 0) cdFrac = 0;
    HUD::setTorpedoInfo(Torpedo::ammoRemaining(),
                        Constants::TORPEDO_MAX, cdFrac);
    HUD::setHullIntegrity(Game::hullStrikesRemaining(), Game::hullMax());
    HUD::setStatus(Game::isPlaying() ? "EXPLORING"
                  : (Game::state == Game::State::Won ? "COMPLETE" : "DOWN"));
    glViewport(0, 0, fbW, fbH);
    HUD::render(fbW, fbH);
}
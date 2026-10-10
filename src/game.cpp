// ============================================================================
// game.cpp
// ----------------------------------------------------------------------------
// Rules:
//   - The submarine can take 3 mine hits before game over.
//   - 1.5 s invulnerability window after each hit.
//   - Collect all 5 flags (any order) to win.
// ============================================================================
#include "game.h"
#include "mine.h"
#include "checkpoint.h"
#include "particles.h"
#include "camera.h"
#include "hud.h"
#include "input.h"
#include "torpedo.h"
#include "submarine.h"
#include "rock.h"
#include "fish.h"
#include "cave.h"
#include "island.h"
#include "constants.h"
#include "rng.h"
#include <glm/glm.hpp>
#include <cmath>
#include <vector>
#include <string>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Game {
    static constexpr int   HULL_MAX    = 3;
    static constexpr float INVULN_TIME = 1.5f;

    State state       = State::Playing;
    float loseTimer   = 0.0f;
    int   strikes     = 0;
    float invulnTimer = 0.0f;

    void resetAll() {
        Rng::seed(1337u);
        Sub::resetToSpawn();
        Mine::mines.clear();
        Mine::spawnField();
        Rock::spawnField();
        Fish::spawnSchool();
        Island::build();
        Cave::build();
        Checkpoint::build();
        Torpedo::reset();
        state       = State::Playing;
        loseTimer   = 0.0f;
        strikes     = 0;
        invulnTimer = 0.0f;
        HUD::clearOverlay();
    }

    bool isPlaying() { return state == State::Playing; }

    void lose() {
        state = State::Lost;
        loseTimer = 0.0f;
        Camera::addShake(2.0f);
        HUD::pushOverlay("lose", "HULL DESTROYED",
                         {"PLAY AGAIN", "QUIT"}, 0);
    }

    int hullStrikes()          { return strikes; }
    int hullStrikesRemaining() { return HULL_MAX - strikes; }
    int hullMax()              { return HULL_MAX; }

    void update(float dt,
                const glm::vec3& subPos,
                float subBoundingR,
                const glm::vec3&) {

        if (invulnTimer > 0.0f) {
            invulnTimer = std::max(0.0f, invulnTimer - dt);
        }

        if (state == State::Playing) {
            if (invulnTimer <= 0.0f) {
                for (auto& m : Mine::mines) {
                    if (!m.alive) continue;
                    float hitDist = m.scale * Constants::MINE_HIT_R
                                    + subBoundingR;
                    if (glm::length(m.pos - subPos) < hitDist) {
                        // Feedback grows with each strike.
                        float flashAmt = 0.5f;
                        float shakeAmt = 0.7f;
                        int   debrisN  = 14;
                        if (strikes == 1) { flashAmt = 0.7f;  shakeAmt = 1.0f; debrisN = 20; }
                        if (strikes >= 2) { flashAmt = 0.95f; shakeAmt = 1.5f; debrisN = 26; }

                        Particles::triggerFlash(flashAmt);
                        Particles::spawnDebris(m.pos, glm::vec3(0),
                                              glm::vec3(1.0f, 0.7f, 0.2f),
                                              debrisN);
                        Particles::spawnSpark(m.pos,
                                              glm::vec3(1.0f, 0.9f, 0.5f));
                        Camera::addShake(shakeAmt);

                        m.alive = false;
                        strikes++;
                        invulnTimer = INVULN_TIME;

                        if (strikes >= HULL_MAX) {
                            lose();
                        }
                        break;
                    }
                }
            }

            if (state == State::Playing && Checkpoint::allVisited()) {
                state = State::Won;
                HUD::pushOverlay("win", "MISSION COMPLETE",
                                 {"PLAY AGAIN", "QUIT"}, 0);
            }
        }
        else if (state == State::Lost) {
            loseTimer += dt;
            if (loseTimer > 0.7f) {
                int d = Input::menuUpDown();
                static int selected = 0;
                if (d != 0) {
                    selected = (selected + d + 2) % 2;
                    HUD::pushOverlay("lose", "HULL DESTROYED",
                                     {"PLAY AGAIN", "QUIT"}, selected);
                }
                if (Input::menuEnter()) {
                    if (selected == 0) {
                        resetAll();
                    } else {
                        glfwSetWindowShouldClose(glfwGetCurrentContext(),
                                                 GLFW_TRUE);
                    }
                }
            }
        }
        else if (state == State::Won) {
            static int selected = 0;
            int d = Input::menuUpDown();
            if (d != 0) {
                selected = (selected + d + 2) % 2;
                HUD::pushOverlay("win", "MISSION COMPLETE",
                                 {"PLAY AGAIN", "QUIT"}, selected);
            }
            if (Input::menuEnter()) {
                if (selected == 0) {
                    resetAll();
                } else {
                    glfwSetWindowShouldClose(glfwGetCurrentContext(),
                                             GLFW_TRUE);
                }
            }
        }
    }
}
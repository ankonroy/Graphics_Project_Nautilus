// ============================================================================
// input.h
// ----------------------------------------------------------------------------
// Owns the key callback, the per-frame polled-key state and the toggle-set
// state. Designed to be the only place that calls glfwGetKey or sets the
// key callback.
// ============================================================================
#pragma once

#include <GLFW/glfw3.h>

namespace Input {
    // Installs the GLFW key callback.
    void install(GLFWwindow* w);

    // Called once per frame BEFORE the game logic to refresh per-frame state.
    void update();

    // Held keys (current frame).
    bool down(int key);
    bool pressed(int key);   // rising edge

    // ---- Toggle flags driven by key presses ----
    int&  shadingMode();     // 0=flat 1=gouraud 2=phong
    bool& headlight();
    bool& showSonar();
    bool& showFish();
    bool& showMines();
    bool& paused();
    int&  camMode();         // 0=chase 1=bow 2=interior
    bool  fireTorpedo();     // latched - cleared after read
    bool  resetSub();        // latched
    int   menuUpDown();      // -1/0/+1 for menu navigation
    bool  menuEnter();       // latched
    bool& quit();
}

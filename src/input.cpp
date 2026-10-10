// ============================================================================
// input.cpp
// ============================================================================
#include "input.h"
#include <cstring>

namespace {
    bool held[512] = {false};
    bool prev[512]  = {false};

    // 0 = chase, 1 = bow (nose), 2 = interior, 3 = side
    int  g_camMode   = 0;
    int  g_shading   = 2;
    bool g_light     = true;
    bool g_sonar     = true;
    bool g_fish      = true;
    bool g_mines     = true;
    bool g_paused    = false;
    bool g_fireTor   = false;
    bool g_resetSub  = false;
    int  g_menuUpDown = 0;
    bool g_menuEnter = false;
    bool g_quit      = false;

    void setKey(int key, bool v) {
        if (key < 0 || key >= 512) return;
        held[key] = v;
    }
}

namespace Input {
    void install(GLFWwindow* w) {
        glfwSetKeyCallback(w, [](GLFWwindow*, int key, int, int action, int) {
            if (action == GLFW_REPEAT) return;
            if (action == GLFW_PRESS) {
                switch (key) {
                    case GLFW_KEY_ESCAPE: g_quit = true; break;
                    case GLFW_KEY_C:      g_camMode = (g_camMode + 1) % 4; break;
                    case GLFW_KEY_1:      g_shading = 0; break;
                    case GLFW_KEY_2:      g_shading = 1; break;
                    case GLFW_KEY_3:      g_shading = 2; break;
                    case GLFW_KEY_4:      g_light = !g_light; break;
                    case GLFW_KEY_5:      g_sonar = !g_sonar; break;
                    case GLFW_KEY_F:      g_fish  = !g_fish; break;
                    case GLFW_KEY_M:      g_mines = !g_mines; break;
                    case GLFW_KEY_P:      g_paused = !g_paused; break;
                    case GLFW_KEY_R:      g_resetSub = true; break;
                    case GLFW_KEY_E:      g_fireTor  = true; break;
                    case GLFW_KEY_UP:     g_menuUpDown = +1; break;
                    case GLFW_KEY_DOWN:   g_menuUpDown = -1; break;
                    case GLFW_KEY_ENTER:  g_menuEnter  = true; break;
                    default: break;
                }
            }
        });
    }

    void update() {
        for (int k = 0; k < 512; k++) {
            prev[k] = held[k];
            held[k] = glfwGetKey(glfwGetCurrentContext(), k) == GLFW_PRESS;
        }
    }

    bool down(int key) {
        if (key < 0 || key >= 512) return false;
        return held[key];
    }
    bool pressed(int key) {
        if (key < 0 || key >= 512) return false;
        return held[key] && !prev[key];
    }

    int&  camMode()   { return g_camMode; }
    int&  shadingMode(){ return g_shading; }
    bool& headlight() { return g_light; }
    bool& showSonar() { return g_sonar; }
    bool& showFish()  { return g_fish; }
    bool& showMines() { return g_mines; }
    bool& paused()    { return g_paused; }
    bool fireTorpedo() {
        bool v = g_fireTor;
        g_fireTor = false;
        return v;
    }
    bool resetSub() {
        bool v = g_resetSub;
        g_resetSub = false;
        return v;
    }
    int  menuUpDown() {
        int v = g_menuUpDown;
        g_menuUpDown = 0;
        return v;
    }
    bool menuEnter() {
        bool v = g_menuEnter;
        g_menuEnter = false;
        return v;
    }
    bool& quit() { return g_quit; }
}
// ============================================================================
// hud.h
// ============================================================================
#pragma once

#include <string>
#include <vector>

namespace HUD {
    void setStatus(const std::string& s);
    void setCheckpoints(int visited, int total);
    void setTorpedoInfo(int remaining, int max, float cooldownFrac);
    void setHullIntegrity(int remaining, int max);

    void pushOverlay(const std::string& id, const std::string& title,
                     const std::vector<std::string>& buttons,
                     int selected);
    void clearOverlay();
    bool hasOverlay();

    void render(int fbW, int fbH);
    void drawFlash(float intensity, int fbW, int fbH);
}
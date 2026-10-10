// ============================================================================
// constants.h
// ============================================================================
#pragma once

namespace Constants {
    constexpr int   WINDOW_W = 1280;
    constexpr int   WINDOW_H = 720;

    // ---- World ----------------------------------------------------------
    constexpr float WATER_LEVEL      = 3.0f;
    constexpr float WORLD_RADIUS     = 200.0f;
    constexpr float FLOOR_Y          = -55.0f;
    constexpr float ISLAND_FOOTPRINT = 95.0f;

    // ---- Island / Volcano ------------------------------------------------
    constexpr float ISLAND_BEACH_RADIUS = 32.0f;
    constexpr float ISLAND_BEACH_TOP    = 6.0f;
    constexpr float VOLCANO_BASE_RADIUS = 24.0f;
    constexpr float VOLCANO_TOP_RADIUS  = 4.0f;
    constexpr float VOLCANO_HEIGHT      = 11.0f;
    constexpr float VOLCANO_BASE_Y      = 10.0f;
    constexpr float CRATER_RADIUS       = 3.5f;

    // ---- Cave ------------------------------------------------------------
    constexpr float CAVE_TUNNEL_RADIUS  = 6.0f;
    constexpr float CAVE_ENDPOINT_R     = 20.0f;
    constexpr float CAVE_ENDPOINT_Y     = -30.0f;
    constexpr float CAVE_ENTRANCE_Y     = -22.0f;
    constexpr float CAVE_SUNBEAM_RADIUS = 5.0f;

    // ---- Submarine -------------------------------------------------------
    // Realistic ratio: length / diameter ≈ 10. Length 14, radius 1.4.
    constexpr float SUB_LENGTH         = 14.0f;
    constexpr float SUB_RADIUS         = 1.4f;
    constexpr float SUB_BOUNDING_R     = 2.0f;
    constexpr float SUB_TORPEDO_OFFSET = 8.5f;   // muzzle just ahead of nose

    // ---- Movement --------------------------------------------------------
    constexpr float SUB_MOVE_SPEED    = 6.0f;
    constexpr float SUB_TURN_SPEED    = 60.0f;
    constexpr float SUB_VERT_SPEED    = 5.0f;
    constexpr float PITCH_TARGET_UP   = 20.0f;
    constexpr float PITCH_TARGET_DOWN = -20.0f;

    // ---- Torpedo ---------------------------------------------------------
    constexpr int   TORPEDO_MAX      = 6;
    constexpr float TORPEDO_SPEED    = 35.0f;
    constexpr float TORPEDO_DRAG     = 0.7f;
    constexpr float TORPEDO_RANGE    = 60.0f;
    constexpr float TORPEDO_RADIUS   = 0.45f;
    constexpr float TORPEDO_BLAST    = 4.0f;
    constexpr float TORPEDO_COOLDOWN = 1.0f;

    // ---- Mine ------------------------------------------------------------
    constexpr float MINE_RADIUS = 0.9f;
    constexpr float MINE_HIT_R  = 0.9f;

    // ---- Checkpoints (flags) ---------------------------------------------
    constexpr int   CHECKPOINT_COUNT         = 5;
    constexpr float CHECKPOINT_POLE_HEIGHT   = 6.0f;
    constexpr float CHECKPOINT_FLAG_WIDTH    = 1.7f;
    constexpr float CHECKPOINT_FLAG_HEIGHT   = 1.2f;
    constexpr float CHECKPOINT_TOUCH_RADIUS  = 4.5f;

    // ---- Particles -------------------------------------------------------
    constexpr int   MAX_PARTICLES = 512;
    constexpr int   MAX_BUBBLES   = 256;
}
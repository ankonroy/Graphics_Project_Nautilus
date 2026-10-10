# Nautilus: Interactive 3D Submarine Explorer — Game Edition

A fully procedural, real-time 3D submarine exploration **game** built with
C++17 and Modern OpenGL 3.3 Core Profile. Pilot a nuclear-style submarine
around a volcanic island, clear five glowing checkpoints, fight mines with
torpedoes, and find the secret chamber under the volcano.

**Status:** Playable MVP game. Procedural geometry, procedural textures, a
procedurally generated island, an underwater cave, an explorable interior,
a full game loop, and HUD text rendering. All without any external assets.

---

## Table of Contents

1. Quick Overview
2. New in the Game Edition
3. Build and Run
4. Controls
5. Game Flow
6. File Structure
7. Architecture
8. Known Limitations and Assumptions

---

## 1. Quick Overview

| Property          | Value                                                   |
|-------------------|---------------------------------------------------------|
| Language          | C++17                                                   |
| Graphics API      | OpenGL 3.3 Core Profile                                 |
| Window/Input      | GLFW 3                                                  |
| GL Loader         | GLEW                                                    |
| Math Library      | GLM                                                     |
| Build System      | CMake 3.10 or higher                                    |
| Target Platform   | Linux (developed on Ubuntu), portable to Windows/macOS  |
| External Assets   | **None.** Geometry, textures, fonts, world — all code   |

---

## 2. New in the Game Edition

This iteration turns the previous sandbox into a playable game. See
`CHANGELOG.md` for the full list. Highlights:

- **Refactored into ~20 files** for clarity.
- **5 checkpoints** as glowing torus rings. Random order; the 5th is
  always inside the underwater cave.
- **Torpedoes**: 6 per round, drag, range limit, ray-vs-sphere collision,
  blast radius, bubble trail, screen flash.
- **Mine explosions** with particles, camera shake, and a game-over
  menu.
- **Win / Lose / Play Again** menu with keyboard navigation.
- **Interior control room** camera (replaces the nose camera) with
  free-fly movement and AABB collision.
- **Island** with a volcano, sandy beach, palm trees, grass.
- **Underwater cave** with a winding tunnel, jagged rocks, an endpoint
  chamber, and a sunbeam shining down from the crater.
- **Modern submarine** with hull, sail, planes, rudder, propeller,
  hatch, periscope, sonar dome, and a red waterline stripe.
- **Realistic rocks** (deformed low-poly) and **realistic fish** (with
  wagging forked tails).
- **Procedural textures** (sand, rock, lava, metal, wood, stripes).
- **HUD** with a 5x7 bitmap font rendered as textured quads.

---

## 3. Build and Run

### Prerequisites (Ubuntu / Debian)

```bash
sudo apt update
sudo apt install build-essential cmake libglfw3-dev libglew-dev libglm-dev
```

### Build

```bash
mkdir -p build
cd build
cmake ..
cmake --build . -j
```

### Run

```bash
./MyOpenGLProject
```

### VS Code

Install the C/C++ and CMake Tools extensions. The bottom status bar has
a Build and Run button. If a kit prompt appears, choose your system
GCC.

---

## 4. Controls

### Piloting the submarine (always active)

| Key         | Action                                          |
|-------------|-------------------------------------------------|
| W / S       | Move forward / backward                         |
| A / D       | Turn left / right                               |
| Space       | Rise (pitches nose up)                          |
| Left Shift  | Dive (pitches nose down)                        |

### Interior control room (only when `C` is set to Interior)

| Key         | Action                                          |
|-------------|-------------------------------------------------|
| Arrow keys  | Move inside the room (IJKL are an alternative)  |
| I / K       | Look up / down                                  |
| J / L       | Look left / right                               |
| PageUp/Dn   | Move up / down (alternative)                    |

The submarine is **still being piloted** by WASD/Space/Shift while
you walk around inside. The interior camera is independent. This is
intentional — feel free to keep the sub moving while you pace the deck.

### Game actions

| Key         | Action                                          |
|-------------|-------------------------------------------------|
| C           | Cycle camera: Chase ↔ Interior                  |
| E           | Fire torpedo                                    |
| F           | Toggle fish                                     |
| M           | Toggle mine visibility (debug)                  |
| 1 / 2 / 3   | Shading mode: Flat / Gouraud / Phong            |
| 4           | Toggle headlight (submarine spot light)         |
| 5           | Toggle sonar minimap                            |
| P           | Pause / resume animation                        |
| R           | Reset submarine to a random spawn position      |
| Esc         | Quit (or dismiss game menu)                     |
| Up / Down   | Move menu selection (win/lose screens)          |
| Enter       | Activate selected menu button                   |

---

## 5. Game Flow

1. **Start**: the submarine spawns at a random position outside the
   island, just below the surface. The first checkpoint is highlighted
   in warm yellow/orange.
2. **Fly through the glowing ring** of the next checkpoint. When you
   pass through, the ring turns green.
3. **Mines** are scattered throughout the world and around each
   checkpoint. Touching one ends the game. Use `E` to fire a
   torpedo — torpedoes destroy mines within a small blast radius.
4. **Find the cave entrance** on the side of the island (the
   `5`th checkpoint lives there, so the 4th is somewhere outside
   leading you toward the island). The cave is dimmer than open
   water — turn on the headlight (`4`) to see clearly.
5. **Pass through the 5th ring**, which sits under a beam of sunlight
   in the chamber below the volcano. The game displays
   `MISSION COMPLETE` and offers Play Again or Quit.
6. **Play Again** resets everything: sub spawn, mine field, fish
   school, checkpoint order, and torpedoes. The 5th checkpoint
   remains inside the cave but the entrance angle is re-randomised
   so the cave is in a new location each run.

---

## 6. File Structure

```
Graphics_Project_Nautilus/
|-- CMakeLists.txt
|-- README.md
|-- CHANGELOG.md
|-- build/                       <- generated by CMake
|   `-- MyOpenGLProject          <- executable
|-- include/                     <- public headers
|   |-- constants.h
|   |-- rng.h
|   |-- shader.h
|   |-- mesh.h
|   |-- texture.h
|   |-- draw.h
|   |-- input.h
|   |-- camera.h
|   |-- submarine.h
|   |-- torpedo.h
|   |-- mine.h
|   |-- fish.h
|   |-- rock.h
|   |-- checkpoint.h
|   |-- island.h
|   |-- cave.h
|   |-- interior.h
|   |-- particles.h
|   |-- hud.h
|   `-- game.h
`-- src/                         <- implementations + main
    |-- main.cpp                 <- window, loop, orchestration
    |-- rng.cpp
    |-- shader.cpp
    |-- mesh.cpp
    |-- texture.cpp
    |-- draw.cpp
    |-- input.cpp
    |-- camera.cpp
    |-- submarine.cpp
    |-- torpedo.cpp
    |-- mine.cpp
    |-- fish.cpp
    |-- rock.cpp
    |-- checkpoint.cpp
    |-- island.cpp
    |-- cave.cpp
    |-- interior.cpp
    |-- particles.cpp
    |-- hud.cpp
    `-- game.cpp
```

CMake globs `src/*.cpp`, so adding a new translation unit does not
require editing `CMakeLists.txt`.

---

## 7. Architecture

### Module responsibilities

| Module        | Responsibility                                                 |
|---------------|----------------------------------------------------------------|
| `constants`   | All magic numbers (window size, world radius, tunables).       |
| `rng`         | Deterministic xorshift32 — same world on every run.            |
| `shader`      | GLSL program + uniform setters.                                |
| `mesh`        | Procedural primitives: cube, sphere, cylinder, cone, plane, triangle, torus, lathe bodies, deformed rocks, hemisphere, open tube. |
| `texture`     | Procedural textures: checker, value noise, sand, rock, metal, wood, stripes, lava. |
| `draw`        | `Draw::mesh`, `Draw::modelMaterial`, `Draw::alignLocalY`.      |
| `input`       | GLFW key callback + per-frame key state + toggle flags.        |
| `camera`      | Chase + Interior cameras, AABB collision, shake.               |
| `submarine`   | Player sub: hull, sail, planes, rudder, prop, etc.             |
| `torpedo`     | Projectiles: drag, range, ray-vs-sphere collision, blast.      |
| `mine`        | Spiked moored mines.                                           |
| `fish`        | Procedural fish with wagging tail.                             |
| `rock`        | Deformed low-poly rocks with 4 cached variants.                |
| `checkpoint`  | Torus rings + pass-through detection.                          |
| `island`      | Volcano, beach, palm trees, grass.                             |
| `cave`        | Tunnel segments, jagged rocks, dome chamber, sunbeam.          |
| `interior`    | Control room geometry + AABB colliders.                        |
| `particles`   | Bubbles, debris, screen flash.                                 |
| `hud`         | Bitmap font + HUD text + game-over overlay.                    |
| `game`        | State machine: Playing / Won / Lost + reset.                   |
| `main`        | Wires everything together.                                     |

### Per-frame loop

1. `glfwGetTime` → `dt`
2. `Input::update()` — refresh held-key state
3. `Sub::update(dt)` — read input, move, clamp
4. `Camera::update(dt)` — interior fly + shake decay
5. `Fish::update`, `Mine::update`, `Particles::update`, `Torpedo::update`
6. `Checkpoint::update` — plane-cross + radial test
7. `Game::update` — sub vs mine (lose), all visited (win), menu nav
8. If `E` pressed and `Playing`, `Torpedo::fire`
9. Build view matrix
10. `renderFrame` — draw the world, interior (if applicable), sonar, HUD

### Coordinate system

Right-handed, Y up, Z toward viewer. Submarine local forward is `-Z`.

---

## 8. Known Limitations and Assumptions

- **Window size**: 1280×720, not user-resizable. The sonar minimap is
  sized as a fraction of the framebuffer.
- **No procedural-texture reload** — textures are baked at startup.
- **Cave entrance angle** is randomised once per game (at
  `Game::resetAll`). It does not move mid-run.
- **Sub-vs-mine collision** is a single sphere check. There is no
  collision against rocks (the sub sails through them).
- **Torpedo aiming** is straight along the sub's forward vector. No
  mouse aim.
- **Interior mode** keeps the sub moving; this is intentional but may
  feel precarious.
- **No LOD** — at the current 140 rocks, 26 fish, and ~14 mines the
  frame is fine, but a larger world would need instancing.
- **No shadows / no reflections / no waves**. The water plane is a
  single static quad.
- **Cave ambient dimming** is binary (full or 45%) based on a simple
  "is the camera inside the cave region?" test. There is no
  distance-based gradient.

### Assumptions made

- `Rng::seed(1337)` is used by default; `Game::resetAll` re-seeds to
  1337 every Play Again. Removing this call gives a different world
  per run.
- The interior camera initial position is `(0, 1.4, 2.6)`, facing
  toward `-Z` (the front of the room with the window).
- The 5th checkpoint's `axis` is hard-coded to `+Y` for simplicity.
- The "depth" label in the title flips to `ABOVE` once the camera is
  above `WATER_LEVEL`.

---

## Project owner

Ankon Roy (Roll 2107113), CSE, KUET.

Originally a procedural submarine sandbox, now a small game.

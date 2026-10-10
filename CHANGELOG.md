# Changelog

## [2.0.0] - Game Edition

### Added
- **Multi-file refactor** (Part 1). The codebase went from one ~800-line
  `src/main.cpp` to ~20 modular files under `src/` + `include/`. The
  single-file build still produces a `MyOpenGLProject` executable.
- **Game loop with 5 checkpoints, torpedoes, win/lose, and a Play Again
  flow** (Parts 2.1–2.5).
- **Interior submarine control room** with a free-fly camera, AABB
  collision against walls and furniture, and a front viewport that
  shows the exterior (Part 3). The chase camera is replaced.
- **Island, volcano, beach, palm trees, and grass** at the world origin
  (Part 4.2).
- **Underwater cave** with a winding tunnel leading from a random
  entrance on the side of the island to a domed chamber under the
  volcano, with a sunbeam shining down from the crater. The cave
  ambient light is dimmer when the camera is inside (Part 4.3).
- **Modern submarine exterior** with a teardrop lathe hull, conning
  tower, sonar dome, planes, rudder, propeller, hatch, periscope, and
  a red waterline stripe (Part 5.1).
- **Realistic rocks** built from deformed subdivided cubes (4 variants)
  with rock-noise texture (Part 5.2).
- **Realistic fish** with an ellipsoid body, dorsal fin, two pectoral
  fins, and a forked tail that wags as the fish swims its circular
  orbit (Part 5.3).
- **Procedural textures** for sand, rock, lava, metal panels, wood, and
  stripes — all generated at startup from value noise + checker
  patterns (Part 5.4).
- **Bitmap-font HUD** with a 5x7 hand-encoded subset (A-Z, 0-9, common
  punctuation) drawn as textured quads. No external font library.
- **Particle system** for bubbles, debris, and a screen flash.
- **Camera shake** (decaying) for the player death and big explosion
  moments.
- **CMake now globs `src/*.cpp`**, so adding new source files needs no
  CMake change.

### Changed
- **World scale** `WORLD_RADIUS` went from 85 to 150 to fit the new
  island and longer play distances.
- The previous "nose camera" has been replaced by the **Interior
  Control Room** camera; `C` now toggles Chase vs Interior.
- Shaders were extended (not replaced) with UV/UV texture sampling, an
  `emissiveBoost` for the checkpoint rings and lava patches, and a
  texture flag.
- The title bar carries more state (camera, lighting, torpedo count,
  checkpoint count, depth label).

### Notes
- All geometry is still procedural — no models, no images, no fonts
  loaded from disk.
- The procedural textures are still produced at runtime; no asset
  files of any kind.
- Compile-tested on Linux with GCC 13 and CMake 3.28.

// ============================================================================
// texture.h - procedural texture handles.
// ============================================================================
#pragma once

#include <GL/glew.h>
#include <string>

namespace Textures {
    // Build a small set of procedural textures. Returns true on success.
    bool initAll();
    void shutdownAll();

    // Each handle is an OpenGL texture name. The functions below return the
    // texture name to bind to texture unit 0.
    GLuint checker();    // 256x256 black/white checker
    GLuint noiseTex();   // 256x256 grayscale value noise
    GLuint sandTex();    // tan sand-like noise
    GLuint rockTex();    // gray rock noise
    GLuint stripe();     // 256x64 diagonal stripes
    GLuint metalPanel(); // dark metal panels
    GLuint woodPanel();  // warm wood
    GLuint lava();       // bright orange/yellow

    // Bind `tex` to active texture unit 0.
    void bind(GLuint tex);
}

// ============================================================================
// rng.h
// ----------------------------------------------------------------------------
// Deterministic xorshift32 PRNG. Same seed -> same world on every run.
// Crucial for coursework demos: the rock field, mine positions, fish paths,
// and island decoration are reproducible.
// ============================================================================
#pragma once

namespace Rng {
    // Seed the generator. 0 is treated as 1 (xorshift32 stalls on 0).
    void seed(unsigned int s);

    // Next 32-bit value.
    unsigned int next();

    // Convenience helpers.
    float randFloat();                      // [0, 1)
    float randRange(float a, float b);      // [a, b)
    int   randInt(int a, int b);            // [a, b]
}

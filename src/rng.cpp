// ============================================================================
// rng.cpp - xorshift32 implementation. See header for rationale.
// ============================================================================
#include "rng.h"

namespace {
    unsigned int state = 123456789u;
}

namespace Rng {
    void seed(unsigned int s) { state = s ? s : 1u; }

    unsigned int next() {
        // Classic xorshift32 step. Three shifts, three xors, very fast.
        state ^= state << 13;
        state ^= state >> 17;
        state ^= state << 5;
        return state;
    }

    float randFloat() { return (next() & 0x00FFFFFF) / float(0x01000000); }
    float randRange(float a, float b) { return a + (b - a) * randFloat(); }

    int randInt(int a, int b) {
        // Inclusive on both ends. Good enough for small ranges.
        unsigned int r = next();
        return a + (int)(r % (unsigned)(b - a + 1));
    }
}

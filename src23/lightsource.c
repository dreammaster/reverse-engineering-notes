#include "lightsource.h"

static const uint16_t kLitFlagBits[LightSourceCount] = {0x2000, 0x0800, 0x0400};

bool lightSourceApply(LightSourceState *state, unsigned actionId) {
    LightSourceKind kind;
    switch (actionId) {
    case 8:
        kind = LightSourceCandle;
        break;
    case 0xE:
        kind = LightSourceTorch;
        break;
    case 0xB:
        kind = LightSourceGeneric;
        break;
    default:
        return false;
    }
    state->litFlags = (uint16_t)(state->litFlags | kLitFlagBits[kind]);
    state->duration[kind]++;
    return true;
}

bool lightSourceTick(LightSourceState *state, unsigned actionId) {
    LightSourceKind kind;
    switch (actionId) {
    case 9:
        kind = LightSourceCandle;
        break;
    case 0xF:
        kind = LightSourceTorch;
        break;
    case 0xC:
        kind = LightSourceGeneric;
        break;
    default:
        return false;
    }
    if (state->duration[kind] > 0) {
        state->duration[kind]--;
    }
    if (state->duration[kind] == 0) {
        state->litFlags = (uint16_t)(state->litFlags & ~kLitFlagBits[kind]);
    }
    return true;
}

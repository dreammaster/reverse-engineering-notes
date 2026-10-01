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

bool lightSourceTickItemSlot(LightSourceState *state, uint8_t *slot, uint16_t elapsedMinutes) {
    uint16_t id = itemSlotId(slot);
    LightSourceKind kind;
    switch (id) {
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

    uint16_t extra = itemSlotExtra(slot);
    if (elapsedMinutes < extra) {
        itemSlotSet(slot, id, (uint16_t)(extra - elapsedMinutes));
        return true;
    }

    itemSlotSet(slot, (uint16_t)(id + 1), 0);
    if (state->instanceCount[kind] > 0) {
        state->instanceCount[kind]--;
    }
    if (state->instanceCount[kind] == 0) {
        state->litFlags = (uint16_t)(state->litFlags & ~kLitFlagBits[kind]);
    }
    return true;
}

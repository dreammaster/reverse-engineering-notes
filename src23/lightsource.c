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
    state->litCount[kind]++;
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
    if (state->litCount[kind] > 0) {
        state->litCount[kind]--;
    }
    if (state->litCount[kind] == 0) {
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
    if (state->litCount[kind] > 0) {
        state->litCount[kind]--;
    }
    if (state->litCount[kind] == 0) {
        state->litFlags = (uint16_t)(state->litFlags & ~kLitFlagBits[kind]);
    }
    return true;
}

bool lightSourceArmSpellTimer(LightSourceState *state, unsigned slot, uint16_t duration) {
    if (slot < 1 || slot > LightSpellTimerCount) {
        return false;
    }
    state->timers[slot - 1] = duration;
    state->litFlags = (uint16_t)(state->litFlags | 0x8000u | (0x100u >> (slot - 1)));
    return true;
}

void lightSourceTickTimers(LightSourceState *state, uint16_t elapsedMinutes) {
    if (!(state->litFlags & 0x8000u)) {
        return;
    }
    state->litFlags = (uint16_t)(state->litFlags & 0x7FFFu);
    for (unsigned i = 0; i < LightSpellTimerCount; i++) {
        uint16_t bit = (uint16_t)(0x100u >> i);
        if ((int16_t)state->timers[i] > 0) {
            if ((int32_t)(int16_t)state->timers[i] - (int32_t)(int16_t)elapsedMinutes > 0) {
                state->timers[i] = (uint16_t)(state->timers[i] - elapsedMinutes);
                state->litFlags = (uint16_t)(state->litFlags | 0x8000u);
                continue;
            }
        }
        state->timers[i] = 0;
        state->litFlags = (uint16_t)(state->litFlags & ~bit);
    }
}

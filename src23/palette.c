#include "palette.h"

#include <string.h>

uint32_t paletteBlockOffset(GameKind game, unsigned block) {
    return (game == GameYendor3 ? 0x95BDAu : 0x8270Au) + PaletteBytes * block;
}

void dayNightFadeBegin(DayNightFade *fade, unsigned clockMinutes) {
    fade->stepsLeft = DayNightSteps;
    if (clockMinutes == 1080) {
        fade->offset = 0x14D;
        fade->delta = -3;
    } else {
        fade->offset = 0;
        fade->delta = 3;
    }
}

bool dayNightFadeStep(DayNightFade *fade, const uint8_t block2[PaletteBytes], uint8_t out[DayNightWindowBytes]) {
    for (int i = 0; i < DayNightWindowBytes; i++) {
        int at = fade->offset + i;
        out[i] = (at >= 0 && at < PaletteBytes) ? block2[at] : 0;
    }
    fade->offset += fade->delta;
    fade->stepsLeft--;
    return fade->stepsLeft > 0;
}

void paletteCycleFrame(const uint8_t block2[PaletteBytes], unsigned phase, uint8_t out[CycleBytes]) {
    const uint8_t *table = block2 + 3 * CycleFirstColour;
    unsigned rotate = phase & 3;
    for (unsigned group = 0; group < 4; group++) {
        for (unsigned colour = 0; colour < 4; colour++) {
            memcpy(out + 3 * (group * 4 + colour), table + 3 * (group * 4 + (colour + rotate) % 4), 3);
        }
    }
}

bool paletteFadeOutRound(uint8_t palette[PaletteBytes]) {
    bool changed = false;
    for (unsigned i = 0; i < PaletteBytes; i++) {
        if (palette[i] > 0) {
            palette[i]--;
            changed = true;
        }
    }
    return changed;
}

void paletteFadeInFrame(const uint8_t master[PaletteBytes], unsigned round, uint8_t out[PaletteBytes]) {
    for (unsigned i = 0; i < PaletteBytes; i++) {
        int value = (int)master[i] - FadeRounds + (int)round;
        out[i] = (uint8_t)(value < 0 ? 0 : value > master[i] ? master[i] : value);
    }
}

bool paletteFadeUpRound(uint8_t palette[PaletteBytes], const uint8_t master[PaletteBytes]) {
    bool changed = false;
    for (unsigned i = 0; i < PaletteBytes; i++) {
        if (palette[i] < master[i]) {
            palette[i]++;
            changed = true;
        }
    }
    return changed;
}

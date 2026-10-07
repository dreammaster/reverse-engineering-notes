#include "palettefade.h"

#include <string.h>

static void setRange(PaletteFader *fader, const uint8_t *source, unsigned first, unsigned count, PaletteFrameFn frame, void *ctx) {
    if (first + count > 256) {
        count = first < 256 ? 256 - first : 0;
    }
    memcpy(fader->dac + first * 3, source + first * 3, (size_t)count * 3);
    if (frame) {
        frame(ctx, fader, first, count);
    }
}

unsigned paletteFadeRange(PaletteFader *fader, unsigned mode, unsigned rounds, unsigned count, unsigned first, PaletteFrameFn frame, void *ctx) {
    if (first + count > 256) {
        count = first < 256 ? 256 - first : 0;
    }
    size_t lo = (size_t)first * 3, n = (size_t)count * 3;
    unsigned written = 0;
    switch (mode) {
    case 0:
    case 2:
        memcpy(fader->buffer, fader->dac, PaletteBytes);
        /* fall through */
    case 3:
    case 4:
        for (unsigned r = 0; r < rounds; r++) {
            bool down = mode == 0 || mode == 3, changed = false;
            for (size_t i = lo; i < lo + n; i++) {
                if (down) {
                    if (fader->buffer[i] > 0) {
                        fader->buffer[i]--;
                        changed = true;
                    }
                } else if (fader->buffer[i] < fader->target[i]) {
                    fader->buffer[i]++;
                    changed = true;
                }
            }
            if (!changed) {
                break;
            }
            setRange(fader, fader->buffer, first, count, frame, ctx);
            written++;
        }
        break;
    case 1:
        for (size_t i = lo; i < lo + n; i++) {
            fader->work[i] = (uint8_t)(fader->target[i] - 0x3F);
        }
        for (unsigned r = 0; r < rounds; r++) {
            for (size_t i = lo; i < lo + n; i++) {
                uint8_t v = fader->work[i];
                if ((v & 0x80) || v != fader->target[i]) {
                    v++;
                    fader->work[i] = v;
                    if (!(v & 0x80)) {
                        fader->out[i] = v;
                    }
                }
            }
            setRange(fader, fader->out, first, count, frame, ctx);
            written++;
        }
        memset(fader->out, 0, PaletteBytes);
        break;
    case 6: /* RunPaletteFadeSequence: mode 1's rounds continuing from the work area as it is (no initialisation, `out` is kept) */
        for (unsigned r = 0; r < rounds; r++) {
            for (size_t i = lo; i < lo + n; i++) {
                uint8_t v = fader->work[i];
                if ((v & 0x80) || v != fader->target[i]) {
                    v++;
                    fader->work[i] = v;
                    if (!(v & 0x80)) {
                        fader->out[i] = v;
                    }
                }
            }
            setRange(fader, fader->out, first, count, frame, ctx);
            written++;
        }
        break;
    case 5:
        for (unsigned r = 0; r < rounds; r++) {
            for (size_t i = lo; i < lo + n; i++) {
                if (fader->work[i] != fader->target[i]) {
                    fader->work[i]--;
                }
            }
            setRange(fader, fader->work, first, count, frame, ctx);
            written++;
        }
        break;
    default:
        break;
    }
    return written;
}

void paletteSetToWhite(PaletteFader *fader) {
    memset(fader->work, 0x3F, PaletteBytes);
    memcpy(fader->dac, fader->work, PaletteBytes);
}

void paletteFadeInPrepare(PaletteFader *fader) {
    for (size_t i = 0; i < PaletteBytes; i++) {
        fader->work[i] = (uint8_t)(fader->target[i] - 0x3F);
    }
    memset(fader->out, 0, PaletteBytes);
}

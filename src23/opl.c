#include "opl.h"

#include <math.h>
#include <string.h>

enum { StateOff, StateAttack, StateDecay, StateSustain, StateRelease };

static const uint8_t kModOffset[OplChannels] = {0, 1, 2, 8, 9, 10, 16, 17, 18};
static const uint8_t kMultiplier[16] = {1, 2, 4, 6, 8, 10, 12, 14, 16, 18, 20, 20, 24, 24, 30, 30}; /* in half units: 0.5 1 2 3 ... */

void oplReset(Opl *opl, unsigned sampleRate) {
    memset(opl, 0, sizeof(*opl));
    opl->sampleRate = (double)sampleRate;
    for (unsigned i = 0; i < 18; i++) {
        opl->att[i] = 96.0;
    }
}

/* The operator (0-17) behind a register offset 0x00-0x15 (the chip's gaps at 6, 7, 0xE, 0xF), or -1. */
static int operatorForOffset(unsigned offset) {
    if (offset > 0x15 || (offset & 7) > 5) {
        return -1;
    }
    return (int)((offset / 8) * 6 + (offset & 7));
}

static int opIndex(unsigned channel, unsigned carrier) {
    return operatorForOffset(kModOffset[channel] + (carrier ? 3u : 0u));
}

void oplWrite(Opl *opl, uint8_t reg, uint8_t value) {
    opl->reg[reg] = value;
    if (reg >= 0xB0 && reg <= 0xB8) {
        unsigned ch = reg - 0xB0u;
        bool on = (value & 0x20) != 0;
        if (on && !opl->keyOn[ch]) {
            for (unsigned c = 0; c < 2; c++) {
                int op = opIndex(ch, c);
                opl->state[op] = StateAttack;
                opl->phase[op] = 0.0;
            }
        } else if (!on && opl->keyOn[ch]) {
            for (unsigned c = 0; c < 2; c++) {
                int op = opIndex(ch, c);
                if (opl->state[op] != StateOff) {
                    opl->state[op] = StateRelease;
                }
            }
        }
        opl->keyOn[ch] = on;
    }
}

static double waveform(unsigned wave, double cycles) {
    double x = cycles - floor(cycles);
    double s = sin(x * 6.283185307179586);
    switch (wave & 3) {
    case 1:
        return s > 0 ? s : 0;
    case 2:
        return fabs(s);
    case 3:
        return fmod(x, 0.5) < 0.25 ? fabs(s) : 0;
    default:
        return s;
    }
}

static double rateTimeMs(double base, unsigned rate) {
    return rate == 0 ? 1e12 : base / pow(2.0, (double)rate - 1.0);
}

/* Advances the envelope of one operator by one sample and returns its linear amplitude (before tremolo). */
static double envelope(Opl *opl, int op, unsigned channel, unsigned carrier) {
    unsigned off = kModOffset[channel] + (carrier ? 3u : 0u);
    uint8_t ar = (uint8_t)(opl->reg[0x60 + off] >> 4), dr = opl->reg[0x60 + off] & 15;
    uint8_t sl = (uint8_t)(opl->reg[0x80 + off] >> 4), rr = opl->reg[0x80 + off] & 15;
    bool sustained = (opl->reg[0x20 + off] & 0x20) != 0;
    double dtMs = 1000.0 / opl->sampleRate;
    double *att = &opl->att[op];
    switch (opl->state[op]) {
    case StateAttack:
        if (ar >= 15) {
            *att = 0;
        } else if (ar == 0) {
            break;
        } else {
            *att *= 1.0 - (1.0 - exp(-dtMs / (rateTimeMs(2826.24, ar) / 5.0)));
        }
        if (*att < 0.05) {
            *att = 0;
            opl->state[op] = StateDecay;
        }
        break;
    case StateDecay:
        *att += 96.0 * dtMs / rateTimeMs(39280.0, dr);
        if (*att >= (sl == 15 ? 93.0 : sl * 3.0)) {
            opl->state[op] = StateSustain;
        }
        break;
    case StateSustain:
        if (!sustained) {
            *att += 96.0 * dtMs / rateTimeMs(39280.0, rr);
        }
        break;
    case StateRelease:
        *att += 96.0 * dtMs / rateTimeMs(39280.0, rr);
        break;
    default:
        return 0;
    }
    if (*att >= 96.0) {
        *att = 96.0;
        if (opl->state[op] == StateRelease) {
            opl->state[op] = StateOff;
        }
    }
    return pow(10.0, -*att / 20.0);
}

void oplGenerate(Opl *opl, int16_t *out, unsigned count) {
    double amDepth = (opl->reg[0xBD] & 0x80) ? 4.8 : 1.0, vibCents = (opl->reg[0xBD] & 0x40) ? 14.0 : 7.0;
    for (unsigned n = 0; n < count; n++) {
        opl->lfoAm += 3.7 / opl->sampleRate;
        opl->lfoVib += 6.1 / opl->sampleRate;
        opl->lfoAm -= floor(opl->lfoAm);
        opl->lfoVib -= floor(opl->lfoVib);
        double tremolo = 0.5 * (1.0 - cos(opl->lfoAm * 6.283185307179586)) * amDepth; /* dB */
        double vibrato = sin(opl->lfoVib * 6.283185307179586) * vibCents / 1200.0;
        double mix = 0;
        for (unsigned ch = 0; ch < OplChannels; ch++) {
            int modOp = opIndex(ch, 0), carOp = opIndex(ch, 1);
            if (opl->state[modOp] == StateOff && opl->state[carOp] == StateOff) {
                continue;
            }
            unsigned fnum = (unsigned)(opl->reg[0xA0 + ch] | ((opl->reg[0xB0 + ch] & 3) << 8));
            unsigned block = (opl->reg[0xB0 + ch] >> 2) & 7;
            double base = (double)fnum * (double)(1u << block) / 1048576.0 * OplChipRate / opl->sampleRate;
            unsigned conn = opl->reg[0xC0 + ch] & 1, feedback = (opl->reg[0xC0 + ch] >> 1) & 7;
            double outs[2] = {0, 0};
            for (unsigned c = 0; c < 2; c++) {
                int op = c ? carOp : modOp;
                unsigned off = kModOffset[ch] + (c ? 3u : 0u);
                uint8_t chr = opl->reg[0x20 + off];
                double step = base * kMultiplier[chr & 15] * 0.5;
                if (chr & 0x40) {
                    step *= pow(2.0, vibrato);
                }
                double amp = envelope(opl, op, ch, c);
                amp *= pow(10.0, -((opl->reg[0x40 + off] & 63) * 0.75 + ((chr & 0x80) ? tremolo : 0.0)) / 20.0);
                double modulation = 0;
                if (c == 0 && feedback) {
                    modulation = (opl->fbPrev[ch][0] + opl->fbPrev[ch][1]) * 0.5 * 2.0 * pow(2.0, (double)feedback - 7.0);
                } else if (c == 1 && !conn) {
                    modulation = outs[0] * 2.0;
                }
                outs[c] = waveform(opl->reg[0xE0 + off], opl->phase[op] + modulation) * amp;
                opl->phase[op] += step;
                opl->phase[op] -= floor(opl->phase[op]);
            }
            opl->fbPrev[ch][1] = opl->fbPrev[ch][0];
            opl->fbPrev[ch][0] = outs[0];
            mix += conn ? outs[0] + outs[1] : outs[1];
        }
        double v = (double)out[n] + tanh(mix * 1.2) * 16000.0; /* a soft limiter keeps chords from clipping */
        out[n] = (int16_t)(v > 32767 ? 32767 : v < -32768 ? -32768 : v);
    }
}

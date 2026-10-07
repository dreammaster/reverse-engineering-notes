#ifndef YENDOR23_OPL_H
#define YENDOR23_OPL_H

#include <stdbool.h>
#include <stdint.h>

/*
 * A small floating-point OPL2 (YM3812 / AdLib) FM synthesizer, enough to hear what the CMF music of both games sounds like and to check the
 * parsed tracks (cmf.h, cmfplayer.h); a ScummVM engine would use ScummVM's own OPL emulator instead. Nine two-operator channels in melodic mode;
 * the rhythm (percussion) section is not emulated (its register bits are accepted and ignored).
 *
 * Model: per operator a phase accumulator (cycles per sample = fnum * 2^block * multiplier / 2^20 at the chip rate 49716 Hz), one of the four OPL2
 * waveforms, an envelope in dB (exponential attack, then linear-in-dB decay / sustain / release with the chip's published times: 2826 ms and
 * 39280 ms halving per rate step), total level 0.75 dB per step, tremolo (3.7 Hz, 1 / 4.8 dB) and vibrato (6.1 Hz, 7 / 14 cents), phase modulation of the
 * carrier by the modulator (full scale = 2 cycles), the modulator's feedback (level 7 = 2 cycles) and the two connection modes. Key scaling (KSL / KSR) is
 * ignored. It is not sample-exact against the real chip.
 */
enum { OplChannels = 9, OplChipRate = 49716 };

typedef struct {
    uint8_t reg[256];
    double phase[18];
    double att[18];    /* current attenuation in dB (0 = full, 96 = silent) */
    uint8_t state[18]; /* 0 off, 1 attack, 2 decay, 3 sustain, 4 release */
    double fbPrev[9][2];
    bool keyOn[9];
    double lfoAm, lfoVib;
    double sampleRate;
} Opl;

void oplReset(Opl *opl, unsigned sampleRate);
void oplWrite(Opl *opl, uint8_t reg, uint8_t value);

/* Adds `count` mono samples (16-bit, silence is 0) to `out` -- the caller clears the buffer first. */
void oplGenerate(Opl *opl, int16_t *out, unsigned count);

#endif

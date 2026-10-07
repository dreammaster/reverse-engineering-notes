#ifndef YENDOR23_CMFPLAYER_H
#define YENDOR23_CMFPLAYER_H

#include <stdbool.h>
#include <stdint.h>

#include "cmf.h"
#include "opl.h"

/*
 * Plays a parsed CMF track (cmf.h) through the OPL2 synthesizer (opl.h): the MIDI channels share the nine FM voices. A program change selects one of the track's
 * AdLib instruments (16 bytes, loaded into a voice's registers at note on: operator characteristics, key scale / total level, attack / decay, sustain / release,
 * waveforms, feedback / connection); note on takes a free voice (or steals the longest-sounding one), note off releases it; the carrier's total level is the
 * instrument's scaled by velocity and by the channel volume (controller 7), and pitch bend (and the CMF controllers 0x68 / 0x69, the fine pitch up / down in
 * 1/128 of a semitone) shifts the frequency. The percussive mode controller (0x67) is accepted but the rhythm channels are not synthesized. Time runs at the
 * track's tempo (quarter notes per minute) and ticks per quarter note; a set-tempo meta event (0x51) overrides it.
 */
enum { CmfVoiceCount = OplChannels };

typedef struct {
    CmfTrack track;
    CmfReader reader;
    Opl opl;
    unsigned sampleRate;
    double samplesPerTick;
    double samplesUntilEvent;
    bool finished, haveEvent;
    CmfEvent pending;
    uint8_t program[16], volume[16];
    int16_t bend[16];
    int8_t fine[16];
    struct {
        bool active;
        uint8_t channel, note;
        uint32_t age;
    } voice[CmfVoiceCount];
    uint32_t clock;
    unsigned tail; /* samples of release tail left after the last event (-1 = not started) */
} CmfPlayer;

bool cmfPlayerStart(CmfPlayer *player, const uint8_t *data, size_t size, unsigned sampleRate);

/* Renders up to `count` mono 16-bit samples (overwrites the buffer); returns how many were produced, fewer only when the track has ended and every voice is silent. */
unsigned cmfPlayerRender(CmfPlayer *player, int16_t *out, unsigned count);

#endif

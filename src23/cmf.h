#ifndef YENDOR23_CMF_H
#define YENDOR23_CMF_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Creative Music Files, the format of the music tracks in WORLD.DAT (audio.h): a "CTMF" header, an AdLib instrument block and a
 * standard-MIDI-style event stream.
 *
 *   +0  "CTMF"   +4 u16 version (0x0101)   +6 u16 instrument block offset   +8 u16 music block offset
 *   +10 u16 ticks per quarter note   +12 u16 clock ticks per second   +14 u16 title / +16 composer / +18 remarks offsets (0 = none)
 *   +20 16 bytes channels in use   +36 u16 instrument count   +38 u16 tempo (quarter notes per minute)
 * Each instrument is 16 bytes of OPL2 register values (operator characteristic/scale/attack-decay/sustain-release/wave for the
 * modulator and carrier, then feedback/connection ...; the standard CMF layout). The music block is a MIDI track without a header:
 * variable-length delta ticks, then events with running status; 0xFF 0x2F ends it. CMF-specific controllers: 0x66 marker, 0x67 melodic (0)
 * / percussive (1) mode, 0x68 pitch up, 0x69 pitch down. Every track of both games parses to its end marker (test_cmf.c).
 */
enum { CmfInstrumentSize = 16 };

typedef struct {
    const uint8_t *data;
    size_t size;
    unsigned version;
    unsigned ticksPerQuarter, ticksPerSecond, tempo;
    unsigned instrumentCount;
    const uint8_t *instruments; /* instrumentCount * 16 bytes */
    const uint8_t *music;
    size_t musicSize;
    uint8_t channelsInUse[16];
} CmfTrack;

/* Validates the header and fills `track`; false if it is not a CTMF block or its offsets do not fit. */
bool cmfOpen(CmfTrack *track, const uint8_t *data, size_t size);

typedef struct {
    uint32_t delta;      /* ticks since the previous event */
    uint8_t status;      /* 0x80-0xEF channel message, 0xF0 sysex, 0xFF meta */
    uint8_t data1, data2; /* channel message data (data2 unused for program change / channel pressure) */
    uint8_t metaType;
    const uint8_t *payload; /* sysex / meta bytes */
    unsigned payloadLength;
} CmfEvent;

typedef struct {
    const uint8_t *position, *end;
    uint8_t runningStatus;
} CmfReader;

void cmfReaderStart(CmfReader *reader, const CmfTrack *track);

/* The next event; false at the end of the data (after the end-of-track meta) or on a malformed event. */
bool cmfReadEvent(CmfReader *reader, CmfEvent *event);

#endif

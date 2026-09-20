/* ags/sound.h -- the SOUNDCLIP class hierarchy. Direct port of
 * apply_structs.py's SAFE_DECLS. This build's SOUNDCLIP base is
 * drastically smaller than 2011's (~0x40 bytes of volume-percentage/
 * positional-audio state) -- just {vtable; done;}. Every field AND
 * every virtual method (poll/set_volume/destroy -- 2011's ~11-slot
 * vtable, including seek/get_pos/get_length_ms/restart/play, is
 * CONFIRMED ABSENT down to just these 3 slots) is individually
 * confirmed for every derived class below; see
 * reversing/notes/struct-layout-drift.md for the complete writeup.
 * IDA structs can't model C++ inheritance, so each derived struct
 * below inlines SOUNDCLIP's own vtbl/done fields at their absolute
 * offsets, matching this project's own convention. The real vtables
 * live in the disassembly's .rdata and aren't reproduced here -- these
 * headers are for the DATA layout only; wiring up an actual matching
 * v-table (or, more likely, a plain C dispatch-by-tag) is later
 * milestone work (src/PLAN.md's M10).
 */
#ifndef AGS_SOUND_H
#define AGS_SOUND_H

#include "ags/types.h"

struct SOUNDCLIP {
    void *vtbl;   /* +0x00 */
    int done;       /* +0x04 */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct SOUNDCLIP) == 8, "SOUNDCLIP must be 8 bytes");

struct MYWAVE {
    void *vtbl;   /* +0x00 */
    int done;       /* +0x04 */
    void *wave;       /* +0x08, the loaded Allegro SAMPLE* */
    int voice;          /* +0x0C, the Allegro voice handle */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct MYWAVE) == 0x10, "MYWAVE must be 0x10 bytes");

struct MYMP3 {
    void *vtbl;   /* +0x00 */
    int done;       /* +0x04 */
    void *stream;     /* +0x08, ALMP3_MP3STREAM* */
    void *in;           /* +0x0C, PACKFILE* */
    void *buffer;         /* +0x10, malloc'd MP3CHUNKSIZE-sized read buffer */
    int chunksize;          /* +0x14 */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct MYMP3) == 0x18, "MYMP3 must be 0x18 bytes");

struct MYSTATICMP3 {
    void *vtbl;   /* +0x00 */
    int done;       /* +0x04 */
    void *tune;       /* +0x08, ALMP3_MP3* */
    int vol;            /* +0x0C, local field (SOUNDCLIP base here has no room for it) */
    void *mp3buffer;      /* +0x10 */
    char repeat;             /* +0x14, local field */
    char _pad_align[3];        /* +0x15..0x18, compiler alignment padding */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct MYSTATICMP3) == 0x18, "MYSTATICMP3 must be 0x18 bytes");

#endif /* AGS_SOUND_H */

#ifndef YENDOR23_AUDIO_H
#define YENDOR23_AUDIO_H

#include <stdbool.h>
#include <stdint.h>

#include "game.h"

/*
 * Where the sound lives in WORLD.DAT. The music tracks are Creative Music Files (a "CTMF" header, an FM instrument block and a
 * MIDI-style event stream) played through SBFMDRV.COM; the effects are Creative Voice Files ("Creative Voice File" header,
 * 8-bit PCM). The original finds them with fixed tables of 32-bit file offsets and 16-bit lengths in its data segment
 * (LookupMusicTrackBlockOffset yendor2.asm:42528, LookupSoundEffectBlockOffset :42941; ids are 1-based, id 0 = no sound; see
 * music.h for which track plays where): Chapter 2 has 21 tracks and 80 effects, Chapter 3 24 and 141. The blocks are
 * contiguous (offset + length = the next offset), music first. A further 2493-byte block just before the first track
 * (Chapter 2 offset 538890, Chapter 3 618714) is the Creative CT-VOICE.DRV driver binary, not game data.
 * Extracted by ida_scripts/dump_audio_tables.py.
 */
enum { AudioMusicTracksYendor2 = 21, AudioMusicTracksYendor3 = 24, AudioEffectsYendor2 = 80, AudioEffectsYendor3 = 141 };

unsigned audioMusicTrackCount(GameKind game);
unsigned audioEffectCount(GameKind game);

/* The offset and length in WORLD.DAT of music track / effect `id` (1-based); false if there is none. */
bool audioMusicTrack(GameKind game, unsigned id, uint32_t *offset, uint32_t *length);
bool audioEffect(GameKind game, unsigned id, uint32_t *offset, uint32_t *length);

#endif

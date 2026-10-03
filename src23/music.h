#ifndef YENDOR23_MUSIC_H
#define YENDOR23_MUSIC_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "game.h"

/*
 * Which music track plays: UpdateAmbientMusicForRegion (yendor2.asm:37699, yendor3.asm identical) and
 * UpdateAmbientMusic (:43589).
 *
 * By map page. The world is cut into the same 40 x 24 pages the local map uses (page = (y / 24) * 20 + x / 40). Whenever
 * the party's page changes, the page's track id is read from a table in WORLD.DAT -- one u16 per page, 120 (Chapter 2,
 * offset 0x71048) / 140 (Chapter 3, 0x83DD0) entries, ids 0-17 / 0-23, 0 = silence (ida_scripts/dump_music_table_offset.py)
 * -- and played.
 *
 * Destinations override it. Travelling to a destination (travel.c) sets a day track and a night track from the
 * destination record (rawA/rawB of TravelDestination in Chapter 2; day = 07:00-19:00 inclusive). Every second or so
 * UpdateAmbientMusic, when its "restart" flag (g_uiScratchFlags4 bit 0x10) is set, plays the forced track if one is set
 * (g_forcedMusicTrack, used by the title and character creation), otherwise the destination's day or night track --
 * but only while g_uiScratchFlags1 bit 0x2000 allows ambient music, and never a track id of 0.
 */
enum {
    MusicTableOffsetYendor2 = 0x71048,
    MusicTableOffsetYendor3 = 0x83DD0,
    MusicDayStartMinutes = 0x1A4,
    MusicDayEndMinutes = 0x474,
    MusicFlagAmbientAllowed = 0x2000
};

unsigned musicPageForPosition(int x, int y);

/* The page's track id from a WORLD.DAT image (0 outside it). */
uint16_t musicPageTrack(GameKind game, const uint8_t *worldDat, size_t size, unsigned page);

/*
 * UpdateAmbientMusicForRegion: given the page last checked, returns true (and the track to play) when the party has
 * moved to a different page; *lastPage is updated.
 */
bool musicRegionChanged(unsigned *lastPage, int x, int y, GameKind game, const uint8_t *worldDat, size_t size, uint16_t *track);

/*
 * UpdateAmbientMusic's choice once its restart flag is set: 0 = play nothing. `forcedTrack` wins outright (even when
 * ambient music is not allowed).
 */
uint16_t musicAmbientChoice(uint16_t forcedTrack, uint16_t dayTrack, uint16_t nightTrack, uint16_t clockMinutes, uint16_t uiFlags1);

#endif

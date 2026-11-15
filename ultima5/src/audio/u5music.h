#ifndef _AUDIO_U5MUSIC_H
#define _AUDIO_U5MUSIC_H

//
// Ultima V music cues of the "Exodus Project: Ultima V Upgrade" (Release 1.0, Michael C. Maggio, 2001).
//
// The upgrade patches ULTIMA.EXE and six overlays so that they call a small driver (MID.DRV) at fixed points of the game;
// MID.DRV in turn plays XMI files through a Midpak TSR.  This module is a port of that driver's decision logic plus the places
// where the game calls it (see docs/u5upgrade_music.md for the analysis).  Nothing here produces sound: the "host" (a ScummVM
// MIDI player, or the trace logger used by the tests) is told which track to start / stop and is asked whether it is still playing.
//
// The driver is a state machine with two variables: the current track and an "auto" flag.  While auto is on, every call to
// U5MUSIC_Update() derives the track from the game state (map, party tile, level); while it is off the current track is kept
// (and restarted when it ends, which is how the XMI files loop).
//

#define U5MUSIC_TRACK_COUNT 16
#define U5MUSIC_NONE 0xff

typedef enum U5MusicTrack
{
    U5MUSIC_THEME = 0,   // U5THEME.XMI  Ultima V Theme (main menu)
    U5MUSIC_BRITLAND,    // BRITLAND.XMI Britannic Lands (overworld)
    U5MUSIC_HORNPIPE,    // HORNPIPE.XMI Cap'n Johne's Hornpipe (ships)
    U5MUSIC_ENGGMNT,     // ENGGMNT.XMI  Engagement and Melee (combat)
    U5MUSIC_STONES,      // STONES.XMI   Stones (shrines, camping, introduction)
    U5MUSIC_GREYSON,     // GREYSON.XMI  Greyson's Tale (keeps)
    U5MUSIC_FANFARE,     // FANFARE.XMI  Fanfare for the Virtuous
    U5MUSIC_MONARCH,     // MONARCH.XMI  The Missing Monarch (Castle of Lord British)
    U5MUSIC_TRNTLLA,     // TRNTLLA.XMI  Villager Tarantella (towns)
    U5MUSIC_HALLS,       // HALLS.XMI    Halls of Doom (dungeons)
    U5MUSIC_WRLDBLW,     // WRLDBLW.XMI  Worlds Below (underworld)
    U5MUSIC_BLCKTHRN,    // BLCKTHRN.XMI Lord Blackthorn (palace of Blackthorn)
    U5MUSIC_LADYNAN,     // LADYNAN.XMI  Dream of Lady Nan (dwellings)
    U5MUSIC_REUNION,     // REUNION.XMI  Joyous Reunion (ending)
    U5MUSIC_RULEBRIT,    // RULEBRIT.XMI Rule Britannia (end of the game)
    U5MUSIC_AMIGA        // AMIGA.XMI    Amiga theme (character creation)
} U5MusicTrack;

// The game state that selects the track while "auto" is on
typedef struct U5MusicGameState
{
    int mapId;     // D_5893_map_id (0 overworld/underworld, 1-0x20 towns..., 0x21-0x28 dungeons, 0xff combat)
    int partyTile; // D_587c_partyTile
    int level;     // D_5895_map_level (non-zero in the underworld)
    int d58a3;     // D_58a3 (non-zero in the final combat)
} U5MusicGameState;

// What plays the music.  Start() begins track `track` (file U5MUSIC_TrackFile(track)) from the start; Stop() stops it;
// IsPlaying() tells whether the started sequence is still running (the driver restarts it when it is not).
typedef struct U5MusicHost
{
    void (*Start)(int track);
    void (*Stop)(void);
    int (*IsPlaying)(void);
} U5MusicHost;

// ---- driver (MID.DRV) ------------------------------------------------------------------------------------------------
extern void U5MUSIC_SetHost(const U5MusicHost* host); // NULL = the built-in trace host
extern const char* U5MUSIC_TrackFile(int track);      // "U5THEME.XMI" ...
extern int U5MUSIC_TrackForGameState(const U5MusicGameState* gs);  // MID.DRV +16d (without the auto test)
extern int U5MUSIC_TrackForIntroScene(int scene);                  // MID.DRV +223
extern int U5MUSIC_TrackForEndingScene(int scene);                 // MID.DRV +24a
extern int U5MUSIC_GetCurrentTrack(void);
extern int U5MUSIC_GetAuto(void);

extern void U5MUSIC_Reset(void);                                   // new driver instance: no track, auto off
// The ten driver entry points, in jump-table order (offset 0x00, 0x03 ... 0x1b)
extern void U5MUSIC_EntryAuto(const U5MusicGameState* gs);         // 00: follow the game state (or keep/restart the current track)
extern void U5MUSIC_EntryStop(void);                               // 03: stop, auto off
extern void U5MUSIC_EntryIntroScene(int scene);                    // 06: track for an introduction scene
extern void U5MUSIC_EntryTheme(void);                              // 09: auto off, play the theme
extern void U5MUSIC_EntryAmiga(void);                              // 0c: play the Amiga theme
extern void U5MUSIC_EntryAutoOn(void);                             // 0f: switch auto on
extern void U5MUSIC_EntryStones(void);                             // 12: auto off, play Stones
extern void U5MUSIC_EntryReunion(void);                            // 15: auto off, play Reunion, then Rule Britannia when it ends
extern void U5MUSIC_EntryEndingScene(int scene);                   // 18: track for an ending scene
extern void U5MUSIC_EntryRuleBritannia(void);                      // 1b: play Rule Britannia

// ---- calls made by the game (the patched call sites) -----------------------------------------------------------------
// Each wrapper is documented with the address of the upgrade's own wrapper (ULTIMA.EXE code offset) and the game function that calls it.
extern void U5MUSIC_Init(void);                  // df1: at start-up, before the intro (loads MID.DRV)
extern void U5MUSIC_Update(void);                // e0e: in front of every key poll of the main wait loop and a few others
extern void U5MUSIC_AutoOn(void);                // e1d
extern void U5MUSIC_PlayTheme(void);             // title screen
extern void U5MUSIC_PlayAmiga(void);             // character creation / transfer from Ultima IV
extern void U5MUSIC_PlayIntroScene(int scene);   // introduction story, per scene
extern void U5MUSIC_PlayStones(void);            // e40: shrines, camping
extern void U5MUSIC_PlayReunion(void);           // ending, first scene
extern void U5MUSIC_PlayEndingScene(int scene);  // ending slides
extern void U5MUSIC_PlayRuleBritannia(void);     // end of the game
extern void U5MUSIC_Stop(void);                  // e26: before the program ends, death and the Blackthorn capture

// ---- key polling with a music refresh --------------------------------------------------------------------------------
// The upgrade replaces the call of ULTIMA_1d5e_PollKey in a few wait loops by a call that refreshes the music first.  Use the macros so
// that the call sites read the same with and without the music (ENABLE_U5_MUSIC).
#if defined(ENABLE_U5_MUSIC)
extern int U5MUSIC_PollKey(void);                // e0e: Update() + PollKey()
extern int U5MUSIC_PollKeyIntro(int scene);      // INTRO.OVL +adc: PlayIntroScene(scene) + PollKey()
extern int U5MUSIC_PollKeyEnding(int scene);     // ENDGAME.OVL +aee: PlayEndingScene(scene) + PollKey()
#define MUSIC_POLLKEY() U5MUSIC_PollKey()
#define MUSIC_POLLKEY_INTRO(scene) U5MUSIC_PollKeyIntro(scene)
#define MUSIC_POLLKEY_ENDING(scene) U5MUSIC_PollKeyEnding(scene)
#else
#define MUSIC_POLLKEY() ULTIMA_1d5e_PollKey()
#define MUSIC_POLLKEY_INTRO(scene) ULTIMA_1d5e_PollKey()
#define MUSIC_POLLKEY_ENDING(scene) ULTIMA_1d5e_PollKey()
#endif

#endif

// Port of the decision logic of MID.DRV from the Exodus Project Ultima V Upgrade 1.0 (see u5music.h and docs/u5upgrade_music.md).
//
// Offsets in the comments are file offsets of MID.DRV.  The driver keeps two bytes at DS:011e/011f (file 1e/1f): the current track
// (ff = none) and the "auto" flag.  It talks to the Midpak TSR through INT 66h: AX=070d load the XMI named by CX:BX, AX=0702 start
// sequence 0, AX=0705 stop, AX=070c status (1 = playing).  tools/verify_midrv.py runs the original driver code in an emulator and
// checks this port against it.

#include "common/common.h"
#include "audio/u5music.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// the names MID.DRV carries at file offset 0x40 (the DATA.OVL patch only changes the name of the driver itself)
static const char* const s_trackFiles[U5MUSIC_TRACK_COUNT] = {
    "U5THEME.XMI", "BRITLAND.XMI", "HORNPIPE.XMI", "ENGGMNT.XMI", "STONES.XMI",   "GREYSON.XMI", "FANFARE.XMI", "MONARCH.XMI",
    "TRNTLLA.XMI", "HALLS.XMI",    "WRLDBLW.XMI", "BLCKTHRN.XMI", "LADYNAN.XMI", "REUNION.XMI", "RULEBRIT.XMI", "AMIGA.XMI"};

static const char* const s_trackNames[U5MUSIC_TRACK_COUNT] = {
    "THEME", "BRITLAND", "HORNPIPE", "ENGGMNT", "STONES", "GREYSON", "FANFARE", "MONARCH",
    "TRNTLLA", "HALLS", "WRLDBLW", "BLCKTHRN", "LADYNAN", "REUNION", "RULEBRIT", "AMIGA"};

const char* U5MUSIC_TrackFile(int track)
{
    return (track >= 0 && track < U5MUSIC_TRACK_COUNT) ? s_trackFiles[track] : NULL;
}

// ---------------------------------------------------------------------------------------------------------------------------
// built-in host: logs the events when U5_MUSIC_LOG names a file (used by tests/smoke_test.py), and pretends the music never ends

static FILE* s_log;
static int s_logInit;
static int s_logPlaying;

static void LogEvent(const char* what, int track)
{
    if (!s_logInit)
    {
        const char* path = getenv("U5_MUSIC_LOG");
        s_logInit = 1;
        if (path && *path)
            s_log = fopen(path, "w");
    }

    if (s_log)
    {
        if (track >= 0)
            fprintf(s_log, "%s %s\n", what, s_trackNames[track]);
        else
            fprintf(s_log, "%s\n", what);
        fflush(s_log);
    }
}

static void TraceStart(int track)
{
    LogEvent("start", track);
    s_logPlaying = 1;
}

static void TraceStop(void)
{
    LogEvent("stop", -1);
    s_logPlaying = 0;
}

static int TraceIsPlaying(void)
{
    return s_logPlaying;
}

static const U5MusicHost s_traceHost = {TraceStart, TraceStop, TraceIsPlaying};
static const U5MusicHost* s_host = &s_traceHost;

void U5MUSIC_SetHost(const U5MusicHost* host)
{
    s_host = host ? host : &s_traceHost;
}

// ---------------------------------------------------------------------------------------------------------------------------
// the driver

static int s_current = U5MUSIC_NONE; // [011e]
static int s_auto;                   // [011f]

int U5MUSIC_GetCurrentTrack(void)
{
    return s_current;
}

int U5MUSIC_GetAuto(void)
{
    return s_auto;
}

void U5MUSIC_Reset(void)
{
    s_current = U5MUSIC_NONE;
    s_auto = 0;
}

// +0ff: start the track in AL (no-op for ff) and remember it
static void StartTrack(int track)
{
    if (track == U5MUSIC_NONE)
        return;

    s_current = track;
    s_host->Start(track); // INT 66h 070d (load the XMI by name), 0702 (start sequence 0)
}

// +134: stop the music, no current track
static void StopTrack(void)
{
    s_host->Stop(); // INT 66h 0705
    s_current = U5MUSIC_NONE;
}

// +27c: restart the current track when its sequence has ended (INT 66h 070c reports the status in AX, 1 = playing)
static void Service(void)
{
    if (s_host->IsPlaying() == 1)
        return;

    StartTrack(s_current);
}

// +267: play `track`; the same track is only serviced (restarted when finished), a different one replaces the current one
static void Play(int track)
{
    if (track == s_current)
    {
        Service();
    }
    else
    {
        StopTrack();
        StartTrack(track);
    }
}

// +16d: the track for the game state; with auto off the current track stays
int U5MUSIC_TrackForGameState(const U5MusicGameState* gs)
{
    int map = gs->mapId & 0xff;

    if (map == 0xff)
        return (gs->d58a3 & 0xff) == 0 ? U5MUSIC_ENGGMNT : U5MUSIC_THEME;

    if (((gs->partyTile & 0xff) & 0xf8) == 0x20)
        return U5MUSIC_HORNPIPE;

    if (map == 0)
        return (gs->level & 0xff) == 0 ? U5MUSIC_BRITLAND : U5MUSIC_WRLDBLW;

    if (map <= 0x08)
        return U5MUSIC_TRNTLLA;
    if (map <= 0x0c)
        return U5MUSIC_LADYNAN;
    if (map <= 0x10)
        return U5MUSIC_GREYSON;
    if (map <= 0x11)
        return U5MUSIC_MONARCH;
    if (map <= 0x12)
        return U5MUSIC_BLCKTHRN;
    if (map <= 0x18)
        return U5MUSIC_GREYSON;
    if (map <= 0x1d)
        return U5MUSIC_LADYNAN;
    if (map <= 0x20)
        return U5MUSIC_FANFARE;
    if (map <= 0x28)
        return U5MUSIC_HALLS;
    return U5MUSIC_NONE;
}

// +223: introduction scene (BL) -> track
int U5MUSIC_TrackForIntroScene(int scene)
{
    scene &= 0xff;
    if (scene <= 0x07)
        return U5MUSIC_STONES;
    if (scene <= 0x0e)
        return U5MUSIC_HALLS;
    if (scene <= 0x15)
        return U5MUSIC_GREYSON;
    return U5MUSIC_NONE;
}

// +24a: ending scene (BL) -> track
int U5MUSIC_TrackForEndingScene(int scene)
{
    scene &= 0xff;
    if (scene <= 0x03)
        return U5MUSIC_STONES;
    if (scene <= 0x07)
        return U5MUSIC_LADYNAN;
    return U5MUSIC_NONE;
}

// entry 00 (+2a9)
void U5MUSIC_EntryAuto(const U5MusicGameState* gs)
{
    int track = s_auto ? U5MUSIC_TrackForGameState(gs) : s_current;
    Play(track);
}

// entry 03 (+2e2)
void U5MUSIC_EntryStop(void)
{
    StopTrack();
    s_auto = 0;
}

// entry 06 (+2b7)
void U5MUSIC_EntryIntroScene(int scene)
{
    Play(U5MUSIC_TrackForIntroScene(scene));
}

// entry 09 (+2c5)
void U5MUSIC_EntryTheme(void)
{
    s_auto = 0;
    Play(U5MUSIC_THEME);
}

// entry 0c (+2d5); does not touch the auto flag
void U5MUSIC_EntryAmiga(void)
{
    Play(U5MUSIC_AMIGA);
}

// entry 0f (+2ee)
void U5MUSIC_EntryAutoOn(void)
{
    s_auto = 1;
}

// entry 12 (+2f7)
void U5MUSIC_EntryStones(void)
{
    s_auto = 0;
    Play(U5MUSIC_STONES);
}

// entry 15 (+307): Reunion, and the current track is then set to Rule Britannia so that the service of a later "Rule Britannia" call
// starts it as soon as Reunion has ended
void U5MUSIC_EntryReunion(void)
{
    s_auto = 0;
    Play(U5MUSIC_REUNION);
    s_current = U5MUSIC_RULEBRIT;
}

// entry 18 (+31c)
void U5MUSIC_EntryEndingScene(int scene)
{
    Play(U5MUSIC_TrackForEndingScene(scene));
}

// entry 1b (+32a)
void U5MUSIC_EntryRuleBritannia(void)
{
    Play(U5MUSIC_RULEBRIT);
}

// ---------------------------------------------------------------------------------------------------------------------------
// glue to the game (not part of the standalone test)

#if !defined(U5MUSIC_TEST)

#include "vars.h"
#include "funcs.h"

void U5MUSIC_Init(void)
{
    // the upgrade loads MID.DRV here (ULTIMA.EXE +df1, using the file name that u5data.exe stores in DATA.OVL)
    U5MUSIC_Reset();
}

void U5MUSIC_Stop(void)
{
    U5MUSIC_EntryStop();
}

void U5MUSIC_Update(void)
{
    U5MusicGameState gs;
    gs.mapId = D_5893_map_id;
    gs.partyTile = D_587c_partyTile;
    gs.level = D_5895_map_level;
    gs.d58a3 = D_58a3;
    U5MUSIC_EntryAuto(&gs);
}

void U5MUSIC_AutoOn(void)
{
    U5MUSIC_EntryAutoOn();
}

void U5MUSIC_PlayTheme(void)
{
    U5MUSIC_EntryTheme();
}

void U5MUSIC_PlayAmiga(void)
{
    U5MUSIC_EntryAmiga();
}

void U5MUSIC_PlayIntroScene(int scene)
{
    U5MUSIC_EntryIntroScene(scene);
}

void U5MUSIC_PlayStones(void)
{
    U5MUSIC_EntryStones();
}

void U5MUSIC_PlayReunion(void)
{
    U5MUSIC_EntryReunion();
}

void U5MUSIC_PlayEndingScene(int scene)
{
    U5MUSIC_EntryEndingScene(scene);
}

void U5MUSIC_PlayRuleBritannia(void)
{
    U5MUSIC_EntryRuleBritannia();
}

int U5MUSIC_PollKey(void)
{
    U5MUSIC_Update();
    return ULTIMA_1d5e_PollKey();
}

int U5MUSIC_PollKeyIntro(int scene)
{
    U5MUSIC_PlayIntroScene(scene);
    return ULTIMA_1d5e_PollKey();
}

int U5MUSIC_PollKeyEnding(int scene)
{
    U5MUSIC_PlayEndingScene(scene);
    return ULTIMA_1d5e_PollKey();
}

#endif

// Standalone driver for tools/verify_midrv.py: runs the port of MID.DRV (src/audio/u5music.c) on a script read from stdin and
// prints the music events and the driver state after every operation.
//
// script lines:  P <n>                 the host reports status n for the next IsPlaying() query (1 = still playing)
//                E0 <map> <tile> <level> <d58a3>
//                E3 | E9 | EC | EF | E12 | E15 | E1B
//                E6 <scene> | E18 <scene>
// output:        "start NAME" / "stop" for host events, then "state <current> <auto>" after each E line

#ifndef U5MUSIC_TEST
#define U5MUSIC_TEST 1
#endif
#include "audio/u5music.h"

#include <stdio.h>
#include <string.h>

static int s_status = 1;

static void HostStart(int track)
{
    printf("load %s\n", U5MUSIC_TrackFile(track));
    printf("start\n");
}

static void HostStop(void)
{
    printf("stop\n");
}

static int HostIsPlaying(void)
{
    return s_status;
}

int main(void)
{
    U5MusicHost host = {HostStart, HostStop, HostIsPlaying};
    char op[16];

    U5MUSIC_SetHost(&host);
    U5MUSIC_Reset();

    while (scanf("%15s", op) == 1)
    {
        int a, b, c, d;

        if (!strcmp(op, "P"))
        {
            scanf("%d", &a);
            s_status = a;
            continue;
        }
        else if (!strcmp(op, "E0"))
        {
            U5MusicGameState gs;
            scanf("%d %d %d %d", &a, &b, &c, &d);
            gs.mapId = a;
            gs.partyTile = b;
            gs.level = c;
            gs.d58a3 = d;
            U5MUSIC_EntryAuto(&gs);
        }
        else if (!strcmp(op, "E3"))
            U5MUSIC_EntryStop();
        else if (!strcmp(op, "E6"))
        {
            scanf("%d", &a);
            U5MUSIC_EntryIntroScene(a);
        }
        else if (!strcmp(op, "E9"))
            U5MUSIC_EntryTheme();
        else if (!strcmp(op, "EC"))
            U5MUSIC_EntryAmiga();
        else if (!strcmp(op, "EF"))
            U5MUSIC_EntryAutoOn();
        else if (!strcmp(op, "E12"))
            U5MUSIC_EntryStones();
        else if (!strcmp(op, "E15"))
            U5MUSIC_EntryReunion();
        else if (!strcmp(op, "E18"))
        {
            scanf("%d", &a);
            U5MUSIC_EntryEndingScene(a);
        }
        else if (!strcmp(op, "E1B"))
            U5MUSIC_EntryRuleBritannia();
        else
        {
            fprintf(stderr, "bad op %s\n", op);
            return 2;
        }

        printf("state %d %d\n", U5MUSIC_GetCurrentTrack(), U5MUSIC_GetAuto());
    }

    return 0;
}

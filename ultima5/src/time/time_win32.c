#include "common/common.h"

#include "event/event.h"

#include <stdlib.h>

#undef ARRAYSIZE
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

void TIME_Initialize(void)
{
}

void TIME_Cleanup(void)
{
}

// U5_VCLOCK=1: virtual clock for fully reproducible test runs.  Time only advances when the game reads it
// (1 ms per read) or sleeps (by the requested amount), so animation frames depend on the sequence of game
// actions, never on wall-clock timing.
static int VirtualClock(void)
{
    static int enabled = -1;
    if (enabled < 0)
        enabled = getenv("U5_VCLOCK") != NULL;
    return enabled;
}

static u32 s_virtualMs;

u32 TIME_GetTicksMs(void)
{
    if (VirtualClock())
        return s_virtualMs++;

    return (u32)GetTickCount64();
}

void TIME_SleepMs(int ms)
{
    if (VirtualClock())
    {
        s_virtualMs += (u32)ms;
        EVT_Yield();
        return;
    }

    // U5_FAST: don't actually wait for the long animation delays (automated tests).  Short (<= 20 ms) sleeps are
    // the per-poll pacing of the keyboard loops (e.g. the intro menu times out after 200 polls) and are kept so that
    // input-driven timing stays realistic.
    if (ms > 20 && getenv("U5_FAST"))
    {
        EVT_Yield();
        return;
    }

    u32 ticks = TIME_GetTicksMs() + (u32)ms;

    EVT_Yield();
    while ((s32)(ticks - TIME_GetTicksMs()) > 0)
    {
        EVT_Yield();
        Sleep(1);
    }
}

u32 TIME_CurrentFrame(void)
{
    // 62.5 hz
    return TIME_GetTicksMs() / 16;
}

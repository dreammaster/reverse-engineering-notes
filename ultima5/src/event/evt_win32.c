#include "common/common.h"
#include "graphics/grap.h"

#include "event.h"

#include <stdlib.h>

extern void GRAP_WIN32_PumpMessages(void);

static EVT_Callback* s_callbacks[16];
static int s_registeredCallbackCount;

void EVT_Initialize(void)
{
}

void EVT_Cleanup(void)
{
}

void EVT_PollMessages(void)
{
    GRAP_WIN32_PumpMessages();
}

void EVT_Yield(void)
{
    GRAP_FlushPendingPresent();
    EVT_PollMessages();

    for (int i = 0; i < s_registeredCallbackCount; i++)
    {
        (*s_callbacks[i])();
    }
}

void EVT_RegisterCallback(EVT_Callback* callback)
{
    ASSERT(s_registeredCallbackCount < ARRAYSIZE(s_callbacks));

    s_callbacks[s_registeredCallbackCount++] = callback;
}

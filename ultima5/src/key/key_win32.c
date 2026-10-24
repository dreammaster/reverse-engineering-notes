// Native Win32 keyboard backend.
//
// Besides real keyboard input this supports a scripted key feed for automated testing (works headless too):
//
//   U5_KEYS="ab{enter}{wait:30}{shot:out.png}{quit}"
//
// Plain characters are delivered one per KEY_PollKey() call.  Tokens: {enter} {esc} {space} {bs} {tab}
// {up} {down} {left} {right} {home} {end} {pgup} {pgdn}, {wait:N} (N empty polls), {sleep:MS} (real-time pause,
// independent of U5_FAST), {shot:file.png} (screenshot), {quit} (exit the process).  When U5_KEYS_EXIT is set the process exits once the script is exhausted.

#include "common/common.h"

#include "vars.h"
#include "macros.h"
#include "time/time.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef ARRAYSIZE
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>

#define KBD_HOME 0x47
#define KBD_UP 0x48
#define KBD_PGUP 0x49
#define KBD_LEFT 0x4B
#define KBD_RIGHT 0x4D
#define KBD_END 0x4f
#define KBD_DOWN 0x50
#define KBD_PGDN 0x51

extern void EVT_Yield(void);
extern int GRAP_WIN32_SaveScreenshot(const char* path);

static u16 s_lastDownKeycode = 0;
static const char* s_script;
static int s_waitPolls;
static u32 s_sleepUntil;

void KEY_Initialize(void)
{
    s_script = getenv("U5_KEYS");
}

void KEY_Cleanup(void)
{
}

// based on ULTIMA_1d5e
static u16 ExtendedToUltimaKeycode(u8 extendedKeycode)
{
    if (extendedKeycode <= 0x44)
    {
        // F1..F10 -> c9..d2
        return extendedKeycode + 0x8e;
    }

    // Arrow keys
    // l, r, d, u, home, end, pgup, pgdn -> 1, 2, 3, 4, d3, d4, d5, d6
    int i;
    for (i = 0; i < 8; i++)
    {
        if (extendedKeycode == D_540e[i])
            break;
    }

    if (i == 8)
        return 0;

    u16 key = D_5416[i];
    key |= 0x100; // special keystroke
    return key;
}

void KEY_WIN32_OnKeyDown(unsigned vk, int ctrl)
{
    (void)ctrl;
    u8 ext;
    switch (vk)
    {
    case VK_HOME: ext = KBD_HOME; break;
    case VK_UP: ext = KBD_UP; break;
    case VK_PRIOR: ext = KBD_PGUP; break;
    case VK_LEFT: ext = KBD_LEFT; break;
    case VK_RIGHT: ext = KBD_RIGHT; break;
    case VK_END: ext = KBD_END; break;
    case VK_DOWN: ext = KBD_DOWN; break;
    case VK_NEXT: ext = KBD_PGDN; break;
    case VK_F12:
        GRAP_WIN32_SaveScreenshot("screenshot.png");
        return;
    default:
        return;
    }

    s_lastDownKeycode = ExtendedToUltimaKeycode(ext);
}

void KEY_WIN32_OnChar(unsigned ch, int ctrl)
{
    if (ch >= 0x80)
        return;

    if (ctrl && ch < 0x20)
    {
        // only the control keys the game knows about; anything else maps to an unused code
        switch (ch)
        {
        case 2: // ctrl-B
        case U5_KEY_CTRL_E:
        case U5_KEY_CTRL_K:
        case U5_KEY_CTRL_M:
        case U5_KEY_CTRL_S:
        case U5_KEY_CTRL_V:
        case U5_KEY_BACKSPACE:
        case U5_KEY_ESC:
            break;
        default:
            ch = 0x1a;
            break;
        }
    }

    s_lastDownKeycode = (u16)ch;
}

// returns 1 and sets *key when the script produced a key this poll
static int ScriptNextKey(u16* key)
{
    static const struct
    {
        const char* name;
        u16 key;
    } named[] = {{"enter", U5_KEY_ENTER}, {"esc", U5_KEY_ESC}, {"space", U5_KEY_SPACE}, {"bs", U5_KEY_BACKSPACE},
                 {"tab", 9}};
    static const struct
    {
        const char* name;
        u8 ext;
    } ext[] = {{"up", KBD_UP}, {"down", KBD_DOWN}, {"left", KBD_LEFT}, {"right", KBD_RIGHT},
               {"home", KBD_HOME}, {"end", KBD_END}, {"pgup", KBD_PGUP}, {"pgdn", KBD_PGDN}};

    if (s_sleepUntil && (s32)(s_sleepUntil - TIME_GetTicksMs()) > 0)
        return 0;
    s_sleepUntil = 0;

    if (s_waitPolls > 0)
    {
        s_waitPolls--;
        return 0;
    }

    while (s_script && *s_script)
    {
        if (*s_script != '{')
        {
            *key = (u8)*s_script++;
            return 1;
        }

        const char* end = strchr(s_script, '}');
        if (!end)
        {
            s_script = NULL;
            break;
        }

        char tok[300];
        size_t len = (size_t)(end - s_script - 1);
        if (len >= sizeof(tok))
            len = sizeof(tok) - 1;
        memcpy(tok, s_script + 1, len);
        tok[len] = 0;
        s_script = end + 1;

        if (!strncmp(tok, "wait:", 5))
        {
            s_waitPolls = atoi(tok + 5);
            return 0;
        }
        if (!strncmp(tok, "sleep:", 6))
        {
            s_sleepUntil = TIME_GetTicksMs() + (u32)atoi(tok + 6);
            if (!s_sleepUntil)
                s_sleepUntil = 1;
            return 0;
        }
        if (!strncmp(tok, "shot:", 5))
        {
            GRAP_WIN32_SaveScreenshot(tok + 5);
            continue;
        }
        if (!strcmp(tok, "quit"))
            exit(0);

        for (size_t i = 0; i < sizeof(named) / sizeof(named[0]); i++)
        {
            if (!strcmp(tok, named[i].name))
            {
                *key = named[i].key;
                return 1;
            }
        }
        for (size_t i = 0; i < sizeof(ext) / sizeof(ext[0]); i++)
        {
            if (!strcmp(tok, ext[i].name))
            {
                *key = ExtendedToUltimaKeycode(ext[i].ext);
                return 1;
            }
        }
    }

    if (s_script && !*s_script && getenv("U5_KEYS_EXIT"))
        exit(0);

    return 0;
}

int KEY_PollKey(void)
{
    int ret;

    D_538a = 0;

    EVT_Yield();

    if (s_script && s_lastDownKeycode == 0)
    {
        u16 key;
        if (ScriptNextKey(&key))
            s_lastDownKeycode = key;
    }

    // special keystroke
    if (s_lastDownKeycode & 0x100)
    {
        s_lastDownKeycode &= 0xff;
        D_538a = 1;
    }

    ret = s_lastDownKeycode;
    s_lastDownKeycode = 0;

    if (ret && getenv("U5_KEYS_TRACE"))
        fprintf(stderr, "key %02x%s\n", ret, D_538a ? " (special)" : "");

    return ret;
}

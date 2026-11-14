// Native Win32 keyboard backend.
//
// Besides real keyboard input this supports a scripted key feed for automated testing (works headless too):
//
//   U5_KEYS="ab{enter}{wait:30}{shot:out.png}{quit}"
//
// Plain characters are delivered one per KEY_PollKey() call.  Tokens: {enter} {esc} {space} {bs} {tab}
// {up} {down} {left} {right} {home} {end} {pgup} {pgdn}, {wait:N} (N empty polls), {sleep:MS} (real-time pause,
// independent of U5_FAST), {shot:file.png} (screenshot), {dump} (print town NPC positions to stderr), {go:x,y} (walk to a town tile), {tm:DLG} (walk to a merchant and talk), {quit} (exit the process).  When U5_KEYS_EXIT is set the process exits once the script is exhausted.

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

// {go:x,y}: walk to a town tile with the arrow keys, planning over the current town map (BFS over brick floors, grass and a
// few furniture-free tiles).  Closed loop: it re-plans every step, so wandering NPCs in the way only cost a retry.
static int s_goX = -1, s_goY = -1, s_goTries;
static signed char s_goBlocked[32][32];
static int s_goLastX = -1, s_goLastY, s_goFromX, s_goFromY;

// Optimistic planner: every tile is walkable except plain walls (4f, 50-57); a step that does not move the party marks its
// target tile as blocked and the route is re-planned, so doors, furniture and wandering NPCs are learned on the way.
static int GoNextStep(u16* key)
{
    static const int dx[4] = {0, 0, -1, 1}, dy[4] = {-1, 1, 0, 0};
    static const u8 ext[4] = {KBD_UP, KBD_DOWN, KBD_LEFT, KBD_RIGHT};
    int sx = D_5896_map_x, sy = D_5897_map_y;

    if (s_goLastX >= 0 && sx == s_goFromX && sy == s_goFromY)
        s_goBlocked[s_goLastY][s_goLastX] = 1; // the last step did not move us
    s_goLastX = -1;

    if ((sx == s_goX && sy == s_goY) || ++s_goTries > 400)
    {
        s_goX = -1;
        return 0;
    }

    static short dist[32][32];
    int qx[1024], qy[1024], head = 0, tail = 0;
    memset(dist, -1, sizeof(dist));
    dist[s_goY][s_goX] = 0;
    qx[tail] = s_goX;
    qy[tail++] = s_goY;
    while (head < tail)
    {
        int cx = qx[head], cy = qy[head++];
        for (int d = 0; d < 4; d++)
        {
            int nx = cx + dx[d], ny = cy + dy[d];
            if (nx < 0 || ny < 0 || nx > 31 || ny > 31 || dist[ny][nx] >= 0 || s_goBlocked[ny][nx])
                continue;
            u8 t = D_6608_map.town[ny][nx];
            if ((t == 0x4f || (t >= 0x50 && t <= 0x57)) && !(nx == sx && ny == sy))
                continue;
            dist[ny][nx] = dist[cy][cx] + 1;
            qx[tail] = nx;
            qy[tail++] = ny;
        }
    }
    for (int d = 0; d < 4; d++)
    {
        int nx = sx + dx[d], ny = sy + dy[d];
        if (nx >= 0 && ny >= 0 && nx < 32 && ny < 32 && dist[ny][nx] >= 0 && dist[ny][nx] < dist[sy][sx])
        {
            *key = ExtendedToUltimaKeycode(ext[d]);
            s_goLastX = nx;
            s_goLastY = ny;
            s_goFromX = sx;
            s_goFromY = sy;
            return 1;
        }
    }
    s_goX = -1; // no route
    return 0;
}

// {tm:DLG}: walk up to the merchant whose dialog number is DLG (129-136) and talk to them across the counter: find a reachable
// tile 1-2 squares from the merchant in a straight line, go there, then press T and the direction.  Re-targets if the merchant
// has wandered off by the time we arrive.
static int s_tmDlg = -1, s_tmTries;
static u16 s_tmQueue[2];
static int s_tmQn, s_tmQi;

static int TalkMerchantStep(u16* key)
{
    static const int dx[4] = {0, 0, -1, 1}, dy[4] = {-1, 1, 0, 0};
    static const u8 ext[4] = {KBD_UP, KBD_DOWN, KBD_LEFT, KBD_RIGHT};
    static short dist[32][32];
    int qx[1024], qy[1024], head = 0, tail = 0;

    if (s_tmQi < s_tmQn)
    {
        *key = s_tmQueue[s_tmQi++];
        if (s_tmQi == s_tmQn)
            s_tmDlg = -1;
        return 1;
    }

    int npc = -1;
    for (int i = 0; i < 0x20; i++)
    {
        if (D_659e[i] != 0 && D_5f5e[i]._0 != 0 && D_5f5e[i]._a == s_tmDlg)
            npc = i;
    }
    if (npc < 0 || ++s_tmTries > 40)
    {
        s_tmDlg = -1;
        return 0;
    }
    int mx = D_5f5e[npc]._2_x, my = D_5f5e[npc]._4_y;
    int sx = D_5896_map_x, sy = D_5897_map_y;

    memset(dist, 0xff, sizeof(dist));
    dist[sy][sx] = 0;
    qx[tail] = sx;
    qy[tail++] = sy;
    while (head < tail)
    {
        int cx = qx[head], cy = qy[head++];
        for (int d = 0; d < 4; d++)
        {
            int nx = cx + dx[d], ny = cy + dy[d];
            if (nx < 0 || ny < 0 || nx > 31 || ny > 31 || dist[ny][nx] >= 0 || s_goBlocked[ny][nx])
                continue;
            u8 t = D_6608_map.town[ny][nx];
            if (t == 0x4f || (t >= 0x50 && t <= 0x57))
                continue;
            dist[ny][nx] = dist[cy][cx] + 1;
            qx[tail] = nx;
            qy[tail++] = ny;
        }
    }

    int best = -1, bx = 0, by = 0, bd = 0;
    for (int d = 0; d < 4; d++)
    {
        for (int k = 1; k <= 2; k++)
        {
            int cx = mx - dx[d] * k, cy = my - dy[d] * k; // standing tile; the merchant lies in direction d from it
            if (cx < 0 || cy < 0 || cx > 31 || cy > 31 || dist[cy][cx] < 0)
                continue;
            if (best < 0 || dist[cy][cx] < best)
            {
                best = dist[cy][cx];
                bx = cx;
                by = cy;
                bd = d;
            }
        }
    }
    if (best < 0)
    {
        s_tmDlg = -1;
        return 0;
    }
    if (best == 0)
    {
        s_tmQueue[0] = 't';
        s_tmQueue[1] = ExtendedToUltimaKeycode(ext[bd]);
        s_tmQn = 2;
        s_tmQi = 0;
        *key = s_tmQueue[s_tmQi++];
        return 1;
    }
    s_goX = bx;
    s_goY = by;
    s_goTries = 0;
    s_goLastX = -1;
    return 0;
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

    if (s_goX >= 0)
    {
        if (GoNextStep(key))
            return 1;
    }

    if (s_tmDlg >= 0)
    {
        if (TalkMerchantStep(key))
            return 1;
        if (s_tmDlg >= 0)
            return 0; // heading for the merchant (go active) or re-targeting next poll
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
        if (!strncmp(tok, "tm:", 3))
        {
            s_tmDlg = atoi(tok + 3);
            s_tmTries = 0;
            s_tmQn = s_tmQi = 0;
            memset(s_goBlocked, 0, sizeof(s_goBlocked));
            return 0;
        }
        if (!strncmp(tok, "go:", 3))
        {
            s_goX = atoi(tok + 3);
            const char* comma = strchr(tok, ',');
            s_goY = comma ? atoi(comma + 1) : 0;
            s_goTries = 0;
            s_goLastX = -1;
            memset(s_goBlocked, 0, sizeof(s_goBlocked));
            return 0;
        }
        if (!strcmp(tok, "quit"))
            exit(0);
        if (!strcmp(tok, "dump"))
        {
            // test authoring aid: party and NPC positions of the current town on stderr
            fprintf(stderr, "DUMP map=%d party=%d,%d hour=%d\n", D_5893_map_id, D_5896_map_x, D_5897_map_y, D_587f);
            for (int i = 0; i < 0x20; i++)
            {
                if (D_659e[i] != 0 && D_5f5e[i]._0 != 0)
                    fprintf(stderr, "NPC %d tile=%02x dlg=%d at %d,%d,%d\n", i, D_659e[i], D_5f5e[i]._a, D_5f5e[i]._2_x, D_5f5e[i]._4_y,
                            D_5f5e[i]._6_z);
            }
            continue;
        }

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

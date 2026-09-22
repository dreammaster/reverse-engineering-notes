/* ags/interaction.h's own implementation. See that header's
 * file-level comment for the complete respond[] evidence and this
 * milestone's own real-vs-stubbed scope.
 */
#include "ags/interaction.h"
#include "ags/room_loader.h"
#include "ags/stub.h"

#include <stdio.h>
#include <string.h>

int ags_get_hotspot_at(const struct RoomStruct *rst, int x, int y)
{
    if (x < 0 || y < 0 || x >= rst->width || y >= rst->height) {
        return 0;
    }
    return getpixel(rst->lookat, x, y);
}

void ags_run_event_block(struct AgsGameContext *ctx, const struct EventBlock *block, int checkAgainst)
{
    int i;

    for (i = 0; i < block->numcmd; i++) {
        if (block->list[i] != checkAgainst) {
            continue;
        }

        switch (block->respond[i]) {
        case 0: { /* NewRoom(respondval) */
            char roomfile[32];
            sprintf(roomfile, "room%d.crm", block->respondval[i]);
            if (ags_load_room(roomfile, ctx->rst) != AGS_ROOM_LOAD_OK) {
                ctx->quit_requested = 1;
            } else if (ctx->player) {
                ctx->player->room = block->respondval[i];
            }
            break;
        }
        case 1: /* no-op ("None") */
            break;
        case 2: /* StopMoving(player) */
            if (ctx->player) {
                ctx->player->walking = 0;
            }
            if (ctx->player_walk) {
                ctx->player_walk->active = 0;
            }
            break;
        case 3: /* run_on_event(GE_MAN_DIES, data) -- named-script-function dispatch, not built yet */
            AGS_STUB_VOID();
            break;
        case 4: /* Run Animation (GameAnimation/AnimationStruct resource table) */
            AGS_STUB_VOID();
            break;
        case 5: /* DisplayMessage(respondval) -- character-attribution extension not ported */
            ags_display_message(ctx->rst, block->respondval[i]);
            break;
        case 6: /* ObjectOff(respondval) -- no RoomObject tracking yet */
            AGS_STUB_VOID();
            break;
        case 7: /* ObjectOff(respondval) + add_inventory(data) -- compound, neither half built yet */
            AGS_STUB_VOID();
            break;
        case 8: /* add_inventory(data) -- no inventory tracking yet */
            AGS_STUB_VOID();
            break;
        case 9: /* Run Script -- named-script-function dispatch, not built yet */
            AGS_STUB_VOID();
            break;
        case 10: /* run_graph_script(respondval) -- M5 never extracts this payload */
            AGS_STUB_VOID();
            break;
        case 11: /* PlaySound(respondval) -- M10's own scope */
            AGS_STUB_VOID();
            break;
        case 12: /* PlayFlic(data, respondval) */
            AGS_STUB_VOID();
            break;
        case 13: /* ObjectOn(respondval) -- no RoomObject tracking yet */
            AGS_STUB_VOID();
            break;
        case 14: /* RunDialog(respondval) -- dialog subsystem not built yet */
            AGS_STUB_VOID();
            break;
        default:
            /* This build's own dispatch treats anything outside
             * [0,14] as a corrupt/unsupported block and quit()s --
             * a real EventBlock this engine loaded should never
             * reach here; log rather than abort. */
            AGS_STUB_VOID();
            break;
        }
    }
}

void ags_run_hotspot_interaction(struct AgsGameContext *ctx, int hotspot, int mood)
{
    int passon = -1;

    switch (mood) {
    case AGS_MODE_TALK: passon = 4; break;
    case AGS_MODE_WALK: passon = 0; break;
    case AGS_MODE_LOOK: passon = 1; break;
    case AGS_MODE_HAND: passon = 2; break;
    case AGS_MODE_PICKUP: passon = 7; break;
    case AGS_MODE_CUSTOM1: passon = 8; break;
    case AGS_MODE_CUSTOM2: passon = 9; break;
    case AGS_MODE_USE: passon = 3; break;
    default: break;
    }

    if (hotspot < 0 || hotspot >= 20) {
        return;
    }

    if (passon >= 0) {
        ags_run_event_block(ctx, &ctx->rst->hscond[hotspot], passon);
    }
    /* "any click on this hotspot" -- always fires too, matching this
     * build's own confirmed single-exit-point run_event_block (no
     * way for the mood-specific pass to suppress it). */
    ags_run_event_block(ctx, &ctx->rst->hscond[hotspot], 5);
}

/* Greedy word-wrap using Allegro's built-in font, into caller-owned
 * line buffers. Returns the number of lines produced (capped at
 * max_lines). */
static int wrap_text(const char *text, int max_width, char lines[][256], int max_lines)
{
    int n = 0;
    const char *p = text;

    while (*p && n < max_lines) {
        char buf[256];
        int len = 0;
        int last_space = -1;

        buf[0] = '\0';
        while (p[len] && len < 250) {
            char tmp[256];
            memcpy(tmp, p, (size_t)len + 1);
            tmp[len + 1] = '\0';
            if (text_length(font, tmp) > max_width) {
                break;
            }
            if (p[len] == ' ') {
                last_space = len;
            }
            len++;
        }

        if (p[len] == '\0') {
            /* rest of the string fits */
            memcpy(buf, p, (size_t)len);
            buf[len] = '\0';
            strcpy(lines[n++], buf);
            p += len;
        } else if (last_space >= 0) {
            memcpy(buf, p, (size_t)last_space);
            buf[last_space] = '\0';
            strcpy(lines[n++], buf);
            p += last_space + 1;
        } else if (len > 0) {
            memcpy(buf, p, (size_t)len);
            buf[len] = '\0';
            strcpy(lines[n++], buf);
            p += len;
        } else {
            break; /* single character wider than max_width -- avoid an infinite loop */
        }
    }

    return n;
}

void ags_display_message(const struct RoomStruct *rst, int msgnum)
{
    char lines[16][256];
    int nlines;
    int line_h = text_height(font);
    int box_w = rst->width - 20;
    int box_h;
    int box_x = 10, box_y;
    int i;
    int frames_waited = 0;
    const int max_wait_frames = 130; /* ~2 seconds at rest(15) */

    if (msgnum < 0 || msgnum >= 100 || !rst->message[msgnum]) {
        return;
    }

    nlines = wrap_text(rst->message[msgnum], box_w - 16, lines, 16);
    box_h = nlines * line_h + 16;
    box_y = rst->height - box_h - 10;
    if (box_y < 0) box_y = 0;

    rectfill(screen, box_x, box_y, box_x + box_w, box_y + box_h, makecol(0, 0, 0));
    rect(screen, box_x, box_y, box_x + box_w, box_y + box_h, makecol(255, 255, 255));
    for (i = 0; i < nlines; i++) {
        textout(screen, font, lines[i], box_x + 8, box_y + 8 + i * line_h,
                makecol(255, 255, 255));
    }

    for (;;) {
        if (keypressed()) {
            readkey();
            break;
        }
        if (mouse_b) {
            while (mouse_b) {
                poll_mouse();
            }
            break;
        }
        if (frames_waited++ >= max_wait_frames) {
            break;
        }
        rest(15);
    }
}

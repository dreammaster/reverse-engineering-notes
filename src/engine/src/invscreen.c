/* ags/invscreen.h's own implementation. See that header for the
 * full scope decision and evidence.
 */
#include "ags/invscreen.h"
#include "ags/inventory.h"
#include "ags/stub.h"

#include <allegro.h>
#include <string.h>

#define AGS_INVSCREEN_ICONSPERLINE 4
#define AGS_INVSCREEN_BUTTON_AREA_H 30
#define AGS_INVSCREEN_MAX_WAIT_FRAMES 2000 /* ~30s at rest(15) */

/* Select/Look/OK -- this build's own reserved icon-bar sprite
 * numbers, confirmed zero drift against real game data
 * (matches.json's own __actual_invscreen entry). */
#define AGS_INVSCREEN_SPR_SELECT 2041
#define AGS_INVSCREEN_SPR_LOOK   2042
#define AGS_INVSCREEN_SPR_OK     2043

void ags_run_inventory_screen(struct AgsGameContext *ctx, struct GameSetupStructBase *game,
                               struct AgsSpriteSet *sprites, int screen_w, int screen_h)
{
    struct CharacterInfo *playerchar = ctx->player;
    struct GameState *play = ctx->play;
    int item_wid = 0, item_hit = 0;
    int i;
    int numinline = AGS_INVSCREEN_ICONSPERLINE;
    int rows;
    int grid_w, grid_h, box_x, box_y, box_w, box_h;
    int look_mode = 0; /* 0 = pick-up/use mode (default), 1 = Look mode */
    int frames_waited = 0;
    int prev_mouse_b = 0;
    int btn_y, btn_x[3];
    BITMAP *btn_spr[3];

    if (!playerchar || !play || !game) {
        AGS_STUB_VOID(); /* no context to run this against */
        return;
    }

    ags_update_invorder(playerchar, play, game->numinvitems);
    if (play->inv_numorder <= 0) {
        ags_display_text_box(screen_w, screen_h, NULL, "You are not carrying anything.");
        return;
    }

    /* Real per-item icon size: the largest owned item's own real
     * sprite dimensions, not a made-up constant. */
    for (i = 0; i < play->inv_numorder; i++) {
        int itemid = play->play_invorder[i];
        int pic = (itemid >= 0 && itemid < game->numinvitems) ? game->invinfo[itemid].pic : 0;
        if (pic > 0) {
            BITMAP *spr = ags_spriteset_load(sprites, pic);
            if (spr) {
                if (spr->w > item_wid) item_wid = spr->w;
                if (spr->h > item_hit) item_hit = spr->h;
                destroy_bitmap(spr);
            }
        }
    }
    if (item_wid < 1) item_wid = 32;
    if (item_hit < 1) item_hit = 32;
    item_wid += 8;
    item_hit += 8;

    rows = (play->inv_numorder + numinline - 1) / numinline;
    grid_w = numinline * item_wid;
    grid_h = rows * item_hit;
    box_w = grid_w;
    box_h = grid_h + AGS_INVSCREEN_BUTTON_AREA_H;
    if (box_w > screen_w - 20) box_w = screen_w - 20;
    if (box_h > screen_h - 20) box_h = screen_h - 20;
    box_x = (screen_w - box_w) / 2;
    box_y = (screen_h - box_h) / 2;

    btn_spr[0] = ags_spriteset_load(sprites, AGS_INVSCREEN_SPR_SELECT);
    btn_spr[1] = ags_spriteset_load(sprites, AGS_INVSCREEN_SPR_LOOK);
    btn_spr[2] = ags_spriteset_load(sprites, AGS_INVSCREEN_SPR_OK);
    btn_y = box_y + grid_h + 2;
    btn_x[0] = box_x + 4;
    btn_x[1] = box_x + 4 + (btn_spr[0] ? btn_spr[0]->w + 4 : 24);
    btn_x[2] = box_x + box_w - (btn_spr[2] ? btn_spr[2]->w + 4 : 24);

    for (;;) {
        int hovered = -1;
        int clicked_ok = 0;

        poll_mouse();
        poll_keyboard();

        if (keypressed()) {
            readkey();
            break; /* __actual_invscreen's own confirmed real behavior: any keypress exits */
        }

        if (mouse_x >= box_x && mouse_x < box_x + grid_w &&
            mouse_y >= box_y && mouse_y < box_y + grid_h) {
            int col = (mouse_x - box_x) / item_wid;
            int row = (mouse_y - box_y) / item_hit;
            int idx = row * numinline + col;
            if (idx >= 0 && idx < play->inv_numorder) {
                hovered = idx;
            }
        }

        if (mouse_b && !prev_mouse_b) {
            if (hovered >= 0) {
                int itemid = play->play_invorder[hovered];
                if (look_mode) {
                    if (itemid >= 0 && itemid < 100) {
                        ags_run_event_block(ctx, &game->__invcond[itemid], 0); /* "Look" */
                    }
                } else {
                    ags_set_active_inventory(playerchar, itemid);
                    break; /* picked up -- close the screen, matching a real item selection */
                }
            } else if (btn_spr[1] && mouse_x >= btn_x[1] && mouse_x < btn_x[1] + btn_spr[1]->w &&
                       mouse_y >= btn_y && mouse_y < btn_y + btn_spr[1]->h) {
                look_mode = !look_mode;
            } else if (mouse_x >= btn_x[2] && mouse_y >= btn_y &&
                       (!btn_spr[2] || (mouse_x < btn_x[2] + btn_spr[2]->w && mouse_y < btn_y + btn_spr[2]->h))) {
                clicked_ok = 1;
            }
        }
        prev_mouse_b = mouse_b;
        if (clicked_ok) {
            break;
        }

        rectfill(screen, box_x, box_y, box_x + box_w - 1, box_y + box_h - 1, makecol(0, 0, 0));
        rect(screen, box_x, box_y, box_x + box_w - 1, box_y + box_h - 1, makecol(255, 255, 255));
        for (i = 0; i < play->inv_numorder; i++) {
            int col = i % numinline;
            int row = i / numinline;
            int cx = box_x + col * item_wid;
            int cy = box_y + row * item_hit;
            int itemid = play->play_invorder[i];
            int pic = (itemid >= 0 && itemid < game->numinvitems) ? game->invinfo[itemid].pic : 0;

            if (i == hovered) {
                rect(screen, cx + 1, cy + 1, cx + item_wid - 2, cy + item_hit - 2, makecol(255, 255, 0));
            }
            if (pic > 0) {
                BITMAP *spr = ags_spriteset_load(sprites, pic);
                if (spr) {
                    draw_sprite(screen, spr, cx + 4, cy + 4);
                    destroy_bitmap(spr);
                }
            }
        }
        if (btn_spr[0]) draw_sprite(screen, btn_spr[0], btn_x[0], btn_y);
        if (btn_spr[1]) draw_sprite(screen, btn_spr[1], btn_x[1], btn_y);
        if (btn_spr[2]) draw_sprite(screen, btn_spr[2], btn_x[2], btn_y);
        if (look_mode) {
            textout(screen, font, "(Look mode)", box_x + 4, btn_y + AGS_INVSCREEN_BUTTON_AREA_H / 4,
                    makecol(255, 255, 255));
        }

        if (frames_waited++ >= AGS_INVSCREEN_MAX_WAIT_FRAMES) {
            break;
        }
        rest(15);
    }

    for (i = 0; i < 3; i++) {
        if (btn_spr[i]) destroy_bitmap(btn_spr[i]);
    }
}

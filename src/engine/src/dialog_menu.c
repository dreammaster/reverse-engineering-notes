/* ags/dialog_menu.h's own implementation. See that header for the
 * full scope decision and evidence.
 */
#include "ags/dialog_menu.h"

#include <allegro.h>
#include <string.h>

#define AGS_DIALOG_MENU_MAX_WAIT_FRAMES 2000 /* ~30s at rest(15) */

int ags_show_dialog_options(const struct DialogTopic *dtpp, const struct GUIMain *dialogface_gui,
                             int screen_w, int screen_h)
{
    int enabled[15];
    int n = 0;
    int i;
    int box_x, box_y, box_w, box_h;
    int line_h = text_height(font);
    int textcol, bgcol;
    int hovered = 0;
    int frames_waited = 0;
    int prev_mouse_b = 0;

    for (i = 0; i < dtpp->numoptions && i < 15; i++) {
        if (dtpp->optionflags[i] & 1) {
            enabled[n++] = i;
        }
    }
    if (n <= 0) {
        return -1; /* source's own "!DoDialog: all options have been turned off" case */
    }

    if (dialogface_gui && dialogface_gui->wid >= 1 && dialogface_gui->hit >= 1) {
        box_x = dialogface_gui->x;
        box_y = dialogface_gui->y;
        box_w = dialogface_gui->wid;
        box_h = dialogface_gui->hit;
        bgcol = dialogface_gui->bgcol;
        textcol = dialogface_gui->fgcol ? dialogface_gui->fgcol : 15;
    } else {
        box_w = screen_w - 20;
        box_h = n * line_h + 16;
        box_x = 10;
        box_y = screen_h - box_h - 10;
        if (box_y < 0) box_y = 0;
        bgcol = 0;
        textcol = 15;
    }

    for (;;) {
        int j;
        int new_hover = hovered;

        poll_mouse();
        poll_keyboard();

        if (mouse_x >= box_x && mouse_x < box_x + box_w &&
            mouse_y >= box_y + 4 && mouse_y < box_y + 4 + n * line_h) {
            new_hover = (mouse_y - (box_y + 4)) / line_h;
            if (new_hover >= n) new_hover = n - 1;
        }

        if (keypressed()) {
            int k = readkey() >> 8;
            if (k == KEY_UP) {
                new_hover--;
            } else if (k == KEY_DOWN) {
                new_hover++;
            } else if (k == KEY_ENTER || k == KEY_SPACE) {
                return enabled[hovered];
            } else if (k == KEY_ESC) {
                return -1;
            }
            if (new_hover < 0) new_hover = n - 1;
            if (new_hover >= n) new_hover = 0;
        }
        hovered = new_hover;

        if (mouse_b && !prev_mouse_b) {
            if (mouse_x >= box_x && mouse_x < box_x + box_w &&
                mouse_y >= box_y + 4 && mouse_y < box_y + 4 + n * line_h) {
                int row = (mouse_y - (box_y + 4)) / line_h;
                if (row >= 0 && row < n) {
                    return enabled[row];
                }
            }
        }
        prev_mouse_b = mouse_b;

        if (bgcol != 0) {
            rectfill(screen, box_x, box_y, box_x + box_w - 1, box_y + box_h - 1, bgcol);
        } else {
            rectfill(screen, box_x, box_y, box_x + box_w - 1, box_y + box_h - 1, makecol(0, 0, 0));
            rect(screen, box_x, box_y, box_x + box_w - 1, box_y + box_h - 1, makecol(255, 255, 255));
        }
        for (j = 0; j < n; j++) {
            int line_y = box_y + 4 + j * line_h;
            int fg = textcol, bg = bgcol;
            if (j == hovered) {
                fg = bgcol;
                bg = textcol;
                rectfill(screen, box_x, line_y, box_x + box_w - 1, line_y + line_h - 1, bg);
            }
            /* A small filled-circle bullet stands in for the real
             * (here, always-absent -- dialog_bullet==0) sprite. */
            circlefill(screen, box_x + 10, line_y + line_h / 2, 3, fg);
            textout(screen, font, dtpp->optionnames[enabled[j]], box_x + 20, line_y, fg);
        }

        if (frames_waited++ >= AGS_DIALOG_MENU_MAX_WAIT_FRAMES) {
            return -1;
        }
        rest(15);
    }
}

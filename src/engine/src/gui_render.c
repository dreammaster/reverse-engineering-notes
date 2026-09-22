/* ags/gui_render.h's own implementation. See that header for the
 * full real-vs-stubbed split and evidence.
 */
#include "ags/gui_render.h"
#include "ags/stub.h"

#include <string.h>

/* Same greedy word-wrap convention as ags/interaction.c's own
 * wrap_text() (DisplayMessage, M9) -- duplicated locally rather than
 * shared across translation units, matching this project's existing
 * per-file-static convention for small helpers. */
static int wrap_text(const char *text, int max_width, char lines[][256], int max_lines)
{
    int nlines = 0;
    const char *p = text;

    while (*p && nlines < max_lines) {
        char tmp[256];
        int last_space = -1;
        int len = 0;

        tmp[0] = '\0';
        while (p[len] && len < 250) {
            char c = p[len];
            tmp[len] = c;
            tmp[len + 1] = '\0';
            if (c == ' ') {
                last_space = len;
            }
            if (text_length(font, tmp) > max_width) {
                if (last_space >= 0) {
                    len = last_space;
                }
                tmp[len] = '\0';
                break;
            }
            len++;
        }

        strncpy(lines[nlines], tmp, 255);
        lines[nlines][255] = '\0';
        nlines++;

        p += len;
        while (*p == ' ') {
            p++;
        }
    }
    return nlines;
}

/* GUIButton::Draw's own pic-based branch (matches.json: "reconfirms
 * isover@+0x68/ispushed@+0x64/pic@+0x54/usepic@+0x60... from a
 * further independent site" -- usepic is the field the mouse-state
 * handlers already keep pointed at the right one of pic/overpic/
 * pushedpic, so drawing it directly is the real behavior, not a
 * simplification). Text-button background/border/label drawing
 * (currentcolor rectfill + 3D-bevel line() border) is NOT ported --
 * see this header's own file-level comment. */
static void draw_button(struct GUIButton *b, struct AgsSpriteSet *sprites, BITMAP *sub)
{
    if (b->usepic > 0) {
        BITMAP *spr = ags_spriteset_load(sprites, b->usepic);
        if (spr) {
            draw_sprite(sub, spr, b->x, b->y);
            destroy_bitmap(spr);
        }
        return;
    }
    AGS_STUB_VOID(); /* text-only button background/border/text -- not yet ported */
}

/* GUILabel::Draw's text branch -- see this header's own file-level
 * comment for the word-wrap simplification. wtextcolor(textcol) is
 * matched directly; GetTranslation/replace_macro_tokens (both
 * confirmed real steps in the original) are NOT applied here since
 * neither translations nor macro tokens are wired into this
 * milestone's own test data. */
static void draw_label(struct GUILabel *l, BITMAP *sub)
{
    char lines[16][256];
    int nlines;
    int line_h;
    int i;

    if (l->text[0] == '\0') {
        return;
    }
    nlines = wrap_text(l->text, l->wid, lines, 16);
    line_h = text_height(font);
    for (i = 0; i < nlines; i++) {
        textout(sub, font, lines[i], l->x, l->y + i * line_h, l->textcol);
    }
}

static void draw_control(void *ctrl, int type, struct AgsSpriteSet *sprites, BITMAP *sub)
{
    switch (type) {
    case 1: /* GOBJ_BUTTON */
        draw_button((struct GUIButton *)ctrl, sprites, sub);
        break;
    case 2: /* GOBJ_LABEL */
        draw_label((struct GUILabel *)ctrl, sub);
        break;
    case 3: /* GOBJ_INVENTORY */
    case 4: /* GOBJ_SLIDER */
    case 5: /* GOBJ_TEXTBOX */
    case 6: /* GOBJ_LISTBOX */
        AGS_STUB_VOID(); /* see this header's own file-level comment */
        break;
    default:
        break;
    }
}

/* GUIMain::draw_at (matches.json's own complete match -- see this
 * header's file-level comment for the exact statement-by-statement
 * citation). */
static void draw_gui(struct GUIMain *gm, struct AgsSpriteSet *sprites, BITMAP *target)
{
    BITMAP *sub;
    int i;

    if (gm->wid < 1 || gm->hit < 1) {
        return;
    }
    sub = create_sub_bitmap(target, gm->x, gm->y, gm->wid, gm->hit);
    if (!sub) {
        return;
    }

    if (gm->fgcol == 0 && gm->bgcol != 0) {
        gm->fgcol = 16;
    }
    if (gm->bgcol != 0) {
        clear_to_color(sub, gm->bgcol); /* get_col8_lookup is the identity at color_depth==1 */
    }
    if (gm->fgcol != gm->bgcol) {
        rect(sub, 0, 0, gm->wid - 1, gm->hit - 1, gm->fgcol); /* wrectangle's own outline, AGS's currentcolor wrapper */
    }
    if (gm->bgpic > 0) {
        BITMAP *spr = ags_spriteset_load(sprites, gm->bgpic);
        if (spr) {
            draw_sprite(sub, spr, 0, 0); /* draw_sprite_compensate(bgpic,0,0,0)'s own useAlpha==0 branch */
            destroy_bitmap(spr);
        }
    }

    for (i = 0; i < gm->numobjs; i++) {
        int type = (gm->objrefptr[i] >> 16) & 0xFFFF;
        if (gm->objs[i]) {
            draw_control(gm->objs[i], type, sprites, sub);
        }
        /* highlightobj's own border draw (wsetcolor(14), not traced
         * past that point in matches.json) is NOT ported. */
    }

    destroy_bitmap(sub);
}

void ags_gui_draw_all(const struct AgsGuiSet *set, struct AgsSpriteSet *sprites, BITMAP *target)
{
    int i;
    for (i = 0; i < set->numgui; i++) {
        /* .on!=0 gate -- array order is draw order (no z-order sort
         * exists in this build, see ags/gui.h's own GUIMain.zorder
         * comment). Cast away const: draw_gui mutates fgcol's own
         * lazy default exactly like the real GUIMain::draw_at does. */
        struct GUIMain *gm = (struct GUIMain *)&set->guis[i];
        if (gm->on) {
            draw_gui(gm, sprites, target);
        }
    }
}

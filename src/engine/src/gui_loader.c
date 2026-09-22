/* ags/gui_loader.h's own implementation. See that header for the
 * full fidelity notes. Every ElementSize/ElementCount pair quoted in
 * a comment below is copied verbatim from the corresponding
 * GUIXxx__ReadFromFile disassembly (rob_blanc_1.asm), read directly
 * for this milestone -- not inferred from matches.json's own prose.
 */
#include "ags/gui_loader.h"

#include <stdlib.h>
#include <string.h>

static int read_le32(FILE *f, int *out)
{
    unsigned char b[4];
    if (fread(b, 1, 4, f) != 4) {
        return -1;
    }
    *out = (int)((unsigned int)b[0] | ((unsigned int)b[1] << 8) |
                 ((unsigned int)b[2] << 16) | ((unsigned int)b[3] << 24));
    return 0;
}

void ags_gui_rebuild_array(struct AgsGuiSet *set, struct GUIMain *gm)
{
    int ff;

    for (ff = 0; ff < gm->numobjs; ff++) {
        int packed = gm->objrefptr[ff];
        int type = (packed >> 16) & 0xFFFF;
        int index = packed & 0xFFFF;

        switch (type) {
        case 1: /* GOBJ_BUTTON */
            gm->objs[ff] = &set->guibuts[index];
            break;
        case 2: /* GOBJ_LABEL */
            gm->objs[ff] = &set->guilabels[index];
            break;
        case 3: /* GOBJ_INVENTORY */
            gm->objs[ff] = &set->guiinv[index];
            break;
        case 4: /* GOBJ_SLIDER */
            gm->objs[ff] = &set->guislider[index];
            break;
        case 5: /* GOBJ_TEXTBOX */
            gm->objs[ff] = &set->guitext[index];
            break;
        case 6: /* GOBJ_LISTBOX */
            gm->objs[ff] = &set->guilist[index];
            break;
        default:
            /* Source's own "guimain: unknown control type found on
             * gui" quit() -- this port just leaves the slot NULL
             * rather than aborting; nothing in this game's own real
             * data ever hits this (every objrefptr comes from this
             * same build's own editor). */
            gm->objs[ff] = NULL;
            break;
        }
    }
}

/* GUIButton::ReadFromFile: fread(&flags,7,4,f) @+4 [flags,x,y,wid,hit,
 * zorder,activated], fread(&pic,12,4,f) @+0x54 [pic..rclickdata],
 * fread(&text,50,1,f) @+0x20, then "if(textcol==0) textcol=16". */
static int read_button(FILE *f, struct GUIButton *b)
{
    if (fread(&b->flags, 4, 7, f) != 7) return -1;
    if (fread(&b->pic, 4, 12, f) != 12) return -1;
    if (fread(b->text, 1, 50, f) != 50) return -1;
    if (b->textcol == 0) b->textcol = 16;
    return 0;
}

/* GUILabel::ReadFromFile: base(28)@+4, text[200](200)@+0x20,
 * [font,textcol,align](12)@+0xE8, then "if(textcol==0) textcol=16". */
static int read_label(FILE *f, struct GUILabel *l)
{
    if (fread(&l->flags, 4, 7, f) != 7) return -1;
    if (fread(l->text, 1, 200, f) != 200) return -1;
    if (fread(&l->font, 4, 3, f) != 3) return -1;
    if (l->textcol == 0) l->textcol = 16;
    return 0;
}

/* GUITextBox::ReadFromFile: identical shape to GUILabel's own (base,
 * text[200], [font,textcol,exflags], textcol default). */
static int read_textbox(FILE *f, struct GUITextBox *t)
{
    if (fread(&t->flags, 4, 7, f) != 7) return -1;
    if (fread(t->text, 1, 200, f) != 200) return -1;
    if (fread(&t->font, 4, 3, f) != 3) return -1;
    if (t->textcol == 0) t->textcol = 16;
    return 0;
}

/* GUIInv::ReadFromFile: only the base 28-byte block -- no getw calls,
 * pre-version-109 code path (confirmed absent: any per-object field). */
static int read_inv(FILE *f, struct GUIInv *iv)
{
    if (fread(&iv->flags, 4, 7, f) != 7) return -1;
    return 0;
}

/* GUISlider::ReadFromFile: base(28)@+4, then [min,max,value,mpressed]
 * (16 bytes, ElementSize=4,ElementCount=4)@+0x20. */
static int read_slider(FILE *f, struct GUISlider *s)
{
    if (fread(&s->flags, 4, 7, f) != 7) return -1;
    if (fread(&s->min, 4, 4, f) != 4) return -1;
    return 0;
}

/* GUIListBox::ReadFromFile: base(28)@+4, [numItems..exflags]
 * (44 bytes, 11 ints, ElementSize=4,ElementCount=0xB)@+0x1B0, then
 * numItems null-terminated strings (read byte-by-byte via fgetc, no
 * length prefix) each malloc'd(strlen+5)+strcpy'd into items[i], then
 * "if([this+0x1D0]==0) [this+0x1D0]=0x10" -- +0x1D0 is textcol
 * (ags/gui.h), matched exactly below. */
static int read_listbox(FILE *f, struct GUIListBox *lb)
{
    int i;

    if (fread(&lb->flags, 4, 7, f) != 7) return -1;
    if (fread(&lb->numItems, 4, 11, f) != 11) return -1;

    if (lb->numItems < 0 || lb->numItems > AGS_GUI_MAX_LISTBOX_ITEMS) {
        return -1;
    }
    for (i = 0; i < lb->numItems; i++) {
        char buf[0x130];
        size_t n = 0;
        int c;
        char *item;

        for (;;) {
            c = fgetc(f);
            if (c == EOF) return -1;
            if (n < sizeof(buf) - 1) {
                buf[n] = (char)c;
            }
            if (c == 0) break;
            n++;
        }
        buf[sizeof(buf) - 1] = '\0';

        item = (char *)malloc(strlen(buf) + 5); /* source's own strlen+5 */
        if (!item) return -1;
        strcpy(item, buf);
        lb->items[i] = item;
    }

    if (lb->textcol == 0) lb->textcol = 16;
    return 0;
}

enum AgsGuiLoadError ags_load_guis(FILE *f, struct GameSetupStructBase *game, struct AgsGuiSet *set)
{
    int gver;
    int i;

    memset(set, 0, sizeof(*set));

    /* "cmp eax, 0CAFEBEEFh ... read_gui: file is corrupt" */
    {
        int magic;
        if (read_le32(f, &magic) != 0) return AGS_GUI_LOAD_READ_ERROR;
        if ((unsigned int)magic != 0xCAFEBEEFu) return AGS_GUI_LOAD_READ_ERROR;
    }

    if (read_le32(f, &gver) != 0) return AGS_GUI_LOAD_READ_ERROR;
    if (gver < 0x64) {
        /* Old, version-less format: the value just read WAS numgui
         * itself, and gver resets to 0 (oldest-format marker used by
         * the version-gated reads further down). */
        set->numgui = gver;
        gver = 0;
    } else if (gver > 0x66) {
        return AGS_GUI_LOAD_BAD_VERSION;
    } else {
        if (read_le32(f, &set->numgui) != 0) return AGS_GUI_LOAD_READ_ERROR;
    }

    if (set->numgui > AGS_GUI_MAX_GUIS) {
        return AGS_GUI_LOAD_TOO_MANY_GUIS;
    }

    if (set->numgui > 0 &&
        fread(set->guis, sizeof(struct GUIMain), (size_t)set->numgui, f) != (size_t)set->numgui) {
        return AGS_GUI_LOAD_READ_ERROR;
    }
    for (i = 0; i < set->numgui; i++) {
        if (set->guis[i].hit < 2) {
            set->guis[i].hit = 2;
        }
        ags_gui_rebuild_array(set, &set->guis[i]);
    }

    if (read_le32(f, &set->numguibuts) != 0) return AGS_GUI_LOAD_READ_ERROR;
    if (set->numguibuts >= AGS_GUI_MAX_CONTROLS) return AGS_GUI_LOAD_TOO_MANY_CONTROLS;
    for (i = 0; i < set->numguibuts; i++) {
        if (read_button(f, &set->guibuts[i]) != 0) return AGS_GUI_LOAD_READ_ERROR;
    }

    if (read_le32(f, &set->numguilabels) != 0) return AGS_GUI_LOAD_READ_ERROR;
    if (set->numguilabels >= AGS_GUI_MAX_CONTROLS) return AGS_GUI_LOAD_TOO_MANY_CONTROLS;
    for (i = 0; i < set->numguilabels; i++) {
        if (read_label(f, &set->guilabels[i]) != 0) return AGS_GUI_LOAD_READ_ERROR;
    }

    if (read_le32(f, &set->numguiinv) != 0) return AGS_GUI_LOAD_READ_ERROR;
    for (i = 0; i < set->numguiinv; i++) {
        if (read_inv(f, &set->guiinv[i]) != 0) return AGS_GUI_LOAD_READ_ERROR;
    }

    /* All three of these are gated on `gver` (the version this
     * specific load used -- 0/0x64/0x65/0x66), matching read_gui's
     * own three-way "cmp var_4, 0x64/0x65/0x66" cascade exactly. */
    if (gver >= 0x64) {
        if (read_le32(f, &set->numguislider) != 0) return AGS_GUI_LOAD_READ_ERROR;
        for (i = 0; i < set->numguislider; i++) {
            if (read_slider(f, &set->guislider[i]) != 0) return AGS_GUI_LOAD_READ_ERROR;
        }
    }
    if (gver >= 0x65) {
        if (read_le32(f, &set->numguitext) != 0) return AGS_GUI_LOAD_READ_ERROR;
        for (i = 0; i < set->numguitext; i++) {
            if (read_textbox(f, &set->guitext[i]) != 0) return AGS_GUI_LOAD_READ_ERROR;
        }
    }
    if (gver >= 0x66) {
        if (read_le32(f, &set->numguilist) != 0) return AGS_GUI_LOAD_READ_ERROR;
        for (i = 0; i < set->numguilist; i++) {
            if (read_listbox(f, &set->guilist[i]) != 0) return AGS_GUI_LOAD_READ_ERROR;
        }
    }

    game->numgui = set->numgui;
    return AGS_GUI_LOAD_OK;
}

/* ags/gui.h -- GUIMain and the six GUIObject-derived control classes
 * (GUIButton/GUITextBox/GUILabel/GUIListBox/GUIInv/GUISlider). Direct
 * port of apply_structs.py's SAFE_DECLS.
 *
 * These six share a common base layout in the original C++
 * (GUIObject: vtbl/flags/x/y/wid/hit/zorder/activated, +0x00..+0x1C,
 * own fields starting at +0x20) but IDA structs can't model C++
 * inheritance, so -- matching this project's own convention -- each
 * struct below inlines those 8 base fields at their absolute offsets
 * rather than embedding a shared `struct GUIObject base;` member (C
 * doesn't have inheritance either, and embedding would shift every
 * derived field's offset by the embedded member's own size unless
 * declared as an anonymous/unnamed struct -- simplest and safest to
 * just repeat the 8 fields, matching the reference IDA decls exactly).
 * The real vtables (GUIObject's own 9 virtual methods: MouseMove/
 * MouseOver/MouseLeave/MouseDown/MouseUp/KeyPress/Draw/WriteToFile/
 * ReadFromFile -- 2011's later IsOverControl/Resized/GetNumEvents/
 * GetEventName/GetEventArgs additions are CONFIRMED ABSENT from every
 * one of these vtables' own shape) aren't reproduced here -- wiring an
 * actual dispatch mechanism is later milestone work (src/PLAN.md's
 * M11+).
 */
#ifndef AGS_GUI_H
#define AGS_GUI_H

#include "ags/types.h"

/* GUIMain -- guis[]'s element type. Its confirmed fields (x/y/
 * numobjs/mouseover/mousedownon/on/objs/objrefptr) match 2011's
 * CURRENT/live GUIMain declaration with zero drift -- a rare case in
 * this project. transparency/zorder/guiId/reserved[6] are MEDIUM
 * confidence positional fits with an exhaustively-checked (28
 * functions touching guis[]) absence of any reader/writer, consistent
 * with this build predating GUI z-order sorting as a feature
 * entirely. */
struct GUIMain {
    char vtext[4];                  /* +0x00 */
    char name[16];                     /* +0x04, MEDIUM confidence: positional only */
    char clickEventHandler[20];           /* +0x14, MEDIUM confidence; CONFIRMED ABSENT behaviorally: no 3-argument click-forwarding exists in this build */
    int x;                                   /* +0x28 */
    int y;                                     /* +0x2C */
    int wid;                                     /* +0x30 */
    int hit;                                       /* +0x34 */
    int focus;                                       /* +0x38, vestigial even in 2011 */
    int numobjs;                                       /* +0x3C */
    int popup;                                           /* +0x40, POPUP_MOUSEY=1/POPUP_SCRIPT=2 */
    int popupyp;                                           /* +0x44 */
    int bgcol;                                               /* +0x48, default 8 */
    int bgpic;                                                 /* +0x4C */
    int fgcol;                                                   /* +0x50, default 1 */
    int mouseover;                                                 /* +0x54, default -1 */
    int mousewasx;                                                   /* +0x58, default -1 */
    int mousewasy;                                                     /* +0x5C, default -1 */
    int mousedownon;                                                     /* +0x60, default -1 */
    int highlightobj;                                                      /* +0x64, default -1 */
    int flags;                                                               /* +0x68, bit0=GUIF_NOCLICK, default 0 */
    int transparency;                                                          /* +0x6C, MEDIUM confidence */
    int zorder;                                                                  /* +0x70, MEDIUM confidence; exhaustively confirmed unused */
    int guiId;                                                                     /* +0x74, MEDIUM confidence */
    int reserved[6];                                                                 /* +0x78..0x90, MEDIUM confidence */
    int on;                                                                            /* +0x90, default 1 */
    void *objs[30];                                                                      /* +0x94 */
    int objrefptr[30];                                                                     /* +0x10C */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct GUIMain) == 0x184, "GUIMain must be 0x184 bytes");

/* GUIButton -- guibuts[81]'s element type (a FIXED array here; 2011's
 * own DynamicArray<GUIButton> is a genuine simplification, not just a
 * smaller capacity number). sizeof(GUIButton)==0x84 confirmed EXACT
 * (not a minimum): both ReadFromFile/WriteToFile are tiny, fully
 * linear functions that never touch anything past rclickdata --
 * 2011's own trailing textAlignment/reserved1/eventHandlers[] fields
 * are CONFIRMED ABSENT. Also carries a new GUIF_DEFAULT bit (0x1,
 * acgui.h:111 -- a genuinely different enum from GUIMain.flags' own
 * bit-0 GUIF_NOCLICK). */
struct GUIButton {
    void *vtbl;              /* +0x00 */
    unsigned int flags;         /* +0x04, base field */
    int x;                        /* +0x08, base field */
    int y;                          /* +0x0C, base field */
    int wid;                          /* +0x10, base field */
    int hit;                            /* +0x14, base field */
    int zorder;                           /* +0x18, base field */
    int activated;                          /* +0x1C, base field */
    char text[50];                            /* +0x20 */
    char _pad_align[2];                          /* +0x52..0x54, compiler alignment padding (50 isn't 4-byte aligned) */
    int pic;                                       /* +0x54 */
    int overpic;                                     /* +0x58 */
    int pushedpic;                                     /* +0x5C */
    int usepic;                                          /* +0x60 */
    int ispushed;                                          /* +0x64 */
    int isover;                                              /* +0x68 */
    int font;                                                  /* +0x6C */
    int textcol;                                                 /* +0x70 */
    int leftclick;                                                 /* +0x74 */
    int rightclick;                                                  /* +0x78 */
    int lclickdata;                                                    /* +0x7C */
    int rclickdata;                                                      /* +0x80 */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct GUIButton) == 0x84, "GUIButton must be 0x84 bytes");

struct GUITextBox {
    void *vtbl;              /* +0x00 */
    unsigned int flags;         /* +0x04, base field */
    int x;                        /* +0x08, base field */
    int y;                          /* +0x0C, base field */
    int wid;                          /* +0x10, base field */
    int hit;                            /* +0x14, base field */
    int zorder;                           /* +0x18, base field */
    int activated;                          /* +0x1C, base field */
    char text[200];                           /* +0x20, TEXTBOX_MAXLEN=49 characters enforced by ReadFromFile */
    int font;                                    /* +0xE8 */
    int textcol;                                    /* +0xEC */
    int exflags;                                      /* +0xF0 */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct GUITextBox) == 0xF4, "GUITextBox must be 0xF4 bytes");

struct GUILabel {
    void *vtbl;              /* +0x00 */
    unsigned int flags;         /* +0x04, base field */
    int x;                        /* +0x08, base field */
    int y;                          /* +0x0C, base field */
    int wid;                          /* +0x10, base field */
    int hit;                            /* +0x14, base field */
    int zorder;                           /* +0x18, base field */
    int activated;                          /* +0x1C, base field */
    char text[200];                           /* +0x20 */
    int font;                                    /* +0xE8 */
    int textcol;                                    /* +0xEC */
    int align;                                         /* +0xF0 */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct GUILabel) == 0xF4, "GUILabel must be 0xF4 bytes");

/* GUIListBox -- 2011's own numItems/selected/topItem/mousexp/mouseyp/
 * rowheight/num_items_fit/font/textcol/backcol/exflags 11-int block
 * confirmed with zero drift. saveGameIndex[] is CONFIRMED ABSENT --
 * this 2002 build predates the field itself. GLF_NOBORDER/
 * GLF_NOARROWS are never checked (border/scrollbar always drawn);
 * selectedbgcol/alignment are never read (selection always uses
 * textcol/backcol, items always left-aligned) -- three confirmed
 * simplifications. */
struct GUIListBox {
    void *vtbl;              /* +0x00 */
    unsigned int flags;         /* +0x04, base field */
    int x;                        /* +0x08, base field */
    int y;                          /* +0x0C, base field */
    int wid;                          /* +0x10, base field */
    int hit;                            /* +0x14, base field */
    int zorder;                           /* +0x18, base field */
    int activated;                          /* +0x1C, base field */
    char *items[100];                         /* +0x20..0x1B0 */
    int numItems;                               /* +0x1B0 */
    int selected;                                 /* +0x1B4 */
    int topItem;                                    /* +0x1B8 */
    int mousexp;                                      /* +0x1BC */
    int mouseyp;                                        /* +0x1C0 */
    int rowheight;                                        /* +0x1C4 */
    int num_items_fit;                                      /* +0x1C8 */
    int font;                                                 /* +0x1CC */
    int textcol;                                                /* +0x1D0 */
    int backcol;                                                  /* +0x1D4, default 7 */
    int exflags;                                                    /* +0x1D8 */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct GUIListBox) == 0x1DC, "GUIListBox must be 0x1DC bytes");

/* GUIInv -- the minimal footprint one would expect from a struct with
 * no charId/itemWidth/itemHeight/topIndex of its own: every layout
 * computation routes through the GLOBAL play_inv_ and inv_item_
 * prefixed fields instead (see ags/gamestate.h). The disabled/greyed-out darkening
 * effect is CONFIRMED ABSENT. */
struct GUIInv {
    void *vtbl;              /* +0x00 */
    unsigned int flags;         /* +0x04, base field */
    int x;                        /* +0x08, base field */
    int y;                          /* +0x0C, base field */
    int wid;                          /* +0x10, base field */
    int hit;                            /* +0x14, base field */
    int zorder;                           /* +0x18, base field */
    int activated;                          /* +0x1C, base field */
    int isover;                               /* +0x20 */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct GUIInv) == 0x24, "GUIInv must be 0x24 bytes");

/* GUISlider -- handlepic/handleoffset/bgimage (custom-graphic slider
 * support) are CONFIRMED ABSENT from this build's own bulk
 * WriteToFile. mpressed confirmed three independent ways (set by
 * MouseDown, cleared by MouseUp, guarded on at the top of MouseMove). */
struct GUISlider {
    void *vtbl;              /* +0x00 */
    unsigned int flags;         /* +0x04, base field */
    int x;                        /* +0x08, base field */
    int y;                          /* +0x0C, base field */
    int wid;                          /* +0x10, base field */
    int hit;                            /* +0x14, base field */
    int zorder;                           /* +0x18, base field */
    int activated;                          /* +0x1C, base field */
    int min;                                  /* +0x20 */
    int max;                                    /* +0x24, default 10 */
    int value;                                    /* +0x28 */
    int mpressed;                                   /* +0x2C */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct GUISlider) == 0x30, "GUISlider must be 0x30 bytes");

#endif /* AGS_GUI_H */

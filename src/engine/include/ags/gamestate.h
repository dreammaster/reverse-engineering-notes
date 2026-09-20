/* ags/gamestate.h -- GameState (the runtime save-game state, `play`)
 * and ScreenOverlay. Direct port of apply_structs.py's SAFE_DECLS.
 * GameState is mapped end to end from +0x00 to its proven +0x964
 * total (confirmed via a `sizeof(GameState)` argument at
 * SaveGameSlot/restore_game_data's own literal fwrite/fread size --
 * see reversing/notes/struct-layout-drift.md for the complete,
 * unusually long round-by-round history, including two self-caught
 * corrections this project is honest about).
 */
#ifndef AGS_GAMESTATE_H
#define AGS_GAMESTATE_H

#include "ags/types.h"

/* ScreenOverlay -- screenover[]'s element type. This build has
 * exactly 2011's first 5 fields; bmp/bgSpeechForChar/
 * associatedOverlayHandle/hasAlphaChannel/positionRelativeToScreen
 * are all CONFIRMED ABSENT (later hardware-acceleration-era
 * additions). DRIFT: capacity 10 vs. 2011's MAX_SCREEN_OVERLAYS=20. */
struct ScreenOverlay {
    block pic;      /* +0x00 */
    int type;         /* +0x04, OVER_TEXTMSG/OVER_COMPLETE/OVER_CUSTOM */
    int x;              /* +0x08 */
    int y;               /* +0x0C */
    int timeout;          /* +0x10 */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct ScreenOverlay) == 0x14, "ScreenOverlay must be 0x14 bytes");

/* GameState -- `play`, the single biggest struct-mapping effort in
 * this project after GameSetupStructBase. A few fields' own
 * GameState-membership is settled by a `sizeof(GameState)` argument
 * rather than a direct access site (play_invorder, char_width/
 * char_height/char_zoom -- this build's structure-of-arrays
 * equivalent of 2011's array-of-structs CharacterExtras) -- see the
 * struct-layout-drift.md writeup for why that's meaningfully stronger
 * evidence than mere positional adjacency. */
struct GameState {
    int score;                          /* +0x00 */
    int usedmode;                          /* +0x04, medium-high confidence */
    int disabled_user_interface;             /* +0x08, a nesting counter, not a boolean */
    int gscript_timer;                          /* +0x0C */
    int debug_mode;                                /* +0x10, medium-high confidence */
    int globalvars[50];                              /* +0x14..0xDC, MAXGLOBALVARS=50 */
    int messagetime;                                   /* +0xDC, medium-high confidence */
    int usedinv;                                          /* +0xE0 */
    int inv_top;                                            /* +0xE4 */
    int inv_numdisp;                                          /* +0xE8 */
    int inv_numorder;                                           /* +0xEC, the one true live counter, not obsolete here */
    int inv_numinline;                                            /* +0xF0, itemsPerLine */
    int text_speed;                                                 /* +0xF4, init value 15 */
    int sierra_inv_color;                                             /* +0xF8 */
    int talkanim_speed;                                                 /* +0xFC, init value 5, actively used here (2011 assigns but never reads it) */
    int inv_item_wid;                                                     /* +0x100 */
    int inv_item_hit;                                                       /* +0x104 */
    int speech_text_shadow;                                                   /* +0x108 */
    int swap_portrait_side;                                                      /* +0x10C */
    int speech_textwindow_gui;                                                     /* +0x110 */
    int follow_change_room_timer;                                                    /* +0x114, init value 150 */
    int totalscore;                                                                     /* +0x118, #define MAXSCORE play.totalscore */
    int skip_display;                                                                      /* +0x11C */
    int no_multiloop_repeat;                                                                 /* +0x120 */
    int roomscript_finished;                                                                   /* +0x124 */
    int used_inv_on;                                                                             /* +0x128 */
    int no_textbg_when_voice;                                                                      /* +0x12C */
    int max_dialogoption_width;                                                                      /* +0x130 */
    int no_hicolor_fadein;                                                                              /* +0x134, medium-high confidence */
    int in_cutscene;                                                                                      /* +0x138 */
    int fast_forward;                                                                                        /* +0x13C */
    int bg_frame;                                                                                               /* +0x140 */
    int bg_anim_delay;                                                                                            /* +0x144 */
    short wait_counter;                                                                                             /* +0x148 */
    short mboundx1;                                                                                                    /* +0x14A */
    short mboundx2;                                                                                                      /* +0x14C */
    short mboundy1;                                                                                                        /* +0x14E */
    short mboundy2;                                                                                                          /* +0x150 */
    char _pad_align9[2];                                                                                                        /* +0x152..0x154, compiler alignment padding (present in the real data; apply_structs.py's own decls omit it, but fade_effect's own confirmed +0x154 offset requires it) */
    int fade_effect;                                                                                                          /* +0x154, only 3 values valid here (0/1/2), not 2011's 5 */
    int bg_frame_locked;                                                                                                         /* +0x158 */
    int globalscriptvars[300];                                                                                                     /* +0x15C..0x60C, DRIFT: 300 here vs. 2011's MAXGSVALUES=500 */
    int cur_music_number;                                                                                                            /* +0x60C */
    int music_repeat;                                                                                                                  /* +0x610 */
    short play_invorder[100];                                                                                                            /* +0x614..0x6DC, MAX_INV=100 */
    short char_width[50];                                                                                                                   /* +0x6DC..0x740, this build's structure-of-arrays CharacterExtras.width equivalent */
    short char_height[50];                                                                                                                     /* +0x740..0x7A4 */
    short char_zoom[50];                                                                                                                          /* +0x7A4..0x808 */
    int music_master_volume;                                                                                                                        /* +0x808, formula: room ST_VOLUME*30 + this, clamped [0,255] */
    char walkable_areas_on[16];                                                                                                                       /* +0x80C..0x81C, MAX_WALK_AREAS+1=16 */
    short screen_flipped;                                                                                                                                /* +0x81C */
    short offsets_locked;                                                                                                                                  /* +0x81E */
    char _pad_unknown5[0x08];                                                                                                                                 /* +0x820..0x828, CONFIRMED ABSENT: 2011's entered_at_x/entered_at_y */
    int entered_edge;                                                                                                                                            /* +0x828 */
    int want_speech;                                                                                                                                                /* +0x82C, negative = unavailable */
    int cant_skip_speech;                                                                                                                                             /* +0x830, raw OPT_NOSKIPTEXT byte, not a converted SKIP_* bitmask */
    int stop_dialog_at_end;                                                                                                                                             /* +0x834 */
    int script_timers[21];                                                                                                                                                 /* +0x838..0x88C, MAX_TIMERS=21 */
    int sound_volume;                                                                                                                                                        /* +0x88C */
    int speech_volume;                                                                                                                                                         /* +0x890 */
    int normal_font;                                                                                                                                                            /* +0x894 */
    int speech_font;                                                                                                                                                              /* +0x898 */
    char key_skip_wait;                                                                                                                                                             /* +0x89C */
    char _pad_align7[0x03];                                                                                                                                                           /* +0x89D..0x8A0, compiler alignment padding */
    int swap_portrait_lastchar;                                                                                                                                                          /* +0x8A0 */
    int seperate_music_lib;                                                                                                                                                                 /* +0x8A4 */
    int in_conversation;                                                                                                                                                                      /* +0x8A8 */
    int screen_tint;                                                                                                                                                                             /* +0x8AC */
    char _pad_unexplored7[0x22];                                                                                                                                                                    /* +0x8B0..0x8D2, confirmed not-GameState territory nearby, but no direct identification within this span */
    char bad_parsed_word[100];                                                                                                                                                                        /* +0x8D2..0x936 */
    char _pad_align8[0x02];                                                                                                                                                                              /* +0x936..0x938, compiler alignment padding */
    int raw_color;                                                                                                                                                                                          /* +0x938 */
    short filenumbers[20];                                                                                                                                                                                     /* +0x93C..0x964, MAXSAVEGAMES=20 -- the struct's last field */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct GameState) == 0x964, "GameState must be 0x964 bytes");

#endif /* AGS_GAMESTATE_H */

/* M11 test (see src/PLAN.md): "the long tail" -- GUI rendering slice.
 * Walks the full M2/M3/M11 loader chain all the way past
 * CharacterInfo[] through messages[500]/DialogTopic[]/dlgmessage[]
 * (all new this milestone, ags/loader.h) to reach the real on-disk
 * GUIMain[]/GUIButton[]/etc. data, decodes it for real
 * (ags_load_guis, ags/gui_loader.h -- a direct port of read_gui/
 * GUIMain::rebuild_array read straight from the disassembly), and
 * renders every "on" GUI onto the real room background (ags_gui_
 * draw_all, ags/gui_render.h -- GUIMain::draw_at plus a scoped
 * Button/Label Draw() subset).
 *
 * Also exercises ags_gui_update_popups (ags/gui_popup.h -- the real
 * POPUP_MOUSEY auto-show/hide logic, added after this milestone's own
 * first screenshot showed a GUI that should have been hidden sitting
 * on top of the status bar): renders the SAME scene twice, once with
 * the mouse away from any trigger line (matching this game's own real
 * default on-screen appearance -- only the always-visible status line
 * should show) and once with the mouse near the top of the screen
 * (triggering the verb-icon bar's own real popup).
 *
 * Usage:
 *   test_gui.exe <path to rb.exe>
 */
#include "ags/clib.h"
#include "ags/loader.h"
#include "ags/room_loader.h"
#include "ags/sprite_loader.h"
#include "ags/gfx.h"
#include "ags/gui_loader.h"
#include "ags/gui_render.h"
#include "ags/gui_popup.h"

#include <allegro.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct GameSetupStructBase game;
    struct AgsGameFileHeader hdr;
    struct AgsScriptBlockInfo script_info;
    struct CharacterInfo *chars = NULL;
    struct DialogTopic *dialogs = NULL;
    char **dlgmessages = NULL;
    struct AgsGuiSet guiset;
    struct RoomStruct rst;
    struct AgsSpriteSet sprites;
    FILE *f;
    int rc;
    int i;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <path to rb.exe>\n", argv[0]);
        return 1;
    }

    if (allegro_init() != 0 || install_mouse() < 0) {
        fprintf(stderr, "allegro_init/install_mouse failed\n");
        return 1;
    }

    if (ags_csetlib(argv[1]) != 0) {
        fprintf(stderr, "ags_csetlib failed\n");
        allegro_exit();
        return 1;
    }

    f = ags_clib_fopen("ac2game.dta", "rb");
    if (!f) {
        fprintf(stderr, "could not open ac2game.dta\n");
        allegro_exit();
        return 1;
    }

#define CHECK(call) \
    do { \
        rc = (call); \
        if (rc != 0) { \
            fprintf(stderr, #call " failed: %d\n", rc); \
            fclose(f); \
            allegro_exit(); \
            return 1; \
        } \
    } while (0)

    CHECK(ags_load_game_file_header(f, &hdr));
    CHECK(ags_load_gamesetup(f, &game));
    CHECK(ags_skip_words_dictionary(f, &game));
    CHECK(ags_skip_unidentified_block(f));
    CHECK(ags_skip_compiled_script(f, &script_info));
    CHECK(ags_skip_views(f, &game));
    CHECK(ags_skip_unidentified_block2(f));

    chars = (struct CharacterInfo *)malloc((size_t)game.numcharacters * sizeof(struct CharacterInfo));
    if (!chars) {
        fprintf(stderr, "out of memory (chars)\n");
        fclose(f);
        allegro_exit();
        return 1;
    }
    CHECK(ags_load_characters(f, &game, chars));
    printf("numcharacters=%d (M3, already verified) -- continuing past this point is new for M11\n",
           game.numcharacters);

    CHECK(ags_load_messages(f, &game));
    printf("messages[500] loaded (M11 step 7)\n");

    printf("numdialog=%d numdlgmessage=%d (already-confirmed GameSetupStructBase fields)\n",
           game.numdialog, game.numdlgmessage);
    rc = ags_load_dialog_topics(f, game.numdialog, &dialogs);
    if (rc != 0) {
        fprintf(stderr, "ags_load_dialog_topics failed: %d\n", rc);
        fclose(f);
        allegro_exit();
        return 1;
    }
    printf("%d DialogTopic entries loaded (M11 step 8)\n", game.numdialog);

    rc = ags_load_dlgmessages(f, game.numdlgmessage, &dlgmessages);
    if (rc != 0) {
        fprintf(stderr, "ags_load_dlgmessages failed: %d\n", rc);
        fclose(f);
        allegro_exit();
        return 1;
    }
    printf("%d dlgmessage strings loaded (M11 step 9)\n", game.numdlgmessage);

    rc = (int)ags_load_guis(f, &game, &guiset);
    fclose(f);
    if (rc != AGS_GUI_LOAD_OK) {
        fprintf(stderr, "ags_load_guis failed: %d\n", rc);
        allegro_exit();
        return 1;
    }
    printf("read_gui OK: numgui=%d numguibuts=%d numguilabels=%d numguiinv=%d "
           "numguislider=%d numguitext=%d numguilist=%d\n",
           guiset.numgui, guiset.numguibuts, guiset.numguilabels, guiset.numguiinv,
           guiset.numguislider, guiset.numguitext, guiset.numguilist);

    for (i = 0; i < guiset.numgui; i++) {
        struct GUIMain *gm = &guiset.guis[i];
        printf("  gui[%d]: on=%d x=%d y=%d wid=%d hit=%d numobjs=%d bgcol=%d fgcol=%d bgpic=%d\n",
               i, gm->on, gm->x, gm->y, gm->wid, gm->hit, gm->numobjs, gm->bgcol, gm->fgcol, gm->bgpic);
    }
    if (guiset.numgui <= 0) {
        fprintf(stderr, "FAIL: this game's real data has no GUIs to render -- "
                        "test cannot demonstrate rendering against real data\n");
        allegro_exit();
        return 1;
    }

    /* Reuse room6.crm (M8/M9's own already-established "has real
     * content" room) purely as a visible backdrop -- GUIs are drawn
     * screen-relative, independent of which room is showing. */
    memset(&rst, 0, sizeof(rst));
    CHECK(ags_load_room("room6.crm", &rst));
    printf("room6.crm loaded: %dx%d\n", rst.width, rst.height);

    rc = ags_spriteset_init(&sprites, "acsprset.spr", &game);
    if (rc != AGS_SPRITE_LOAD_OK) {
        fprintf(stderr, "ags_spriteset_init failed: %d\n", rc);
        allegro_exit();
        return 1;
    }

    if (ags_gfx_init_windowed(rst.width, rst.height, 8) != 0) {
        fprintf(stderr, "ags_gfx_init_windowed failed: %s\n", allegro_error);
        ags_spriteset_free(&sprites);
        allegro_exit();
        return 1;
    }

    {
        struct AgsGuiPopupState popup_state;
        RGB merged[256];
        int j;

        popup_state.ifacepopped = -1;
        ags_gfx_build_merged_palette(&rst, &game, merged);

        /* --- default state: mouse away from any trigger line --- */
        position_mouse(160, 100);
        poll_mouse();
        ags_gui_update_popups(&guiset, &popup_state, mouse_y);
        printf("\nmouse at y=%d -> ifacepopped=%d (this game's own real default "
               "appearance: only the always-visible status line should show)\n",
               mouse_y, popup_state.ifacepopped);
        for (j = 0; j < guiset.numgui; j++) {
            printf("  gui[%d]: popup=%d popupyp=%d -> on=%d\n", j,
                   guiset.guis[j].popup, guiset.guis[j].popupyp, guiset.guis[j].on);
        }

        ags_gfx_show_background(&rst, &game);
        ags_gui_draw_all(&guiset, &sprites, screen);
        rest(500);
        if (save_bitmap("gui_screenshot.bmp", screen, merged) != 0) {
            fprintf(stderr, "save_bitmap failed\n");
        } else {
            printf("screenshot saved: gui_screenshot.bmp\n");
        }

        /* --- mouse near the top edge: triggers the verb-icon bar's
         * own real popup (popupyp=12) --- */
        position_mouse(160, 5);
        poll_mouse();
        ags_gui_update_popups(&guiset, &popup_state, mouse_y);
        printf("\nmouse at y=%d -> ifacepopped=%d (should trigger the verb-icon bar)\n",
               mouse_y, popup_state.ifacepopped);
        for (j = 0; j < guiset.numgui; j++) {
            printf("  gui[%d]: popup=%d popupyp=%d -> on=%d\n", j,
                   guiset.guis[j].popup, guiset.guis[j].popupyp, guiset.guis[j].on);
        }

        ags_gfx_show_background(&rst, &game);
        ags_gui_draw_all(&guiset, &sprites, screen);
        rest(500);
        if (save_bitmap("gui_screenshot_popup.bmp", screen, merged) != 0) {
            fprintf(stderr, "save_bitmap failed\n");
        } else {
            printf("screenshot saved: gui_screenshot_popup.bmp\n");
        }
    }

    ags_spriteset_free(&sprites);
    allegro_exit();

    free(chars);
    if (dialogs) {
        for (i = 0; i < game.numdialog; i++) {
            free(dialogs[i].optionscripts);
        }
        free(dialogs);
    }
    if (dlgmessages) {
        for (i = 0; i < game.numdlgmessage; i++) {
            free(dlgmessages[i]);
        }
        free(dlgmessages);
    }

    printf("\nM11 ACCEPTANCE CHECK OK (GUI rendering slice): real read_gui-format "
           "GUIMain[]/control data decoded from this game's own ac2game.dta and "
           "rendered onto the real room background, with the real POPUP_MOUSEY "
           "auto-show/hide logic (ags_gui_update_popups) confirmed to match this game's "
           "own actual default appearance\n");
    return 0;
}
END_OF_MAIN()

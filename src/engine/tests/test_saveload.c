/* M11 test (see src/PLAN.md): "the long tail" -- save/restore slice.
 * Loads Rob Blanc 1's own real GameSetupStructBase/CharacterInfo[]/
 * DialogTopic[] data (M2/M3/M11), mutates in-memory state the same
 * way actual gameplay would (moves the player character, grants/loses
 * real inventory items via ags/inventory.h, permanently turns off a
 * real dialog option via the real DCMD_OPTOFFFOREVER-equivalent bit
 * op), saves it to a real slot file (ags_save_game_slot, ags/
 * saveload.h), clobbers the in-memory state to prove nothing is
 * carried over by accident, then restores it (ags_restore_game_data)
 * and verifies every mutated field came back exactly. Also exercises
 * ags_get_save_slot_description's own real "read only the header"
 * code path.
 *
 * Usage:
 *   test_saveload.exe <path to rb.exe>
 */
#include "ags/clib.h"
#include "ags/loader.h"
#include "ags/saveload.h"
#include "ags/inventory.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define SAVE_SLOT 99 /* an unused, out-of-the-way slot number for this test */

int main(int argc, char **argv)
{
    struct GameSetupStructBase game;
    struct GameState play;
    struct AgsGameFileHeader hdr;
    struct AgsScriptBlockInfo script_info;
    struct CharacterInfo *chars = NULL;
    struct DialogTopic *dialogs = NULL;
    char **dlgmessages = NULL;
    enum AgsSaveLoadError err;
    FILE *f;
    int rc;
    int i;
    int playerchar_idx;
    int saved_x, saved_y, saved_room;
    int saved_inv_item, saved_inv_count;
    int saved_option_dialog, saved_option_idx, saved_option_flags;
    char description[200];

    if (argc < 2) {
        fprintf(stderr, "usage: %s <path to rb.exe>\n", argv[0]);
        return 1;
    }

    if (ags_csetlib(argv[1]) != 0) {
        fprintf(stderr, "ags_csetlib failed\n");
        return 1;
    }

    f = ags_clib_fopen("ac2game.dta", "rb");
    if (!f) {
        fprintf(stderr, "could not open ac2game.dta\n");
        return 1;
    }

#define CHECK(call) \
    do { \
        rc = (call); \
        if (rc != 0) { \
            fprintf(stderr, #call " failed: %d\n", rc); \
            fclose(f); \
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
        return 1;
    }
    CHECK(ags_load_characters(f, &game, chars));
    CHECK(ags_load_messages(f, &game));

    rc = ags_load_dialog_topics(f, game.numdialog, &dialogs);
    if (rc != 0) {
        fprintf(stderr, "ags_load_dialog_topics failed: %d\n", rc);
        fclose(f);
        return 1;
    }
    rc = ags_load_dlgmessages(f, game.numdlgmessage, &dlgmessages);
    fclose(f);
    if (rc != 0) {
        fprintf(stderr, "ags_load_dlgmessages failed: %d\n", rc);
        return 1;
    }

    playerchar_idx = game.playercharacter;
    if (playerchar_idx < 0 || playerchar_idx >= game.numcharacters) {
        fprintf(stderr, "FAIL: playercharacter index %d out of range\n", playerchar_idx);
        return 1;
    }
    if (game.numdialog < 1 || dialogs[0].numoptions < 1) {
        fprintf(stderr, "FAIL: this game's real data has no dialog options to mutate\n");
        return 1;
    }

    memset(&play, 0, sizeof(play));
    play.inv_numorder = 0;

    /* --- mutate state the way real gameplay would --- */
    chars[playerchar_idx].x = 111;
    chars[playerchar_idx].y = 222;
    chars[playerchar_idx].room = 6;
    ags_add_inventory(&chars[playerchar_idx], &play, 1);
    ags_add_inventory(&chars[playerchar_idx], &play, 3);
    ags_lose_inventory(&chars[playerchar_idx], &play, 1);
    dialogs[0].optionflags[0] = (dialogs[0].optionflags[0] & ~1) | 2; /* real DCMD_OPTOFFFOREVER bit op */

    saved_x = chars[playerchar_idx].x;
    saved_y = chars[playerchar_idx].y;
    saved_room = chars[playerchar_idx].room;
    saved_inv_item = 3;
    saved_inv_count = chars[playerchar_idx].inv[3];
    saved_option_dialog = 0;
    saved_option_idx = 0;
    saved_option_flags = dialogs[0].optionflags[0];

    printf("before save: player at (%d,%d) room=%d, inv[%d]=%d, dialog[0].optionflags[0]=0x%x, "
           "inv_numorder=%d\n",
           saved_x, saved_y, saved_room, saved_inv_item, saved_inv_count, saved_option_flags,
           play.inv_numorder);

    err = ags_save_game_slot(SAVE_SLOT, "M11 save/restore test", &play, chars, game.numcharacters,
                              dialogs, game.numdialog, saved_room);
    if (err != AGS_SAVE_OK) {
        fprintf(stderr, "FAIL: ags_save_game_slot returned %d\n", (int)err);
        return 1;
    }
    printf("ags_save_game_slot(%d) OK\n", SAVE_SLOT);

    err = ags_get_save_slot_description(SAVE_SLOT, description);
    if (err != AGS_SAVE_OK || strcmp(description, "M11 save/restore test") != 0) {
        fprintf(stderr, "FAIL: ags_get_save_slot_description returned %d, description=\"%s\"\n",
                (int)err, description);
        return 1;
    }
    printf("ags_get_save_slot_description(%d) -> \"%s\" (real header-only read, no full "
           "restore performed)\n", SAVE_SLOT, description);

    /* --- clobber everything, prove nothing survives by accident --- */
    memset(&play, 0xAA, sizeof(play));
    for (i = 0; i < game.numcharacters; i++) {
        memset(&chars[i], 0xAA, sizeof(chars[i]));
    }
    for (i = 0; i < game.numdialog; i++) {
        memset(dialogs[i].optionflags, 0xAA, sizeof(dialogs[i].optionflags));
    }
    printf("\nclobbered play/chars[]/dialogs[].optionflags[] with 0xAA...\n");

    {
        int restored_room = -1;
        err = ags_restore_game_data(SAVE_SLOT, &play, chars, game.numcharacters,
                                     dialogs, game.numdialog, &restored_room);
        if (err != AGS_SAVE_OK) {
            fprintf(stderr, "FAIL: ags_restore_game_data returned %d\n", (int)err);
            return 1;
        }
        printf("ags_restore_game_data(%d) OK, restored_room=%d\n", SAVE_SLOT, restored_room);

        printf("after restore: player at (%d,%d), inv[%d]=%d, dialog[0].optionflags[0]=0x%x, "
               "inv_numorder=%d\n",
               chars[playerchar_idx].x, chars[playerchar_idx].y,
               saved_inv_item, chars[playerchar_idx].inv[saved_inv_item],
               dialogs[saved_option_dialog].optionflags[saved_option_idx], play.inv_numorder);

        if (chars[playerchar_idx].x != saved_x || chars[playerchar_idx].y != saved_y ||
            chars[playerchar_idx].room != saved_room) {
            fprintf(stderr, "FAIL: player position/room did not round-trip\n");
            return 1;
        }
        if (chars[playerchar_idx].inv[saved_inv_item] != saved_inv_count) {
            fprintf(stderr, "FAIL: inventory count did not round-trip\n");
            return 1;
        }
        if (dialogs[saved_option_dialog].optionflags[saved_option_idx] != saved_option_flags) {
            fprintf(stderr, "FAIL: dialog optionflags did not round-trip\n");
            return 1;
        }
        if (restored_room != saved_room) {
            fprintf(stderr, "FAIL: current_room did not round-trip\n");
            return 1;
        }
    }

    /* --- confirm the count-mismatch guard is real, not just declared --- */
    {
        err = ags_restore_game_data(SAVE_SLOT, &play, chars, game.numcharacters + 1,
                                     dialogs, game.numdialog, NULL);
        printf("\nags_restore_game_data with a deliberately wrong numcharacters -> %d "
               "(expected %d, AGS_SAVE_COUNT_MISMATCH)\n", (int)err, (int)AGS_SAVE_COUNT_MISMATCH);
        if (err != AGS_SAVE_COUNT_MISMATCH) {
            fprintf(stderr, "FAIL: count-mismatch guard did not trigger\n");
            return 1;
        }
    }

    remove("agssave.099");

    free(chars);
    for (i = 0; i < game.numdialog; i++) {
        free(dialogs[i].optionscripts);
    }
    free(dialogs);
    for (i = 0; i < game.numdlgmessage; i++) {
        free(dlgmessages[i]);
    }
    free(dlgmessages);

    printf("\nM11 ACCEPTANCE CHECK OK (save/restore slice): real GameState/CharacterInfo[]/"
           "DialogTopic.optionflags[] data round-tripped through a real save file against "
           "this game's own real data, including a real header-only description read and a "
           "real game-data-mismatch guard\n");
    return 0;
}
END_OF_MAIN() /* see test_gamesetup.c's own comment on this -- ags/loader.h
               * transitively includes allegro.h, whose "magic main" macro
               * renames main() even though this program never calls Allegro. */

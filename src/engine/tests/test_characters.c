/* M3 test (see src/PLAN.md): "Meet the cast." Walks load_game_file's
 * own byte-consumption sequence past GameSetupStructBase (M2) through
 * WordsDictionary/the compiled script/ViewStruct272[]/a second
 * unidentified skip, to reach and decode the real CharacterInfo
 * array, matching reversing/scripts/dump_characters_from_data.py's
 * own (corrected -- see that script's own decode_character()) output
 * field for field. Usage:
 *   test_characters.exe <path to rb.exe>
 */
#include "ags/clib.h"
#include "ags/loader.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct GameSetupStructBase game;
    struct AgsGameFileHeader hdr;
    struct AgsScriptBlockInfo script_info;
    struct CharacterInfo *chars;
    FILE *f;
    int rc, i;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <path to rb.exe>\n", argv[0]);
        return 1;
    }

    rc = ags_csetlib(argv[1]);
    if (rc != 0) {
        fprintf(stderr, "ags_csetlib failed: %d\n", rc);
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

    printf("fread_script sizes: fileVer=%d globaldatasize=%d codesize=%d "
           "stringssize=%d numfixups=%d numimports=%d numexports=%d\n",
           script_info.fileVer, script_info.globaldatasize, script_info.codesize,
           script_info.stringssize, script_info.numfixups, script_info.numimports,
           script_info.numexports);
    printf("numcharacters: %d\n", game.numcharacters);

    if (game.numcharacters <= 0) {
        fprintf(stderr, "FAIL: numcharacters is %d\n", game.numcharacters);
        fclose(f);
        return 1;
    }

    chars = (struct CharacterInfo *)malloc((size_t)game.numcharacters * sizeof(struct CharacterInfo));
    if (!chars) {
        fprintf(stderr, "out of memory\n");
        fclose(f);
        return 1;
    }
    CHECK(ags_load_characters(f, &game, chars));
    fclose(f);

    for (i = 0; i < game.numcharacters; i++) {
        struct CharacterInfo *c = &chars[i];
        printf("  [%d] defview=%d talkview=%d view=%d room=%d prevroom=%d x=%d y=%d "
               "name=%s scrname=%s\n",
               i, c->defview, c->talkview, c->view, c->room, c->prevroom, c->x, c->y,
               c->name, c->scrname);
    }

    /* M3's own stated acceptance check (src/PLAN.md): the real 5
     * characters, by name and script name. */
    {
        static const char *expect_name[5] = {"ROB", "HIGH ONE", "HIGH ONE", "DROID", "HOLOGRAM"};
        static const char *expect_scrname[5] = {"ROB", "HIGHONE", "HIGHTWO", "DROID", "HOLO"};
        int ok = (game.numcharacters == 5);
        if (ok) {
            for (i = 0; i < 5; i++) {
                if (strcmp(chars[i].name, expect_name[i]) != 0 ||
                    strcmp(chars[i].scrname, expect_scrname[i]) != 0) {
                    ok = 0;
                    break;
                }
            }
        }
        free(chars);
        if (!ok) {
            fprintf(stderr, "\nFAIL: character roster does not match the expected 5 "
                             "(ROB/HIGHONE/HIGHTWO/DROID/HOLO)\n");
            return 1;
        }
        printf("\nM3 ACCEPTANCE CHECK OK: all 5 characters (ROB, HIGH ONE x2, "
               "DROID, HOLOGRAM) decoded correctly\n");
    }

    return 0;
}
END_OF_MAIN() /* see test_gamesetup.c's own comment on this -- ags/loader.h
               * transitively includes allegro.h, whose "magic main" macro
               * renames main() even though this program never calls Allegro. */

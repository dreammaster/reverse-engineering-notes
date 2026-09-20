/* M2 test (see src/PLAN.md): "Read the header." Loads ac2game.dta out
 * of a CLIB-packaged file (via M1's ags/clib.h) and prints a handful
 * of GameSetupStructBase fields, matching
 * reversing/scripts/dump_gamesetup_from_data.py's own field list for
 * a direct comparison against that already-verified Python
 * prototype's output. Usage:
 *   test_gamesetup.exe <path to rb.exe (or ac2game.dat)>
 */
#include "ags/clib.h"
#include "ags/loader.h"

#include <stdio.h>
#include <string.h>

static void print_bytes(const char *label, const unsigned char *buf, int n)
{
    int i;
    printf("  %-16s = [", label);
    for (i = 0; i < n; i++) {
        printf("%s%d", i ? ", " : "", buf[i]);
    }
    printf("]\n");
}

int main(int argc, char **argv)
{
    struct GameSetupStructBase game;
    struct AgsGameFileHeader hdr;
    FILE *f;
    int rc;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <path to rb.exe or ac2game.dat>\n", argv[0]);
        return 1;
    }

    rc = ags_csetlib(argv[1]);
    if (rc != 0) {
        fprintf(stderr, "ags_csetlib(\"%s\") failed: %d\n", argv[1], rc);
        return 1;
    }

    f = ags_clib_fopen("ac2game.dta", "rb");
    if (!f) {
        fprintf(stderr, "could not open ac2game.dta via ags_clib_fopen\n");
        return 1;
    }

    rc = ags_load_game_file_header(f, &hdr);
    if (rc != 0) {
        fprintf(stderr, "ags_load_game_file_header failed: %d\n", rc);
        fclose(f);
        return 1;
    }
    printf("ac2game.dta header: teststr=%s marker=%d verstr=%s\n",
           hdr.teststr, hdr.marker, hdr.verstr);

    rc = ags_load_gamesetup(f, &game);
    fclose(f);
    if (rc != 0) {
        fprintf(stderr, "ags_load_gamesetup failed: %d\n", rc);
        return 1;
    }

    printf("GameSetupStructBase fields:\n");
    printf("  %-16s = %s\n", "gamename", game.gamename);
    print_bytes("options[20]", game.options, 20);
    printf("  %-16s = %d\n", "numiface", game.numiface);
    printf("  %-16s = %d\n", "numviews", game.numviews);
    printf("  %-16s = %d\n", "numcharacters", game.numcharacters);
    printf("  %-16s = %u\n", "numinvitems", (unsigned)game.numinvitems);
    printf("  %-16s = %d\n", "numdialog", game.numdialog);
    printf("  %-16s = %d\n", "numfonts", game.numfonts);
    printf("  %-16s = %d\n", "color_depth", game.color_depth);
    printf("  %-16s = %d\n", "uniqueid", game.uniqueid);
    print_bytes("langcodes", (const unsigned char *)game.langcodes, 15);
    printf("  %-16s = %d\n", "numgui", game.numgui);

    /* M2's own stated acceptance check (src/PLAN.md). */
    if (strcmp(game.gamename, "Rob Blanc I") != 0) {
        fprintf(stderr, "\nFAIL: gamename is %s, expected \"Rob Blanc I\"\n", game.gamename);
        return 1;
    }
    if (game.numfonts != 3) {
        fprintf(stderr, "\nFAIL: numfonts is %d, expected 3\n", game.numfonts);
        return 1;
    }
    printf("\nM2 ACCEPTANCE CHECK OK: gamename==\"Rob Blanc I\", numfonts==3\n");
    return 0;
}
END_OF_MAIN() /* ags/gamesetup.h transitively includes allegro.h (for `block`),
               * whose own "magic main" macro renames main() -- needed here even
               * though this program never calls an Allegro function directly. */

/* M1 test (see src/PLAN.md): "Read the manifest." Prints a CLIB
 * asset library's contents in the exact same format
 * reversing/scripts/parse_clib_manifest.py already prints, so the two
 * can be diffed directly against each other -- the actual acceptance
 * test for this milestone. Usage:
 *   test_clib_manifest.exe <path to ac2game.dat or rb.exe>
 */
#include "ags/clib.h"

#include <stdio.h>
#include <string.h>

int main(int argc, char **argv)
{
    int n, i;

    if (argc < 2) {
        fprintf(stderr, "usage: %s <path to a CLIB-format file>\n", argv[0]);
        return 1;
    }

    {
        int rc = ags_csetlib(argv[1]);
        if (rc != 0) {
            fprintf(stderr, "ags_csetlib(\"%s\") failed: %d\n", argv[1], rc);
            return 1;
        }
    }

    n = ags_clib_get_num_files();
    /* Matches reversing/scripts/parse_clib_manifest.py's own print
     * format exactly, field for field, so the two outputs can be
     * diffed directly against each other. */
    printf("CLIB lib_version=%d, %d files\n", ags_clib_get_lib_version(), n);
    for (i = 0; i < n; i++) {
        const char *name = ags_clib_get_file_name(i);
        long off = ags_clib_offset(name);
        long len = ags_clib_file_size(name);
        printf("  %-20s offset=%-10ld length=%ld\n", name, off, len);
    }

    /* Bonus sanity check (not part of the Python-diffed output above):
     * ags_clib_fopen()/ags_clib_open_file() aren't exercised by the
     * manifest print itself, but M2 depends on them directly -- worth
     * verifying now that the returned FILE* is genuinely seeked to
     * the right spot and reports the right size. */
    if (ags_clib_find_index("ac2game.dta") >= 0) {
        FILE *f = ags_clib_fopen("ac2game.dta", "rb");
        if (!f) {
            fprintf(stderr, "SANITY CHECK FAILED: could not open ac2game.dta via ags_clib_fopen\n");
            return 1;
        }
        {
            long expect_off = ags_clib_offset("ac2game.dta");
            long expect_len = ags_clib_file_size("ac2game.dta");
            long actual_pos = ftell(f);
            long reported_size = ags_clib_last_opened_size();
            printf("\nSanity check: ac2game.dta opened at ftell=%ld (expected %ld), "
                   "last_opened_size=%ld (expected %ld) -- %s\n",
                   actual_pos, expect_off, reported_size, expect_len,
                   (actual_pos == expect_off && reported_size == expect_len) ? "OK" : "MISMATCH");
            if (actual_pos != expect_off || reported_size != expect_len) {
                fclose(f);
                return 1;
            }
        }
        fclose(f);
    }

    return 0;
}

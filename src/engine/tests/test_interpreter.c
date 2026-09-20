/* M4 test (see src/PLAN.md): "Run the interpreter on nothing." Walks
 * load_game_file's own byte-consumption sequence (M2/M3) up through the
 * compiled global script's own position, then -- unlike M3's
 * ags_skip_compiled_script, which only skips past it -- actually
 * decodes it via ags_cc_read_script (M4), creates a running instance
 * (ags_cc_create_instance), and calls its "game_start" export
 * (ags_cc_call_instance), exactly as the real engine's own
 * initialize_start_and_play()/RunGameLoop() would as it enters the
 * title/first room. Every native (imported) function the script
 * actually calls gets logged via AGS_STUB the first time it happens
 * (ags/stub.h) -- there's no game engine behind them yet, only M4's
 * own interpreter and instance/import machinery.
 *
 * Usage:
 *   test_interpreter.exe <path to rb.exe>
 */
#include "ags/clib.h"
#include "ags/loader.h"
#include "ags/script_loader.h"
#include "ags/interp.h"
#include "ags/stub.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char **argv)
{
    struct GameSetupStructBase game;
    struct AgsGameFileHeader hdr;
    enum AgsScriptLoadError script_err;
    struct ccScript *scri;
    struct ccInstance *inst;
    FILE *f;
    int rc, i;
    long retval = -12345;
    int run_rc;

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

    /* M3 stopped here and called ags_skip_compiled_script -- M4 reads
     * the same bytes for real instead. */
    scri = ags_cc_read_script(f, &script_err);
    fclose(f);
    if (!scri) {
        fprintf(stderr, "ags_cc_read_script failed: %d\n", (int)script_err);
        return 1;
    }

    printf("script: globaldatasize=%d codesize=%d stringssize=%d "
           "numfixups=%d numimports=%d numexports=%d\n",
           scri->globaldatasize, scri->codesize, scri->stringssize,
           scri->numfixups, scri->numimports, scri->numexports);

    printf("\nexports:\n");
    for (i = 0; i < scri->numexports; i++) {
        printf("  [%d] %s (addr=0x%08lX)\n", i, scri->exports[i],
               (unsigned long)scri->export_addr[i]);
    }

    printf("\nimports (non-NULL only):\n");
    for (i = 0; i < scri->numimports; i++) {
        if (scri->imports[i]) {
            printf("  [%d] %s\n", i, (const char *)scri->imports[i]);
        }
    }

    inst = ags_cc_create_instance(scri);
    if (!inst) {
        fprintf(stderr, "FAIL: ags_cc_create_instance returned NULL\n");
        return 1;
    }
    printf("\ninstance created OK (codesize=%d globaldatasize=%d)\n",
           inst->codesize, inst->globaldatasize);

    printf("\nrunning game_start()...\n");
    printf("--- native calls the script makes (via AGS_STUB) ---\n");
    run_rc = ags_cc_call_instance(inst, "game_start", 0, NULL, &retval);
    printf("--- end of native calls ---\n");

    printf("\nags_cc_call_instance(\"game_start\") returned %d, AX=%ld\n",
           run_rc, retval);

    ags_stub_dump_summary();

    /* M4's own acceptance check (src/PLAN.md): the interpreter must
     * actually run real Rob Blanc 1 bytecode to completion (a normal
     * RET back through the sentinel-0 return address) rather than
     * hitting a decode error -- AGS_CC_RUN_OK is the only acceptable
     * outcome; AGS_CC_RUN_FUNC_NOT_FOUND would mean game_start isn't
     * really exported (already directly disproved above), and any
     * other negative code means the 38-opcode VM (ags/interp.h)
     * disagrees with what this build's own compiler actually emitted. */
    if (run_rc != AGS_CC_RUN_OK) {
        fprintf(stderr, "\nFAIL: game_start() did not run to completion "
                         "(AgsCcRunError %d)\n", run_rc);
        return 1;
    }

    printf("\nM4 ACCEPTANCE CHECK OK: game_start() ran to completion "
           "through the real 38-opcode interpreter\n");
    return 0;
}
END_OF_MAIN() /* see test_gamesetup.c's own comment on this -- ags/loader.h
               * transitively includes allegro.h, whose "magic main" macro
               * renames main() even though this program never calls Allegro. */

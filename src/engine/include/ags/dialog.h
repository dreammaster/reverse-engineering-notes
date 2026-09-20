/* ags/dialog.h -- DialogTopic and WordsDictionary. Direct port of
 * apply_structs.py's SAFE_DECLS.
 */
#ifndef AGS_DIALOG_H
#define AGS_DIALOG_H

#include "ags/types.h"

/* DialogTopic -- dialog[]'s element type, a genuinely NEW struct with
 * NO 2011 ancestor declaration to lean on at all -- every field
 * confirmed via independent access sites rather than a still-existing
 * reference declaration. Byte-code interpreted by run_dialog_script's
 * own DCMD_* opcode set (13/16 declared opcodes present; SETGLOBALINT/
 * GIVESCORE/GOTOPREVIOUS/LOSEINV confirmed absent). */
struct DialogTopic {
    char optionnames[15][0x46];   /* +0x000..0x41A, MAXTOPICOPTIONS=15, 70 bytes/option */
    char _pad_align2[2];            /* +0x41A..0x41C, compiler alignment padding */
    int optionflags[15];              /* +0x41C..0x458 */
    unsigned char *optionscripts;       /* +0x458, DCMD_* bytecode */
    short entrypoints[15];                /* +0x45C..0x47A */
    short startupentrypoint;                /* +0x47A */
    short codesize;                           /* +0x47C */
    char _pad_align[2];                         /* +0x47E..0x480, compiler alignment padding */
    int numoptions;                               /* +0x480..0x484, ends exactly at the struct's own confirmed total */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct DialogTopic) == 0x484, "DialogTopic must be 0x484 bytes");

/* WordsDictionary -- the text-parser word list. This build flattens
 * 2011's dynamic char**word/short*wordnum double-allocation into ONE
 * fixed 1500-word blob behind a single presence-flag pointer (the
 * same idiom as ccScript/compiled_script). */
struct WordsDictionary {
    int num_words;         /* +0x0000 */
    char word[1500][30];      /* +0x0004..0xAFCC */
    short wordnum[1500];        /* +0xAFCC..0xBB84 */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct WordsDictionary) == 0xBB84, "WordsDictionary must be 0xBB84 bytes");

#endif /* AGS_DIALOG_H */

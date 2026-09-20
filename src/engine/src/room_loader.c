/* ags/room_loader.h's own implementation. See that header's file-level
 * comment for overall scope. Ported by reading load_room (Common/
 * acroom.h:1977, disassembly at rob_blanc_1.asm load_room proc,
 * lines 8136-8713) and load_main_block (acroom.h:1605, disassembly
 * lines 6674-8097) end to end for their REAL, version>=9 control flow
 * -- matches.json's own entries for both cite the right evidence but
 * don't spell out full read ORDER, which matters for a byte-exact
 * port. Every field/offset cited below by "+0xNNN" matches ags/room.h
 * / reversing/notes/struct-layout-drift.md exactly.
 */
#include "ags/room_loader.h"
#include "ags/clib.h"
#include "ags/script_loader.h"
#include "ags/lzw.h"

#include <stdlib.h>
#include <string.h>

/* AGS's classic text-obfuscation key (Common/cscommon.cpp's
 * decrypt_text, already matched elsewhere in this project). Reused
 * here for two DIFFERENT ADD-cipher uses this build's own disassembly
 * shows: the BLOCKTYPE_SCRIPT text-script blob (`scripts[i] +=
 * key[i%11]`, load_room's own type-2 handler) and the room password
 * field (`password[i] += key[i]`, load_main_block's own tail, for
 * wasversion>=9 -- the version<9 path instead does `password[i]+=60`,
 * moot here since wasversion<9 is already treated as unsupported). */
static const char AVIS_DURGAN[] = "Avis Durgan"; /* 11 chars + NUL */

/* --- load_lzw (Common/acroom.h:1339), adapted: takes/returns `block`
 * directly instead of writing into a "recalced" global (load_lzw's
 * own OUTPUT was always a byval BITMAP* parameter plus that global
 * purely as a way to hand the new bitmap back to 2002-era callers --
 * our own C call sites can just use the return value directly, a
 * clean simplification with no behavioral difference). Drops the
 * (dead on this x86 target) ALLEGRO_BIG_ENDIAN byte-swap branch.
 * Returns NULL on any read/allocation failure (old_bmm is NOT
 * destroyed in that case, matching source's own destroy-only-once-a-
 * replacement-exists-and-succeeds ordering closely enough for this
 * milestone's purposes). */
static block load_room_lzw(FILE *f, block old_bmm, unsigned char pal[256][4], int bytes_per_pixel)
{
    int maxsize32;
    int uncompsiz32;
    long maxsize, uncompsiz;
    unsigned char *membuffer;
    long *loptr;
    block bmm;
    long arin;

    if (fread(pal, 1, 256 * 4, f) != 256 * 4) {
        return NULL;
    }
    if (fread(&maxsize32, sizeof(maxsize32), 1, f) != 1) {
        return NULL;
    }
    if (fread(&uncompsiz32, sizeof(uncompsiz32), 1, f) != 1) {
        return NULL;
    }
    maxsize = maxsize32;
    uncompsiz = (long)uncompsiz32 + ftell(f);

    membuffer = ags_lzw_expand_to_mem(f, maxsize);
    if (!membuffer) {
        return NULL;
    }

    loptr = (long *)membuffer;
    membuffer += 8;

    if (bytes_per_pixel < 1) {
        bytes_per_pixel = 1;
    }
    bmm = create_bitmap_ex(bytes_per_pixel * 8, (int)(loptr[0] / bytes_per_pixel), (int)loptr[1]);
    if (!bmm) {
        free(membuffer - 8);
        return NULL;
    }

    for (arin = 0; arin < loptr[1]; arin++) {
        memcpy(bmm->line[arin], &membuffer[arin * loptr[0]], (size_t)loptr[0]);
    }

    free(membuffer - 8);

    if (old_bmm) {
        destroy_bitmap(old_bmm);
    }

    if (ftell(f) != uncompsiz) {
        fseek(f, uncompsiz, SEEK_SET);
    }

    return bmm;
}

/* --- loadcompressed_allegro (Common/acroom.h:1439), adapted the same
 * way (return value instead of a BITMAP** out-param); the `ooo`
 * parameter is dropped entirely -- source's own function body never
 * reads it, it only ever gets threaded through as an unused return
 * value for the NEXT call's own unused argument (see room_loader.c's
 * file-level comment; a genuine no-op in the original, not a
 * simplification that changes behavior). */
static block load_room_mask(FILE *f, block old_bmm, unsigned char pal[256][4])
{
    short widd, hitt;
    block bim;
    int ii;

    (void)pal; /* the trailing 768-byte skipped palette block never gets decoded into `pal` either in source -- it's read purely to advance the file position past it */

    if (fread(&widd, sizeof(widd), 1, f) != 1) {
        return NULL;
    }
    if (fread(&hitt, sizeof(hitt), 1, f) != 1) {
        return NULL;
    }

    bim = create_bitmap_ex(8, widd, hitt);
    if (!bim) {
        return NULL;
    }

    for (ii = 0; ii < hitt; ii++) {
        if (ags_cunpackbitl(bim->line[ii], widd, f) != 0) {
            destroy_bitmap(bim);
            return NULL;
        }
    }

    fseek(f, 768, SEEK_CUR); /* skip the trailing per-mask palette block */

    if (old_bmm) {
        destroy_bitmap(old_bmm);
    }

    return bim;
}

/* fgetstring (Common/cscommon.cpp:194-196) as load_main_block's own
 * message-decrypt loop actually calls it: byte-by-byte until NUL, no
 * length prefix. CONFIRMED this build always uses the plain,
 * unencrypted variant regardless of room version -- unlike 2011's
 * read_string_decrypt(version>=22)/fgetstring_limit(version<22) gate,
 * this build's disassembly shows one unconditional fgetstring call.
 * Source's own version has NO explicit bound at all (an unbounded
 * write into a ~900-byte stack buffer in the original -- a real, if
 * never-triggered-by-legitimate-data, overflow risk already noted
 * elsewhere in this project's own findings); this port adds a bound,
 * silently truncating rather than overflowing, matching the same
 * "safety the original disassembly doesn't have" precedent as this
 * file's other capacity checks. */
static int room_fgetstring(FILE *f, char *buf, size_t bufsize)
{
    size_t idx = 0;
    int c;

    for (;;) {
        c = fgetc(f);
        if (c == EOF) {
            return -1;
        }
        if (idx < bufsize - 1) {
            buf[idx] = (char)c;
        }
        if (c == 0) {
            break;
        }
        idx++;
    }
    buf[(idx < bufsize - 1) ? idx : bufsize - 1] = '\0';
    return 0;
}

/* load_script_configuration (Engine/scrptrt.cpp:40) -- a direct port.
 * Skips past the room's obsolete-script-variable-name table: a
 * version check (must be 1), a count, then `count` length-prefixed
 * names skipped via fseek rather than actually read. */
static enum AgsRoomLoadError load_script_configuration(FILE *f)
{
    int version;
    int numvarnames;
    int i;

    if (fread(&version, sizeof(version), 1, f) != 1) {
        return AGS_ROOM_LOAD_READ_ERROR;
    }
    if (version != 1) {
        return AGS_ROOM_LOAD_BAD_SCRIPT; /* "ScriptEdit: invliad config version" (typo preserved from source) */
    }

    if (fread(&numvarnames, sizeof(numvarnames), 1, f) != 1) {
        return AGS_ROOM_LOAD_READ_ERROR;
    }

    for (i = 0; i < numvarnames; i++) {
        int lenoft = fgetc(f);
        if (lenoft == EOF) {
            return AGS_ROOM_LOAD_READ_ERROR;
        }
        if (fseek(f, lenoft, SEEK_CUR) != 0) {
            return AGS_ROOM_LOAD_READ_ERROR;
        }
    }
    return AGS_ROOM_LOAD_OK;
}

/* load_graphical_scripts (Engine/scrptrt.cpp, no 2011 counterpart --
 * see reversing/notes/struct-layout-drift.md's "graph-script"/
 * "Animations" findings) -- SKIPPED rather than extracted to temp
 * files, per this file's own header comment. Loops: a 4-byte marker
 * (-1 or a stream EOF/error condition ends the loop), then a 4-byte
 * size, then that many bytes of payload. Route_script_link's own
 * anti-piracy check (source's leading call) is not ported -- it
 * never reads from the room file at all, so skipping it can't affect
 * byte alignment. */
static enum AgsRoomLoadError skip_graphical_scripts(FILE *f)
{
    for (;;) {
        int marker;
        int size;

        if (fread(&marker, sizeof(marker), 1, f) != 1) {
            return AGS_ROOM_LOAD_READ_ERROR;
        }
        if (marker == -1 || feof(f) || ferror(f)) {
            break;
        }

        if (fread(&size, sizeof(size), 1, f) != 1) {
            return AGS_ROOM_LOAD_READ_ERROR;
        }
        if (size < 0) {
            return AGS_ROOM_LOAD_READ_ERROR;
        }
        if (fseek(f, size, SEEK_CUR) != 0) {
            return AGS_ROOM_LOAD_READ_ERROR;
        }
    }
    return AGS_ROOM_LOAD_OK;
}

/* load_main_block (Common/acroom.h:1605), version>=9 real path only
 * -- see room_loader.h's own file-level comment for why version<9 is
 * treated as unsupported rather than ported. Field-by-field order
 * below matches the disassembly's own real read sequence exactly
 * (NOT always 2011's declared field order -- e.g. top/bottom/left/
 * right are read in that order into differently-ordered fields, and
 * this build reads the hscond/objcond/misccond/hswalkto/hotspotnames/
 * numwalkareas/wallpoints block BEFORE top/bottom/left/right/numsprs/
 * sprs[], where 2011 has no equivalent ordering at all since those
 * first five fields don't exist in its own roomstruct). */
static enum AgsRoomLoadError load_main_block_real(struct RoomStruct *rst, FILE *f, int wasversion)
{
    int i;
    int tmp32;
    int numread; /* NUMREAD local -- walk-area-count override, version>=14 only */

    memset(rst->objbaseline, 0xFF, sizeof(rst->objbaseline));
    memset(rst->hswalkto, 0, sizeof(rst->hswalkto));
    memset(rst->walk_area_zoom, 0, sizeof(rst->walk_area_zoom));
    memset(rst->walk_area_light, 0, sizeof(rst->walk_area_light));

    for (i = 0; i < 20; i++) {
        if (i == 0) {
            strcpy(rst->hotspotnames[i], "No hotspot");
        } else {
            sprintf(rst->hotspotnames[i], "Hotspot %d", i);
        }
    }

    memset(rst->hscond, 0, sizeof(rst->hscond));
    memset(rst->objcond, 0, sizeof(rst->objcond));
    memset(&rst->misccond, 0, sizeof(rst->misccond));

    if (wasversion >= 12) {
        if (fread(&tmp32, sizeof(tmp32), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
        rst->bytes_per_pixel = tmp32;
    } else {
        rst->bytes_per_pixel = 1;
    }
    if (rst->bytes_per_pixel < 1) {
        rst->bytes_per_pixel = 1;
    }

    if (fread(&rst->numobj, sizeof(rst->numobj), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (rst->numobj < 0 || rst->numobj > 15) return AGS_ROOM_LOAD_CAPACITY_EXCEEDED;
    if (fread(rst->objyval, sizeof(short), (size_t)rst->numobj, f) != (size_t)rst->numobj) return AGS_ROOM_LOAD_READ_ERROR;

    if (wasversion < 9) {
        /* CONFIRMED dead code for any real Rob Blanc 1 room file --
         * see room_loader.h's own file-level comment. */
        return AGS_ROOM_LOAD_UNSUPPORTED_VERSION;
    }

    if (fread(&rst->numhotspots, sizeof(rst->numhotspots), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (fread(rst->hscond, sizeof(rst->hscond), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (fread(rst->objcond, sizeof(rst->objcond), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (fread(&rst->misccond, sizeof(rst->misccond), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (fread(rst->hswalkto, sizeof(rst->hswalkto), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (fread(rst->hotspotnames, sizeof(rst->hotspotnames), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;

    if (fread(&rst->numwalkareas, sizeof(rst->numwalkareas), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (rst->numwalkareas < 0 || rst->numwalkareas > 15) return AGS_ROOM_LOAD_CAPACITY_EXCEEDED;
    if (fread(rst->wallpoints, sizeof(struct PolyPoints), (size_t)rst->numwalkareas, f) != (size_t)rst->numwalkareas) return AGS_ROOM_LOAD_READ_ERROR;

    /* Read in this order (matches the disassembly literally), stored
     * into differently-ordered fields. */
    if (fread(&rst->top, sizeof(rst->top), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (fread(&rst->bottom, sizeof(rst->bottom), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (fread(&rst->left, sizeof(rst->left), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (fread(&rst->right, sizeof(rst->right), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;

    memset(rst->sprs, 0, sizeof(rst->sprs));
    if (fread(&rst->numsprs, sizeof(rst->numsprs), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (rst->numsprs < 0 || rst->numsprs > 10) return AGS_ROOM_LOAD_CAPACITY_EXCEEDED;
    if (fread(rst->sprs, sizeof(struct sprstruc), (size_t)rst->numsprs, f) != (size_t)rst->numsprs) return AGS_ROOM_LOAD_READ_ERROR;

    /* wasversion>=9 always true here -- objbaseline/width/height */
    if (fread(rst->objbaseline, sizeof(int), (size_t)rst->numsprs, f) != (size_t)rst->numsprs) return AGS_ROOM_LOAD_READ_ERROR;
    if (fread(&rst->width, sizeof(rst->width), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (fread(&rst->height, sizeof(rst->height), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;

    if (wasversion >= 11) {
        if (fread(&rst->resolution, sizeof(rst->resolution), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    }

    numread = 15;
    if (wasversion >= 14) {
        if (fread(&tmp32, sizeof(tmp32), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
        numread = tmp32;
    }
    if (numread > 16) {
        return AGS_ROOM_LOAD_TOO_MANY_WALKAREAS;
    }

    if (wasversion >= 10) {
        if (fread(rst->walk_area_zoom, sizeof(short), (size_t)numread, f) != (size_t)numread) return AGS_ROOM_LOAD_READ_ERROR;
    }
    if (wasversion >= 13) {
        if (fread(rst->walk_area_light, sizeof(short), (size_t)numread, f) != (size_t)numread) return AGS_ROOM_LOAD_READ_ERROR;
    }

    if (fread(rst->password, sizeof(rst->password), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (fread(rst->options, sizeof(rst->options), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (fread(&rst->nummes, sizeof(rst->nummes), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (rst->nummes < 0 || rst->nummes > 100) return AGS_ROOM_LOAD_CAPACITY_EXCEEDED;

    /* wasversion>=3 always true here */
    if (fread(rst->msgi, sizeof(rst->msgi[0]), (size_t)rst->nummes, f) != (size_t)rst->nummes) return AGS_ROOM_LOAD_READ_ERROR;

    for (i = 0; i < rst->nummes; i++) {
        char buf[3000];
        size_t len;

        if (room_fgetstring(f, buf, sizeof(buf)) != 0) return AGS_ROOM_LOAD_READ_ERROR;

        len = strlen(buf);
        rst->message[i] = (char *)malloc(len + 2);
        if (!rst->message[i]) return AGS_ROOM_LOAD_OUT_OF_MEMORY;
        strcpy(rst->message[i], buf);

        if (len > 0 && (unsigned char)buf[len - 1] == 200) {
            rst->message[i][len - 1] = '\0';
            rst->msgi[i][1] |= 1; /* MSG_DISPLAYNEXT -- msgi[f][1] is the flags byte, confirmed this round via this exact write */
        }
    }

    if (wasversion >= 6) {
        if (fread(&rst->numanims, sizeof(rst->numanims), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
        if (rst->numanims > 0) {
            if (rst->numanims > 10) return AGS_ROOM_LOAD_CAPACITY_EXCEEDED;
            if (fread(rst->anims, sizeof(struct FullAnimation), (size_t)rst->numanims, f) != (size_t)rst->numanims) return AGS_ROOM_LOAD_READ_ERROR;
        }
    } else {
        rst->numanims = 0;
        memset(rst->anims, 0, sizeof(rst->anims));
    }

    if (wasversion >= 4) {
        enum AgsRoomLoadError rc = load_script_configuration(f);
        if (rc != AGS_ROOM_LOAD_OK) return rc;
        rc = skip_graphical_scripts(f);
        if (rc != AGS_ROOM_LOAD_OK) return rc;
    }

    memset(rst->shadinginfo, 0, sizeof(rst->shadinginfo));
    if (wasversion >= 8) {
        if (fread(rst->shadinginfo, sizeof(short), 16, f) != 16) return AGS_ROOM_LOAD_READ_ERROR;
    }

    /* wasversion>=5 always true here: real LZW background decode */
    rst->ebscene[0] = load_room_lzw(f, rst->ebscene[0], rst->pal, rst->bytes_per_pixel);
    if (!rst->ebscene[0]) return AGS_ROOM_LOAD_OUT_OF_MEMORY;

    if (rst->ebscene[0]->w > 320 && wasversion < 11) {
        rst->resolution = 2;
    }

    /* wasversion>=8 always true here: regions mask (loaded, never
     * read back by this build's engine code -- see room.h's own
     * comment on RoomStruct.regions). */
    rst->regions = load_room_mask(f, rst->regions, rst->pal);
    if (!rst->regions) return AGS_ROOM_LOAD_OUT_OF_MEMORY;

    rst->walls = load_room_mask(f, rst->walls, rst->pal);
    if (!rst->walls) return AGS_ROOM_LOAD_OUT_OF_MEMORY;
    rst->object = load_room_mask(f, rst->object, rst->pal);
    if (!rst->object) return AGS_ROOM_LOAD_OUT_OF_MEMORY;
    rst->lookat = load_room_mask(f, rst->lookat, rst->pal);
    if (!rst->lookat) return AGS_ROOM_LOAD_OUT_OF_MEMORY;

    /* wasversion>=9 always true here: ADD-cipher against "Avis
     * Durgan" (the version<9 path, "password[i]+=60", is unreachable
     * given the unsupported-version return far above). */
    for (i = 0; i < 11; i++) {
        rst->password[i] = (char)(rst->password[i] + AVIS_DURGAN[i]);
    }

    return AGS_ROOM_LOAD_OK;
}

/* load_room's own BLOCKTYPE_SCRIPT (type 2) handler: a length-
 * prefixed text-script blob, ADD-ciphered against "Avis Durgan"
 * (modulo 11, unlike load_main_block's own fixed-11-byte password
 * cipher -- this blob can be far longer than 11 bytes). */
static enum AgsRoomLoadError load_text_script_block(struct RoomStruct *rst, FILE *f)
{
    int size;
    int i;

    if (fread(&size, sizeof(size), 1, f) != 1) return AGS_ROOM_LOAD_READ_ERROR;
    if (size < 0) return AGS_ROOM_LOAD_READ_ERROR;

    rst->scripts = (char *)malloc((size_t)size + 5);
    if (!rst->scripts) return AGS_ROOM_LOAD_OUT_OF_MEMORY;

    if (fread(rst->scripts, 1, (size_t)size, f) != (size_t)size) return AGS_ROOM_LOAD_READ_ERROR;
    rst->scripts[size] = '\0';

    for (i = 0; i < size; i++) {
        rst->scripts[i] = (char)(rst->scripts[i] + AVIS_DURGAN[i % 11]);
    }

    return AGS_ROOM_LOAD_OK;
}

enum AgsRoomLoadError ags_load_room(const char *filename, struct RoomStruct *rst)
{
    FILE *f;
    short wasversion;

    /* --- pre-load cleanup (load_room's own opening sequence,
     * acroom.h:1982-2059) -- only matters when *rst held a previously
     * loaded room; harmless no-ops on a freshly zeroed struct. */
    {
        int i;
        for (i = 0; i < rst->nummes; i++) {
            if (rst->message[i]) {
                free(rst->message[i]);
                rst->message[i] = NULL;
            }
        }
    }
    if (rst->scripts) {
        free(rst->scripts);
        rst->scripts = NULL;
    }
    if (rst->compiled_script) {
        ags_cc_free_script(rst->compiled_script);
        rst->compiled_script = NULL;
    }
    {
        int c;
        for (c = 1; c < rst->num_bscenes && c < 5; c++) {
            if (rst->ebscene[c]) {
                destroy_bitmap(rst->ebscene[c]);
                rst->ebscene[c] = NULL;
            }
        }
    }
    rst->num_bscenes = 1;
    rst->bscene_anim_speed = 5;
    memset(rst->objectnames, 0, sizeof(rst->objectnames));

    f = ags_clib_fopen(filename, "rb");
    if (!f) {
        return AGS_ROOM_LOAD_OPEN_ERROR;
    }

    if (fread(&wasversion, sizeof(wasversion), 1, f) != 1) {
        fclose(f);
        return AGS_ROOM_LOAD_READ_ERROR;
    }
    rst->wasversion = wasversion;
    if (wasversion < 2 || wasversion > 14) {
        fclose(f);
        return AGS_ROOM_LOAD_BAD_VERSION;
    }

    if (wasversion == 5) {
        fseek(f, 5, SEEK_CUR);
    }

    if (wasversion < 6) {
        /* Old, pre-block-container room format -- calls
         * load_main_block directly with no type-tagged wrapper at
         * all. CONFIRMED dead code for any real Rob Blanc 1 room
         * file -- see room_loader.h's own file-level comment. */
        fclose(f);
        return AGS_ROOM_LOAD_UNSUPPORTED_VERSION;
    }

    for (;;) {
        int blocktype;
        int block_size;
        long block_end;
        enum AgsRoomLoadError rc = AGS_ROOM_LOAD_OK;

        blocktype = fgetc(f);
        if (blocktype == 0xFF || blocktype == EOF) {
            break;
        }

        if (fread(&block_size, sizeof(block_size), 1, f) != 1) {
            fclose(f);
            return AGS_ROOM_LOAD_READ_ERROR;
        }
        block_end = (long)block_size + ftell(f);

        switch (blocktype) {
        case 1: /* BLOCKTYPE_MAIN */
            rc = load_main_block_real(rst, f, wasversion);
            break;
        case 2: /* BLOCKTYPE_SCRIPT */
            rc = load_text_script_block(rst, f);
            break;
        case 3: /* BLOCKTYPE_COMPSCRIPT */
        case 4: /* BLOCKTYPE_COMPSCRIPT2 */
            rc = AGS_ROOM_LOAD_OLD_FORMAT;
            break;
        case 5: { /* BLOCKTYPE_OBJECTNAMES */
            int chk = fgetc(f);
            if (chk != rst->numsprs) {
                rc = AGS_ROOM_LOAD_INCONSISTENT_OBJECTNAMES;
                break;
            }
            if (fread(rst->objectnames, 30, (size_t)rst->numsprs, f) != (size_t)rst->numsprs) {
                rc = AGS_ROOM_LOAD_READ_ERROR;
            }
            break;
        }
        case 6: { /* BLOCKTYPE_ANIMBKGRND */
            int nb = fgetc(f);
            int bas = fgetc(f);
            int ct;
            rst->num_bscenes = nb;
            rst->bscene_anim_speed = bas;
            for (ct = 1; ct < rst->num_bscenes && ct < 5; ct++) {
                rst->ebscene[ct] = load_room_lzw(f, rst->ebscene[ct], rst->pal, rst->bytes_per_pixel);
                if (!rst->ebscene[ct]) {
                    rc = AGS_ROOM_LOAD_OUT_OF_MEMORY;
                    break;
                }
            }
            break;
        }
        case 7: { /* BLOCKTYPE_COMPSCRIPT3 */
            enum AgsScriptLoadError serr;
            struct ccScript *scri = ags_cc_read_script(f, &serr);
            if (!scri) {
                rc = AGS_ROOM_LOAD_BAD_SCRIPT;
                break;
            }
            rst->compiled_script = scri;
            break;
        }
        default:
            rc = AGS_ROOM_LOAD_UNKNOWN_BLOCK;
            break;
        }

        if (rc != AGS_ROOM_LOAD_OK) {
            fclose(f);
            return rc;
        }

        if (ftell(f) != block_end) {
            fseek(f, block_end, SEEK_SET);
        }
    }

    fclose(f);
    return AGS_ROOM_LOAD_OK;
}

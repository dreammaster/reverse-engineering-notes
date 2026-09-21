/* ags/sprite_loader.h's own implementation. See that header's
 * file-level comment for full scope/evidence.
 */
#include "ags/sprite_loader.h"
#include "ags/clib.h"

#include <stdlib.h>
#include <string.h>

/* get_new_size_for_sprite(int,int,int,int&,int&) (Engine/AC.CPP:25808,
 * already matched in this project) -- a direct port. `mult_x`/
 * `mult_y` are this build's own confirmed SEPARATE horizontal/
 * vertical resolution multipliers (a drift from 2011's single shared
 * one) -- hardcoded to 1 here since every real room this project has
 * decoded so far (M5/M6, six real rooms cross-checked) reports
 * resolution==1 uniformly; revisit if a game-wide forced-640x400
 * option is ever found to apply to Rob Blanc 1. */
static void get_new_size_for_sprite(int spritenum, int ww, int hh,
                                     const struct GameSetupStructBase *game,
                                     int *out_w, int *out_h)
{
    int mult_x = 1;
    int mult_y = 1;
    int neww = ww * mult_x;
    int newh = hh * mult_y;

    if (game->spriteflags[spritenum] & 1) { /* SPF_640x400, confirmed bit 0 */
        if (mult_x == 2) {
            neww = ww;
        } else {
            neww = (ww / 2) * mult_x;
            if (neww < 1) neww = 1;
        }
        if (mult_y == 2) {
            newh = hh;
        } else {
            newh = (hh / 2) * mult_y;
            if (newh < 1) newh = 1;
        }
    }

    *out_w = neww;
    *out_h = newh;
}

void ags_spriteset_free(struct AgsSpriteSet *set)
{
    if (!set) {
        return;
    }
    free(set->cache.offsets);
    free(set->cache.images);
    free(set->cache.mrulist);
    free(set->cache.mrubacklink);
    free(set->width);
    free(set->height);
    if (set->cache.ff) {
        fclose((FILE *)set->cache.ff);
    }
    memset(set, 0, sizeof(*set));
}

enum AgsSpriteLoadError ags_spriteset_init(struct AgsSpriteSet *set, const char *filename,
                                            const struct GameSetupStructBase *game)
{
    FILE *f;
    short vers;
    char sig[13];
    short numspr;
    long elements;
    int vv;

    memset(set, 0, sizeof(*set));

    f = ags_clib_fopen(filename, "rb");
    if (!f) {
        return AGS_SPRITE_LOAD_OPEN_ERROR;
    }

    if (fread(&vers, sizeof(vers), 1, f) != 1) {
        fclose(f);
        return AGS_SPRITE_LOAD_READ_ERROR;
    }
    if (fread(sig, 13, 1, f) != 1) {
        fclose(f);
        return AGS_SPRITE_LOAD_READ_ERROR;
    }
    /* the 256-entry RGB palette -- skipped UNCONDITIONALLY here,
     * regardless of version (see the file-level comment). */
    fseek(f, 0x300, SEEK_CUR);

    if (vers > 4) {
        fclose(f);
        return AGS_SPRITE_LOAD_BAD_VERSION;
    }

    if (fread(&numspr, sizeof(numspr), 1, f) != 1) {
        fclose(f);
        return AGS_SPRITE_LOAD_READ_ERROR;
    }
    if (vers < 4) {
        numspr = 200;
    }

    elements = (long)numspr + 1;

    set->cache.elements = elements;
    set->cache.offsets = (long *)calloc((size_t)elements, sizeof(long));
    set->cache.images = (block *)calloc((size_t)elements, sizeof(block));
    set->cache.mrulist = (int *)calloc((size_t)elements, sizeof(int));
    set->cache.mrubacklink = (int *)calloc((size_t)elements, sizeof(int));
    set->width = (int *)calloc((size_t)elements, sizeof(int));
    set->height = (int *)calloc((size_t)elements, sizeof(int));
    if (!set->cache.offsets || !set->cache.images || !set->cache.mrulist ||
        !set->cache.mrubacklink || !set->width || !set->height) {
        ags_spriteset_free(set);
        fclose(f);
        return AGS_SPRITE_LOAD_OUT_OF_MEMORY;
    }
    set->cache.liststart = -1;
    set->cache.listend = -1;
    set->cache.lastLoad = -2;
    set->cache.maxCacheSize = 5000000; /* this build's own confirmed default (Step 0's own SpriteCache comment) */
    set->cache.ff = f;

    for (vv = 0; vv <= (int)numspr && vv < elements; vv++) {
        short coldep;

        set->cache.offsets[vv] = ftell(f);

        if (fread(&coldep, sizeof(coldep), 1, f) != 1) {
            break;
        }

        if (coldep == 0) {
            set->cache.images[vv] = NULL;
            set->cache.offsets[vv] = 0;
            if (feof(f)) {
                break;
            }
            continue;
        }

        if (feof(f)) {
            break;
        }

        {
            short wdd, htt;
            int neww, newh;
            long datasize;

            if (fread(&wdd, sizeof(wdd), 1, f) != 1) break;
            if (fread(&htt, sizeof(htt), 1, f) != 1) break;

            get_new_size_for_sprite(vv, wdd, htt, game, &neww, &newh);
            set->width[vv] = neww;
            set->height[vv] = newh;

            datasize = (long)wdd * (long)coldep * (long)htt;
            fseek(f, datasize, SEEK_CUR);
        }
    }

    return AGS_SPRITE_LOAD_OK;
}

block ags_spriteset_load(struct AgsSpriteSet *set, int index)
{
    FILE *f = (FILE *)set->cache.ff;
    short coldep;
    short wdd, htt;
    block bmp;
    int hh;

    if (index < 0 || index >= set->cache.elements) {
        return NULL;
    }
    if (set->cache.offsets[index] == 0 && index != 0) {
        return NULL; /* empty slot */
    }

    if (index - 1 != set->cache.lastLoad) {
        fseek(f, set->cache.offsets[index], SEEK_SET);
    }

    if (fread(&coldep, sizeof(coldep), 1, f) != 1) {
        return NULL;
    }
    if (coldep == 0) {
        set->cache.lastLoad = index;
        return NULL;
    }

    if (fread(&wdd, sizeof(wdd), 1, f) != 1) return NULL;
    if (fread(&htt, sizeof(htt), 1, f) != 1) return NULL;

    bmp = create_bitmap_ex(coldep * 8, wdd, htt);
    if (!bmp) {
        return NULL;
    }

    for (hh = 0; hh < htt; hh++) {
        if (fread(bmp->line[hh], (size_t)coldep, (size_t)wdd, f) != (size_t)wdd) {
            destroy_bitmap(bmp);
            return NULL;
        }
    }

    set->cache.lastLoad = index;
    set->cache.images[index] = bmp;
    return bmp;
}

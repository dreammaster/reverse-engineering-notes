/* ags/sprite_loader.h -- M7 ("Meet the room's people", see
 * src/PLAN.md): a real port of SpriteCache::initFile/loadSprite
 * (Common/sprcache.cpp:631/358), narrowed to this build's own
 * genuinely much simpler real on-disk format -- read from
 * SpriteCache::initFile's actual disassembly (rob_blanc_1.asm, proc
 * bounds 4148-4432), not assumed from the 2011 reference:
 *
 *   - Only sprite-file versions 0-4 are ACCEPTED (source's own
 *     version 5/6 -- compressed sprites, a spriteFileID check, an
 *     optional "SPRINDEX" side-index file -- are all rejected
 *     outright by a plain `if (vers>4) return -1`, before even the
 *     version's own meaning is checked further). Consistent with
 *     SpriteCache's own struct-layout-drift.md findings: this build
 *     never has `spritesAreCompressed` true, never has `sizes[]`/
 *     `flags[]` arrays, and (newly confirmed by this milestone) never
 *     has a sprite-index side-file either.
 *   - The 256-entry RGB palette block is skipped via one UNCONDITIONAL
 *     `fseek(768, CUR)` regardless of version -- not the version-
 *     gated `if (vers<5) fseek(...)` 2011's source shows.
 *   - Per-sprite: an empty slot (coldep==0) is zeroed WITHOUT 2011's
 *     "make it a blue cup" fallback (copying sprite 0's own
 *     width/height/offset in so a later lookup can't crash) -- this
 *     build leaves offsets[vv]/images[vv] at a plain 0, confirmed
 *     absent by direct comparison against the disassembly.
 *   - A real sprite's own width/height ARE passed through
 *     get_new_size_for_sprite (Engine/AC.CPP:25808, already matched)
 *     for the width[]/height[] parallel arrays this module keeps
 *     (mirroring the real engine's own SEPARATE spritewidth[]/
 *     spriteheight[] globals -- never part of SpriteCache itself even
 *     in the original) -- but the on-disk PIXEL DATA is always
 *     skipped/read using the file's own RAW (unscaled) width/height,
 *     matching the disassembly's own fseek(wdd*coldep*htt) exactly.
 *
 * SCOPE, this milestone: ags_spriteset_load() ports loadSprite's own
 * "sprites are never compressed in this build" real path directly
 * (create_bitmap_ex + a flat per-row fread) -- initialize_sprite's
 * own separate color-depth-conversion logic (convert_16_to_15/
 * convert_16_to_16bgr, for a 15/16-bit target display) is NOT ported:
 * Rob Blanc 1 is confirmed 8-bit throughout (GameSetupStructBase.
 * color_depth==1, every real sprite/background/mask decoded so far
 * has come back coldep==1) so that whole code path is dead weight for
 * this specific game, the same "genuinely dead for this game, not
 * just unported" scoping this project's own reversing notes already
 * apply elsewhere (e.g. M5's room-format-version<9 skip). There is
 * also no LRU cache/eviction here yet (SpriteCache's own mrulist/
 * mrubacklink/liststart/listend/cachesize/maxCacheSize fields are
 * allocated and initialized to match the real struct, but never
 * updated) -- M7 only ever needs ONE sprite alive at a time (the
 * player's own static pose), so cache eviction has nothing to do yet;
 * a later milestone drawing many on-screen sprites per frame is where
 * that logic earns its keep.
 */
#ifndef AGS_SPRITE_LOADER_H
#define AGS_SPRITE_LOADER_H

#include "ags/sprite.h"
#include "ags/gamesetup.h"

enum AgsSpriteLoadError {
    AGS_SPRITE_LOAD_OK = 0,
    AGS_SPRITE_LOAD_OPEN_ERROR = -1,
    AGS_SPRITE_LOAD_READ_ERROR = -2,
    AGS_SPRITE_LOAD_BAD_VERSION = -3, /* sprite-file version >4 -- see the file-level comment above */
    AGS_SPRITE_LOAD_OUT_OF_MEMORY = -4
};

/* SpriteCache plus the two parallel arrays (width[]/height[]) the
 * real engine keeps as SEPARATE globals (spritewidth[]/
 * spriteheight[]) rather than as SpriteCache members -- mirrored here
 * the same way, not folded into the byte-verified struct itself. */
struct AgsSpriteSet {
    struct SpriteCache cache;
    int *width;
    int *height;
};

/* SpriteCache::initFile (Common/sprcache.cpp:631), this build's own
 * real (simpler) format -- see the file-level comment above. Opens
 * `filename` via ags_clib_fopen and builds the full offsets[]/width[]/
 * height[] index for every sprite slot. `game` supplies spriteflags[]
 * for the SPF_640x400 check inside get_new_size_for_sprite. Leaves
 * the file open (set->cache.ff) for ags_spriteset_load's own later
 * lazy per-sprite reads -- close it with ags_spriteset_free(). */
enum AgsSpriteLoadError ags_spriteset_init(struct AgsSpriteSet *set, const char *filename,
                                            const struct GameSetupStructBase *game);

/* SpriteCache::loadSprite (sprcache.cpp:358), this build's own
 * "never compressed" real path -- see the file-level comment above.
 * Returns a fresh, caller-owned Allegro BITMAP* (destroy_bitmap it
 * when done), or NULL if the slot is empty (coldep==0) or index is
 * out of range/a read fails. */
block ags_spriteset_load(struct AgsSpriteSet *set, int index);

/* Frees offsets[]/images[]/mrulist[]/mrubacklink[]/width[]/height[]
 * and closes the sprite file. Does NOT destroy any BITMAP*s
 * ags_spriteset_load already handed to a caller (the caller owns
 * those, per that function's own contract) -- only cache-internal
 * bookkeeping arrays. */
void ags_spriteset_free(struct AgsSpriteSet *set);

#endif /* AGS_SPRITE_LOADER_H */

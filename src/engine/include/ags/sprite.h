/* ags/sprite.h -- SpriteCache and SpriteListEntry. Direct port of
 * apply_structs.py's SAFE_DECLS.
 */
#ifndef AGS_SPRITE_H
#define AGS_SPRITE_H

#include "ags/types.h"

/* SpriteCache -- spriteset, the global sprite LRU cache. DOES carry
 * the full LRU-eviction subsystem (this was a real, self-caught
 * correction in this project -- an earlier round wrongly declared it
 * absent). CONFIRMED ABSENT: sprite0InitialOffset, sizes[]/flags[]
 * (sprite data is never RLE-compressed on disk in this build),
 * spritesAreCompressed, and the hh>1000/removeAll() corruption-
 * recovery safety net 2011 has. */
struct SpriteCache {
    long *offsets;         /* +0x00 */
    long elements;            /* +0x04 */
    block *images;              /* +0x08 (loosely void** in the IDB decl; really BITMAP**) */
    void *ff;                     /* +0x0C, the open sprite file handle (clibfopen'd) */
    long cachesize;                 /* +0x10 */
    int *mrulist;                     /* +0x14 */
    int *mrubacklink;                    /* +0x18 */
    int liststart;                         /* +0x1C */
    int listend;                             /* +0x20, MEDIUM confidence: positional only */
    int lastLoad;                              /* +0x24 */
    long maxCacheSize;                            /* +0x28, this build's own reduced default: 5,000,000 bytes vs. 2011's 20,240,000 */
    long lockedSize;                                /* +0x2C, MEDIUM confidence: positional only */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct SpriteCache) == 0x30, "SpriteCache must be 0x30 bytes");

/* SpriteListEntry -- this build's own per-frame draw-order list
 * (add_to_sprite_list/draw_sprite_list), a genuinely 2002 predecessor
 * of 2011's own O(n log n) qsort-based version -- this build's own
 * sort is still the pre-2.60.672 O(n^2) insertion sort 2011's own
 * source comments explicitly dismiss. CONFIRMED ABSENT:
 * hasAlphaChannel/takesPriorityIfEqual. DRIFT: overflow limit 39 here
 * vs. 2011's MAX_SPRITES_ON_SCREEN=76. */
struct SpriteListEntry {
    void *bmp;         /* +0x00 */
    int baseline;         /* +0x04 */
    int x;                   /* +0x08 */
    int y;                     /* +0x0C */
    int transparent;             /* +0x10 */
} AGS_PACKED_STRUCT;
AGS_STATIC_ASSERT(sizeof(struct SpriteListEntry) == 0x14, "SpriteListEntry must be 0x14 bytes");

#endif /* AGS_SPRITE_H */

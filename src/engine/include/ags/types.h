/* ags/types.h -- shared basic typedefs and the static-assert helper used
 * throughout src/engine/include/ags/. See src/PLAN.md ("Step 0: the
 * skeleton") for the overall struct-porting strategy, and
 * reversing/notes/struct-layout-drift.md / reversing/scripts/
 * apply_structs.py for the full evidence behind every struct these
 * headers declare -- these headers are a direct, field-for-field port
 * of apply_structs.py's own SAFE_DECLS block (the exact C declarations
 * this project already pushes into the live IDB), not a fresh guess.
 *
 * IMPORTANT: every one of these structs is BYTE-PACKED (no implicit
 * compiler alignment padding at all), matching apply_structs.py's own
 * model exactly -- IDA's own struct editor doesn't auto-insert C-style
 * alignment padding either, so every byte in these decls (including
 * every gap) had to be accounted for explicitly by hand, the same
 * discipline a `#pragma pack(1)`/`__attribute__((packed))` struct
 * needs. Concrete proof this project's own evidence already contains:
 * GameSetupStructBase.defpal sits at +0x132, which is NOT a multiple
 * of 4 -- an `int[256]` field a natural-alignment compiler would
 * never place there un-padded. All of these structs are read/written
 * as raw fwrite/fread'd blobs from the actual game data files, so a
 * byte-packed original declaration (likely the ORIGINAL 2002 source
 * used an explicit pack pragma for exactly this reason) is expected,
 * not a curiosity. AGS_PACKED_STRUCT below applies GCC's packed
 * attribute; every struct below uses it, and every struct's own
 * confirmed total size gets an AGS_STATIC_ASSERT right after its
 * closing brace, so a future mistake (wrong field order, a forgotten
 * pad) fails the BUILD instead of silently producing a wrong-shaped
 * struct.
 */
#ifndef AGS_TYPES_H
#define AGS_TYPES_H

#include <allegro.h>

/* This project's own struct-layout-drift.md convention: a bitmap
 * pointer, used verbatim in several already-confirmed fields (e.g.
 * ScreenOverlay.pic, RoomStruct.ebscene[]). */
typedef BITMAP *block;

/* Every struct in ags/*.h is declared as:
 *     struct Foo {
 *         ...
 *     } AGS_PACKED_STRUCT;
 * -- see the file-level comment above for why. */
#if defined(__GNUC__)
#  define AGS_PACKED_STRUCT __attribute__((packed))
#else
#  error "This project's struct layouts require GCC's __attribute__((packed)); add an equivalent for this compiler before building on it."
#endif

#if defined(__STDC_VERSION__) && __STDC_VERSION__ >= 201112L
#  define AGS_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#elif defined(__GNUC__)
   /* GCC accepts _Static_assert as an extension even in gnu99 mode. */
#  define AGS_STATIC_ASSERT(cond, msg) _Static_assert(cond, msg)
#else
#  define AGS_STATIC_ASSERT_CONCAT_(a, b) a##b
#  define AGS_STATIC_ASSERT_CONCAT(a, b) AGS_STATIC_ASSERT_CONCAT_(a, b)
#  define AGS_STATIC_ASSERT(cond, msg) \
      typedef char AGS_STATIC_ASSERT_CONCAT(ags_static_assert_, __LINE__)[(cond) ? 1 : -1]
#endif

#endif /* AGS_TYPES_H */

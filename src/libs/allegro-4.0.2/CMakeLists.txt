# Allegro 4.0.2 -- static library build for the ags/src reimplementation.
#
# Scope notes (see reversing/notes/third-party-library-identification.md
# for the full archaeology writeup this is based on):
#   - Only the generic core (src/*.c), the portable-C fallback blitters
#     (src/c/*.c, built with ALLEGRO_NO_ASM), the Windows platform driver
#     (src/win/*.c), and three portable src/misc/*.c files (colconv.c/
#     pckeys.c -- shared, platform-independent helpers the Windows driver
#     itself calls into; everything else in src/misc/ is DOS/VGA/VESA-
#     specific and deliberately excluded) are built. The original x86
#     NASM/GAS-optimized blitters (src/i386/*.s) are deliberately excluded
#     -- src/win/asmlock.s and src/c/cmiscs.s were the only asm this
#     platform combination actually required, and both have plain-C
#     replacements here (asmlock.c is a hand-written port; cmisc.c already
#     has a built-in ALLEGRO_NO_ASM fallback for the third). The fourth,
#     src/misc/icolconv.s (colconv.c's own 21 per-depth-pair pixel-format
#     blitters, e.g. 8-to-32, 15-to-16), has no ALLEGRO_NO_ASM fallback in
#     4.0.2 itself -- colconv.c's own table-building/dispatch logic is
#     unchanged, but the blitters were only split into a separate,
#     portable-C file (ccolconv.c) in a LATER Allegro release. ccolconv.c
#     is copied in from the newer Engine/libsrc/allegro-4.2.2/src/misc/
#     reference tree (verified to consume exactly the same 3 externed
#     tables 4.0.2's own colconv.c already builds -- a genuine drop-in,
#     not a version-mismatch risk the way struct layouts are elsewhere in
#     this project).
#   - include/allegro/platform/alplatf.h was regenerated for MinGW32 (the
#     copy under Engine/libsrc/ still has fix.sh's own Unix-host output,
#     which is correct there -- that tree is reference material, not meant
#     to build).

file(GLOB ALLEGRO_CORE_SOURCES   "${CMAKE_CURRENT_SOURCE_DIR}/src/*.c")
file(GLOB ALLEGRO_C_SOURCES      "${CMAKE_CURRENT_SOURCE_DIR}/src/c/*.c")
file(GLOB ALLEGRO_WIN_SOURCES    "${CMAKE_CURRENT_SOURCE_DIR}/src/win/*.c")
set(ALLEGRO_MISC_SOURCES
  "${CMAKE_CURRENT_SOURCE_DIR}/src/misc/colconv.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/misc/pckeys.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/misc/ccolconv.c"
)

add_library(allegro STATIC
  ${ALLEGRO_CORE_SOURCES}
  ${ALLEGRO_C_SOURCES}
  ${ALLEGRO_WIN_SOURCES}
  ${ALLEGRO_MISC_SOURCES}
)

target_include_directories(allegro PUBLIC
  "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_compile_definitions(allegro PUBLIC
  ALLEGRO_STATICLINK   # build (and consume) headers without __declspec(dllimport)
  ALLEGRO_NO_ASM       # use the portable src/c/*.c blitters, no NASM/GAS dependency
  ALLEGRO_USE_C        # a few files (poly3d.c/math.c/colblend.c/compiled.h) gate
                       # their OWN separate asm-vs-C fork on this, distinct from
                       # ALLEGRO_NO_ASM -- notably compiled.h's COMPILED_SPRITE,
                       # which on the asm side means generating and executing raw
                       # x86 machine code at runtime, not just inline asm. Forcing
                       # this avoids that entirely, falling back to the portable
                       # RLE-sprite emulation instead.
)
target_compile_definitions(allegro PRIVATE
  ALLEGRO_SRC          # internal-build flag some headers check in addition to STATICLINK
)

target_link_libraries(allegro PUBLIC
  ddraw dsound winmm gdi32 comctl32 ole32
  dinput    # DirectInputCreateA@16 -- this build's keyboard/mouse drivers
            # (wkeybd.c/wmouse.c) target the older, plain-function-based
            # DirectInput 1-7 API, not DirectInput8's COM-based
            # DirectInput8Create -- matching the 2002-era original.
  dinput8   # dinput8.h's own type declarations are still referenced;
            # kept alongside dinput for that, even though no dinput8-only
            # entry point actually gets called here.
  dxguid    # IID_/CLSID_/GUID_ constants for DirectDraw/DirectSound/
            # DirectInput (IID_IDirectDraw2, GUID_SysKeyboard, etc.) --
            # declared extern by the DirectX headers but only actually
            # *defined* in this import lib, not ddraw/dsound/dinput(8).
)

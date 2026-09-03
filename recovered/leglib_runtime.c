/* ==========================================================================
 *  LEGLIB.EXE  --  hand-written runtime support, as C                [spec]
 *
 *  Legacy of the Ancients is compiled Microsoft BASIC 6.0 linked against a
 *  custom BRUN60 (LEGLIB.EXE) that also carries the shared engine.  The
 *  recovered/*.bas files reconstruct the GAME logic as BASIC.  This file
 *  does the opposite for the RUNTIME: it captures the leglib primitives the
 *  disassembly leans on -- the value-stack evaluator, the RNG, the ON..GOSUB
 *  dispatcher, the text/BSAVE helpers -- as C, because none of it should be
 *  ported "as BASIC".  A ScummVM engine calls the C equivalents directly.
 *
 *  This is a SPEC written in C, not drop-in code.  Signatures and bodies are
 *  illustrative; types are named for clarity.  Confidence tags:
 *      [verified]  matched against a DOSBox trace / bit-exact
 *      [exact]     read straight out of LEGLIB.EXE (constants, table layout)
 *      [derived]   from the disassembly, not independently checked
 *
 *  Cross-refs: docs/game-logic.md S1 (arithmetic), docs/overview.md
 *  ("rtm_FF* value-stack cluster"), recovered/README.md (the node model).
 * ========================================================================== */

#include <stdint.h>
#include <math.h>


/* ==========================================================================
 *  1.  THE VALUE STACK IS A COMPILER ARTIFACT -- DO NOT PORT IT
 * ==========================================================================
 *
 *  BASIC 6.0 evaluates every expression on a software stack of 12-byte nodes
 *  (base ds:0FAC, top pointer ds:111C; a node is [8 unused][2 val-ptr][2 tag]
 *  and the live value is the 4-byte float at [ds:111C]).  In the .asm this
 *  shows up as long runs of:
 *
 *      mov bx, <addr> ; call rt_FF4B      ; push SINGLE [addr]
 *      mov ax, <n>    ; call rt_FF20      ; push INT n
 *                       call rt_FF4C      ; multiply top two
 *                       call rt_FF44      ; add immediate
 *                       call rt_FF22      ; pop -> ax
 *      mov <addr>, ax                     ; store
 *
 *  Every such run is ONE C expression.  You never need a stack.  The only
 *  thing that matters is getting the operator and the OPERAND ORDER right,
 *  because the leglib op table (ds:0F7C, read from the EXE) is NOT canonical
 *  BASIC order and has separate "reversed" thunks:
 *
 *   thunk(s)            operation                         C
 *   -----------------   ------------------------------    --------------------
 *   rt_FF44 / rt_FF42   +                                 a + b
 *   sub_21A02           -   (deeper - top)                a - b
 *   rt_FF53             -   reversed (top - deeper)       b - a
 *   rt_FF4E / rt_FF4C   *                                 a * b
 *   sub_21A4A / rt_FF47 /   (deeper / top)                a / b
 *   rt_FF49             /   reversed; as immediate:       tos / imm
 *   rt_FF1F            compare -> CPU flags, then a       (a <=> b); the
 *                      Jcc encodes the actual operator     Jcc picks < > = etc
 *   rt_FF2B            ^   (own thunk seg004:0x3954)      pow(tos, tos1)
 *
 *  rt_FF20 push-int does `cwd` first (sign-extends).  rt_FF4B push-single is
 *  also used right after B$RND with the pointer B$RND returned.
 *  rt_FF2B / rt_FF27 coerce INT<->SINGLE; FF27 (single->int) TRUNCATES
 *  toward zero, whereas the BASIC `INT()` used in the .bas files is FLOOR
 *  (toward -inf).  They differ only for negatives -- watch the -1000
 *  encounter-mod sentinel and any `INT(x)` where x can go negative.
 *
 *  \  and MOD and the 32-bit ops are separate integer thunks
 *  (rt_FF23 / rt_FF28 / rt_FF48 / rt_14 / rt_EE).  `a \ b` == trunc(a / b)
 *  for the ranges the game uses (both operands >= 0).
 */

typedef float  Single;   /* BASIC SINGLE  -- 32-bit IEEE (matches x86 'float') */
typedef int16_t Int;     /* BASIC INTEGER -- 16-bit signed                     */

static inline Int    basInt(Single x)   { return (Int)floorf(x); }   /* INT()  */
static inline Int    basFix(Single x)   { return (Int)(x); }         /* FIX()/FF27 */
static inline Int    basIDiv(Int a, Int b) { return (Int)(a / b); }  /*  \     */


/* ==========================================================================
 *  2.  RNG  --  B$RND  (rtm_B8 -> sub_1A1E5)                        [exact]
 * ==========================================================================
 *
 *  24-bit LCG.  State at ds:0DA5 (low word) + ds:0DA7 (low byte of high).
 *  Constants read straight from LEGLIB.EXE (DGROUP file 5632):
 *      ds:1AE  dword  0x000343FD  = 214013     (multiplier)
 *      ds:1B2  dword  0x00269EC3  = 2531011    (increment)
 *  Initial state in the EXE image: 0x050000 (ds:0DA7 = 5).
 *
 *  advance:  seed = (seed * 214013 + 2531011) & 0x00FFFFFF
 *  RND(1)  = seed / 2^24            -> Single in [0, 1)
 *
 *  (214013 / 2531011 are the stock Microsoft LCG constants and are plainly
 *  present at ds:1AE / ds:1B2.  sub_1A1E5 does the 24-bit multiply as a
 *  hand-rolled 3x `mul` -- if you need BIT-EXACT replay, single-step that
 *  routine once to confirm the exact carry handling and the seed->SINGLE
 *  rounding; the formula above is the intent.)
 *
 *  RND(x<0) reseeds from x; RND(0) repeats the last value (see rtm_B8's
 *  arg_2 branch).  The game only ever calls RND(1), plus one RANDOMIZE at
 *  startup, so a port that does not need bit-exact replay can use any RNG
 *  that yields [0,1).
 */

typedef struct { uint32_t seed; } LegRng;   /* seed masked to 24 bits */

static void   leg_randomize(LegRng *r, uint32_t s) { r->seed = s & 0xFFFFFFu; }

static Single leg_rnd1(LegRng *r)
{
    r->seed = (r->seed * 214013u + 2531011u) & 0xFFFFFFu;
    return (Single)r->seed / 16777216.0f;      /* [0, 1) */
}


/* ==========================================================================
 *  3.  ON <n> GOSUB / GOTO   --   rt_FC  (leglib seg003:0x1ca63)   [derived]
 * ==========================================================================
 *
 *  MS BASIC 6 compiles `ON <bx> GOSUB a,b,c` as:
 *
 *      mov  bx, <selector>          ; 1-BASED
 *      call far ptr rt_FC
 *      db   <count>                 ; inline, right after the call
 *      dw   arm0, arm1, ... armN    ; near offsets, relative to the call CS
 *      <resume point>               ; arms `retn` back here; also where an
 *                                   ; out-of-range selector continues
 *
 *  rt_FC reads <count> off its own return address (`lds si,[bp+2] ; lodsb`),
 *  indexes the table by bx, pushes the resume offset (so the arm's `retn`
 *  lands past the table), then `push CS ; push arm ; retf` into the arm.
 *  Selector < 1 or > count  ==>  no-op (fall through past the table).
 *
 *  IDA does not know this convention and mis-decodes every table as code.
 *  ida_scripts/fix_on_gosub_tables.py repairs the representation.
 *
 *  C equivalent -- a plain switch, 1-based, default = do nothing:
 *
 *      switch (selector) {
 *          case 1: arm0(); break;
 *          case 2: arm1(); break;
 *          ...
 *          default: break;             // NOT an error
 *      }
 *
 *  Every ON GOSUB in the game (list from fix_on_gosub_tables.py):
 *    out  doMovement          ON facing        -> move N/E/S/W (trial x/y +-1)
 *    out  creatureApproach    ON rawTile+1     -> regionPreset_A..E
 *    out  beginEncounterView  ON rawTile+1     -> combatBeat_1..7
 *    out  applyGameFlag       ON flagSel       -> setFlag_03/38/C0/0300/0800/1000
 *    out  setupLocationDisplay/enterLocation   -> per location-type staging
 *    dun  dungeon-exit        ON exitSel       -> chainToOverworld / chainToMuseum
 *    dun / casdr / twndr / mus / celdrv        -> command + state dispatch
 */


/* ==========================================================================
 *  4.  STRING + TEXT OUTPUT                                        [derived]
 * ==========================================================================
 *
 *  basStrAssign(dst,src)  rt_C2   dst = src            (the #1 runtime call)
 *  basStrConcat(a,b)      rt_C3   a + b
 *  basStrClear(s)         rt_D1   s = ""
 *  rtm_D2(fmt,val)        rt_D2   fmt + STR$(val)       (val = Int)
 *  rtm_D3(fmt,lo,hi)      rt_D3   fmt + STR$(val)       (val = int32)
 *  drawString(s)          rtm_FE26  render s, clearing the text area first
 *  drawStringInner(s)     rtm_FE25  render s, no clear
 *  rtm_FE27(ticks)        pause / wait-for-key
 *
 *  In-string control codes handled by the renderer (drawString):
 *     '%'   newline; a LEADING run "%%%..." moves N lines down first
 *     '@'   column / cursor marker (leading)
 *     '!'   trailing: end paragraph (wait, then clear)
 *     '#'   trailing: page break
 *     '$'   trailing: wait for key
 *     '&'   trailing: wait for key, no prompt glyph
 *  BASIC STR$ prints a LEADING SPACE for non-negative numbers -- the game's
 *  "  6." style spacing in combat lines is STR$, not hand-formatting.
 *
 *  Font: LEGACY.DAT bytes 0x006..0x605, 8x8 1bpp, glyph =
 *  fontBase + (ch - 0x20) * 16, blitted field-interleaved to CGA B800.
 *  (decoders/legacy_font.py)
 */


/* ==========================================================================
 *  5.  BSAVE / array file I/O                                      [derived]
 * ==========================================================================
 *
 *  rtm_FE63  resolveAndOpenGameFile -- resolve the drive/dir per DRCONFIG.DAT
 *            (decoders/drconfig_dat.py), open the named file.
 *  rtm_FE07 -> rtm_02  basBload:
 *      header = 7 bytes:  FD  seg(2, LE)  off(2, LE)  len(2, LE)
 *      then `len` bytes of payload copied into a DIM'd BASIC array.
 *      (The FD magic byte is BASIC's BSAVE tag; seg:off is the original
 *       save address and is IGNORED on load -- data goes to the array.)
 *  rtm_FE37 / FE39   GET / PUT a whole BASIC integer array (raw block)
 *  rtm_FE35 / FE36   peek / poke one word of an array
 *  rtm_FE68          read a BASIC string array
 *  rtm_AF            basScreenInit; also the DIM path (allocate an array)
 *
 *  Files the game BLOADs this way: OUTM<n>.BSV / OUTDATA.BSV (overworld
 *  map+monster banks, n = "0"+something), CASTLE.BS1/BS2, CEL*.BSV, *.GLB
 *  (images), CHAR.DAT (the party save -- see docs/file-formats.md).
 */

struct BsaveHeader {              /* the 7-byte prefix on every .BSV/.GLB */
    uint8_t  magic;              /* 0xFD */
    uint16_t seg;               /* original BSAVE segment  -- ignored on load */
    uint16_t off;               /* original BSAVE offset   -- ignored on load */
    uint16_t len;              /* payload byte count                          */
};


/* ==========================================================================
 *  6.  GRAPHICS (bmXXXX first-person / tile engine)                [derived]
 * ==========================================================================
 *  Not "support functions" in the arithmetic sense -- a real subsystem -- but
 *  listed here so the port knows what the far calls are:
 *
 *  rtm_FE29    CGA mode/palette set: out 0x3D8/0x3D9 + INT 10h AH=0Bh, ONCE
 *              per view.  There is NO palette cycling anywhere in the game.
 *  sub_1FED8   atomic 8x8 field-interleaved CGA cell copy (the primitive)
 *  rtm_FE2A    drawTileRun(srcBase, srcSeg, destOff, tileIdxList, count)
 *  rtm_FE2E    andSpriteMaskCell -- AND-blit one mask cell
 *  rtm_61 / rtm_60   basPutSprite / basPutSpriteXor  (stock BASIC PUT;
 *              image = `dw xBits ; dw yRows ; rows of ceil(xBits/8) bytes`,
 *              2bpp linear, colour 0 transparent)
 *  rtm_FE1B    interior single-cell blitter (26x17 grid)
 *  rtm_FE5B    screenRefresh          rtm_FE54  tone / beep (PC speaker)
 *
 *  Music: GW-BASIC PLAY MML strings via basPlayMusic (rtm_CE) on the
 *  speaker.  No music data files.  (decoders/music_mml.py)
 */


/* ==========================================================================
 *  7.  SUB / FUNCTION FRAME                                        [derived]
 * ==========================================================================
 *  basProcEnter / basProcLeave    -- `mov cx,<localBytes>` precedes the call;
 *                                    Leave does `dec ds:118h`, restores the
 *                                    frame, `jmp [ds:73Ah]` back to caller.
 *  Every SUB body ends: `call basProcExit1 ; ... ; jmp basProcExit2`
 *  (or `retf 2`).  Not needed by a C port -- C's own call frames replace it;
 *  noted so the .asm tails read.
 *
 *  stopFlag (ds: a byte, 0xFF = abort) is polled by rtm_F2 / rtm_FB /
 *  rtm_FF07 -- the "END" / Ctrl-Break path.
 */

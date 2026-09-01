' ==========================================================================
'  LEGLIB.EXE  --  shared runtime + engine primitives          [reference]
'  see recovered/README.md for the value-stack model
' ==========================================================================
'
'  LEGLIB.EXE (Feb 1989, ~104 KB) is the BASIC 6.0 runtime (a custom
'  BRUN60) bundled with the game's common engine.  Every module loads it as
'  its RTM at startup and calls in via `int 3Fh` thunks -- a flat
'  (prefix, ordinal) namespace shared by all modules.
'
'  This file is a REFERENCE, not a reconstruction: it lists the primitives
'  the recovered module files call, so those files read on their own.  The
'  runtime itself is not reproduced (it is standard compiled BASIC + the
'  bmXXXX graphics system, ~2400 KB of asm).
'
'  ------------------------------------------------------------------------
'  1.  THE VALUE STACK  (arithmetic in compiled BASIC 6.0)
'  ------------------------------------------------------------------------
'  A software stack of 12-byte nodes, base ds:0FAC, pointer ds:111C.
'  Node = [8 bytes unused][2-byte value-pointer][2-byte type tag].  The
'  value lives at [node.pointer] (= nodeStart + 12), so the TOP value is
'  the 4-byte single at [ds:111C].
'
'    rtm_FF20(ax)          push ax as an INTEGER          (B$PUSHI, cwd first)
'    rtm_FF4B(addr)        push the SINGLE at [addr]
'                          (also used right after B$RND: addr = the pointer
'                           B$RND returned)
'    rtm_FF50(addr)        pop top  ->  [addr]
'    rtm_FF22              pop top  ->  ax   (end of an integer expression)
'    rtm_FF2B / rtm_FF27   coerce  INT<->SINGLE   (FF27 truncates to int)
'
'  Binary ops dispatch through [ds:0F7C + index] (table filled at RTM init;
'  read from LEGLIB.EXE, file offset 5632 + 0x0F7C):
'
'    index  op            immediate thunk     stack-stack thunk
'    0x00   +             rtm_FF44            rtm_FF42
'    0x04   -   (a - b)   sub_21A02           --
'    0x08   -   (b - a)   rtm_FF53            --
'    0x0C   *             rtm_FF4E            rtm_FF4C
'    0x10   /   (T1 / T)  sub_21A4A           rtm_FF47
'    0x14   /   (T / T1)  rtm_FF49  (imm: T / imm)
'    0x18   compare
'    0x1C   compare       --                  rtm_FF1F  (flags for jb/jnb)
'
'    rtm_FF2B  =  `^`  (own thunk, seg004:0x3954; operand order TOS ^ TOS1)
'    `\`, MOD, and the LONG ops (rtm_FF23/FF28/FF48/rtm_14/rtm_EE) are
'    separate integer thunks.
'
'  ------------------------------------------------------------------------
'  2.  RNG
'  ------------------------------------------------------------------------
'    RND(1)   =   push ds:<seg> : push ds:<off> : call rtm_B8 (B$RND)
'    The far-pointer arg is the module's SINGLE constant 1.0
'    (OUT ds:24E6, DUN ds:2274, CASDR ds:25B0, ...).  B$RND returns a
'    pointer to the result single -> fed straight to rtm_FF4B.
'    Result is in [0, 1).   rtm_FC = reseed / advance.
'
'  ------------------------------------------------------------------------
'  3.  STRINGS + TEXT OUTPUT
'  ------------------------------------------------------------------------
'    basStrAssign(dst, src)   dst$ = src$   (rt_C2; the #1 runtime call)
'    basStrConcat(a, b)       a$ + b$       (rt_C3)
'    basStrClear(s)           s$ = ""       (rt_D1)
'    rtm_D2(fmt, val)         fmt$ + STR$(val)          (int val)
'    rtm_D3(fmt, lo, hi)      fmt$ + STR$(val)          (dword val)
'    drawString(s)            render s$ to the CGA screen   (rtm_FE26)
'    drawStringInner(s)       render, no leading clear      (rtm_FE25)
'    rtm_FE27(addr)           pause [addr] ticks / wait for key
'
'    In-string control codes (drawString):
'      %   newline; a leading run %%% positions N lines down
'      @   column / cursor marker (leading)
'      ! # $ &   trailing paragraph / page-break / wait-for-key
'    The 8x8 CGA font is LEGACY.DAT 0x006..0x605 (decoders/legacy_font.py);
'    glyph = fontBase + (ord(ch) - 0x20) * 16, blitted field-interleaved.
'
'  ------------------------------------------------------------------------
'  4.  FILE I/O  (BSAVE images: .BSV / .GLB / CHAR.DAT / ...)
'  ------------------------------------------------------------------------
'    rtm_FE63  resolveAndOpenGameFile -- pick the drive per DRCONFIG.DAT,
'              open the named file
'    rtm_FE07 -> rtm_02   basBload: read the 7-byte [FD][seg][off][len]
'              header, then the payload, into a DIM'd BASIC array
'    rtm_FE37 / FE39      GET / PUT a whole BASIC integer array
'    rtm_FE35 / FE36      peek / poke one word
'    rtm_FE68             read a BASIC string array
'    rtm_AF  (basScreenInit / falls into the DIM runtime)  DIM an array
'
'  ------------------------------------------------------------------------
'  5.  GRAPHICS  (the bmXXXX first-person / tile engine)
'  ------------------------------------------------------------------------
'    rtm_FE29    CGA register set: out 0x3D8/0x3D9 + BIOS INT 10h AH=0Bh.
'                Called ONCE per view -- there is NO palette cycling.
'    sub_1FED8   atomic 8x8 field-interleaved CGA cell copy
'    rtm_FE2A    drawTileRun(srcBase, srcSeg, destOff, tileIdxList, count)
'    rtm_FE2E    andSpriteMaskCell -- AND-blit one mask cell
'    rtm_61 / rtm_60   basPutSprite / basPutSpriteXor  (stock MS-BASIC PUT;
'                image = `dw xBits ; dw yRows ; rows of ceil(xBits/8) bytes`,
'                2bpp linear, colour 0 transparent)
'    rtm_FE1B    the interior single-cell blitter (26x17 grid)
'    rtm_FE5B    screenRefresh       rtm_FE54  tone / beep
'
'  ------------------------------------------------------------------------
'  6.  PROC FRAME
'  ------------------------------------------------------------------------
'    basProcEnter / basProcLeave     SUB frame; `mov cx,N` before the call
'                                    = local-frame size
'    basProcExit1 / basProcExit2     every SUB tail: call Exit1 / jmp Exit2
'
'  The music is GW-BASIC PLAY MML strings played via basPlayMusic (rtm_CE)
'  on the PC speaker -- no music file (decoders/music_mml.py).

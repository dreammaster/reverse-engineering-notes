"""
Fixes the second small mis-disassembled-as-code patch found while
investigating GAME_NAME_TABLE (linear 0x16AE8-0x16B18 in
ultima_exodus.idb). Turns out to hold the exact confirmed solution
data for Ultima III's Exodus endgame puzzle:

  0x16AE9 "F:" (2-byte label fragment, unrelated pre-existing data)
  0x16AED "Moons\0", 0x16AF3 "Death\0", 0x16AF9 "Love\0", 0x16AFE "Sol\0"
      -- the 4 card names, confirmed directly against
      attemptExodusSequence's own dialogue (aDSLM, "D, S, L, M:\n")
  0x16B02 "MDLSQ" + ESC (0x1B) -- the 6 valid getMenuChoice keys for
      that same prompt (4 letters + Quit + Escape)
  0x16B08 4 near-pointers (0x6AED/0x6AF3/0x6AF9/0x6AFE) -- word-array
      pointers to the 4 name strings above, presumably for a menu
      that displays the card names by index
  0x16B10 2 more near-pointers (0x78AF, 0x78AF -- identical, purpose
      not investigated further)
  0x16B14 "LSMD" (4 raw bytes, NOT null-terminated -- a lookup array,
      not a string) -- CONFIRMED via attemptExodusSequence's own
      `cmp al, [bx+6B14h]` (bx=word_164A0, the puzzle's 0-3 step
      counter) to be the exact expected-letter-per-step table. This
      is the actual solution: step 0 wants 'L', step 1 'S', step 2
      'M', step 3 'D' -- i.e. **Love, Sol, Moons, Death**, matching
      the Time Lord's own in-game hint verbatim ("The one way is
      Love, Sol, Moons & Death, All else fails.", found nearby at
      aYouSeeAVisionO).

Defines the 4 name strings via create_strlit, the letter-list and
LSMD array as plain byte arrays (not strings -- no NUL terminator,
and LSMD in particular is a lookup table, not text for display), and
the pointer pairs as word arrays.
"""

import idc
import ida_bytes

DRY_RUN = False


def define_string(ea):
    end = ea
    while ida_bytes.get_byte(end) != 0:
        end += 1
    length = end - ea + 1
    ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, length)
    ok = idc.create_strlit(ea, ea + length)
    if not ok:
        ok = idc.create_data(ea, idc.FF_BYTE, length, idc.BADADDR)
    return ok, length


def define_bytes(ea, size):
    ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, size)
    return idc.create_data(ea, idc.FF_BYTE, size, idc.BADADDR)


def define_words(ea, count):
    size = count * 2
    ida_bytes.del_items(ea, ida_bytes.DELIT_SIMPLE, size)
    return idc.create_data(ea, idc.FF_WORD, size, idc.BADADDR)


def main():
    if DRY_RUN:
        print("[dry] would fix 0x16AE9-0x16B18; set DRY_RUN=False to apply")
        return

    # "F:" label fragment (2 bytes, no NUL) at 0x16AE9
    ok = define_bytes(0x16AE9, 2)
    print(f"0x16ae9 'F:' (2 bytes): {ok}")

    for ea, name in [(0x16AED, "aMoons"), (0x16AF3, "aDeath_0"),
                      (0x16AF9, "aLove"), (0x16AFE, "aSol")]:
        ok, length = define_string(ea)
        idc.set_name(ea, name, idc.SN_NOWARN)
        print(f"{ea:#x} {name} ({length} bytes): {ok}")

    ok = define_bytes(0x16B02, 6)  # 'M','D','L','S','Q',0x1B
    idc.set_name(0x16B02, "EXODUS_SEQUENCE_MENU_KEYS", idc.SN_NOWARN)
    print(f"0x16b02 EXODUS_SEQUENCE_MENU_KEYS (6 bytes): {ok}")

    ok = define_words(0x16B08, 4)
    idc.set_name(0x16B08, "EXODUS_SEQUENCE_NAME_PTRS", idc.SN_NOWARN)
    print(f"0x16b08 EXODUS_SEQUENCE_NAME_PTRS (4 words): {ok}")

    ok = define_words(0x16B10, 2)
    print(f"0x16b10 (2 words, purpose unconfirmed): {ok}")

    ok = define_bytes(0x16B14, 4)
    idc.set_name(0x16B14, "EXODUS_SEQUENCE_ANSWER", idc.SN_NOWARN)
    print(f"0x16b14 EXODUS_SEQUENCE_ANSWER 'LSMD' (4 bytes): {ok}")


if __name__ == "__main__":
    main()

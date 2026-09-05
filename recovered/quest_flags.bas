' ==========================================================================
'  Quest-flag semantics  --  cross-module (OUT sets / DUN sets+tests /
'  MUS tests)                                                        [v1]
'  reconstructed from out.asm (setupLocationDisplay/applyGameFlag),
'  dun.asm (dunMain/climbDownOrExit), mus.asm (sub_10B59/checkFlag_*/
'  testExhibitFlag) ; see recovered/README.md for the model + tags
'
'  questFlagWord = S4(11) = [ds:1B96 desc + 0x16] -- ONE 16-bit sticky
'  bitfield, OR-only (never cleared), shared verbatim across the whole
'  CHAR.DAT resident record.  It is the game's entire "story progress"
'  state outside of item possession.  TWNDR and CASDR never touch it.
'
'  Two independent mechanisms both key off it:
'    (1) OUT: "do you STILL hold gem coin N" -> OR a fixed mask in, once,
'        every time you re-examine your inventory (setupLocationDisplay,
'        the per-step landmark-array refresh).  DUN adds two more sources
'        of its own (dungeon-exit side quests).
'    (2) MUS: "is the flagWord's masked subset EXACTLY set" gates which
'        museum exhibit GROUP will let you in (testExhibitFlag does
'        `(flagWord AND mask) == mask`, i.e. ALL bits in the mask must be
'        set -- not "any bit").
' ==========================================================================
'
'  ------------------------------------------------------------------------
'  1.  THE 7 GEM COINS = S2(17..23) = Item$(17..23)               [SOLID]
'  ------------------------------------------------------------------------
'  S2() (ds:1BC4, the 24-item possession/count array) and the LEGACY.DAT
'  item-name string array share ONE 0-based index space over items
'  [55..78]; gem coins are the last 7 (index 17..23):
'
'      S2 idx   item name          bit(s) set (mask)   set by
'      17       Jade coin          0-1     (0x03)      OUT: hold it
'      18       Topaz coin         3-5     (0x38)      OUT: hold it
'      19       Amethyst coin      6-7     (0xC0)      OUT: hold it
'      20       Sapphire coin      8-9     (0x0300)    OUT: hold it
'      21       Turquoise coin     --      (none)      OUT: hold it, but
'                                                       does NOT set a bit
'                                                       (see 116FE below)
'      22       Ruby coin          11      (0x0800)    OUT: hold it, OR
'                                                       DUN dungeon-3 exit
'                                                       w/ Guard jewel > 3
'      23       Diamond coin       12      (0x1000)    OUT: hold it
'
'  out.asm setupLocationDisplay (the ON..GOSUB, out.asm:~2611): once per
'  step it re-scans S2(17..23) (the 7 coin slots) and, for every slot you
'  CURRENTLY hold (count > 0), re-fires the matching SetFlag_xx (or, for
'  slot 21 / Turquoise, sub_116FE which sets ds:222Eh=2, a UI transition
'  flag, and skips the story bitfield entirely). Losing/spending a coin
'  later does NOT clear its bit -- "have you EVER held this coin" is
'  permanent, by construction (OR-only).
'
'  applyGameFlag (out.asm:3072): questFlagWord = questFlagWord OR flagMask
'  (via setTileState). setFlag_C0 (Amethyst) ALSO separately checks
'  Intelligence >= 28 with an RND roll into ds:222Eh -- a UI-transition
'  side effect, does not change which bit is set.
'
'
'  ------------------------------------------------------------------------
'  2.  TWO EXTRA BITS COME FROM DUN, NOT FROM HOLDING A COIN        [SOLID]
'  ------------------------------------------------------------------------
'  climbDownOrExit ("YOU CLIMB OUT OF THE DUNGEON.", dun.asm:~1460) runs
'  ON dungeonNumber GOSUB on every dungeon exit and ORs a per-dungeon,
'  per-condition mask into the SAME questFlagWord:
'
'      dungeon 1 exit:  IF S2(16)>0 [Crown] AND S2(20)>0 [Sapphire coin]
'                        THEN bit 4  (mask 0x10)                    [SOLID]
'      dungeon 2 exit:  ALWAYS                bit 8  (mask 0x100)   [SOLID]
'      dungeon 3 exit:  IF S2(14) > 3         [Guard jewel, >3 held]
'                        THEN bit 11 (mask 0x800 -- SAME bit Ruby sets)
'
'  Side effect, gated on the mask having fired THIS exit (dun.asm:~1506):
'      strengthFloor = 10*dungeonNumber + (dungeonNumber > 1 ? 20 : 15)
'                    ' dungeon 1->25, 2->40, 3->50
'      IF strengthFloor > Strength THEN
'          PRINT "STRENGTH: +"; strengthFloor - Strength
'          Strength = strengthFloor                 ' ds:1B08
'      END IF
'  -- this is the "third challenge / dungeon Strength reward" noted
'  elsewhere in game-logic.md, now exact: exiting a dungeon under the
'  qualifying condition sets your Strength to a FLOOR (25/40/50), never
'  lowers it.  (Also bit 8 fires unconditionally on dungeon-2 exit, so
'  its floor of 40 always applies once you've been to dungeon 2 at all.)
'
'  dunMain's level-load init (dun.asm:~394) reads the SAME two conditions
'  back and, if true, CONSUMES the items that earned them:
'      entering dungeon 1 with bit 4 already set  -> S2(16)=0, S2(20)=0
'                                                     (Crown + Sapphire spent)
'      entering dungeon 3 with bit 11 already set -> (mirror consequence,
'                                                     not fully traced --
'                                                     see OPEN)
'
'
'  ------------------------------------------------------------------------
'  3.  MUS: WHICH EXHIBIT NEEDS WHICH COIN                         [SOLID]
'  ------------------------------------------------------------------------
'  enterExhibit (mus.asm:975) maps exhibitId (0..13, the 14 real display
'  cases) to chainTargetIdx via a threshold staircase (>3/>6/>8/>10/>11/>12)
'  -- and chainTargetIdx IS the required coin's S2/Item$ index (17..23):
'
'      exhibitId  exhibit name                    chainTargetIdx = coin
'      0          Ancient Artifact                17  Jade
'      1          The Ancient Art of Weaponry      17  Jade
'      2          Thornberry (a Tarmalon town)     17  Jade
'      3          (unnamed -- sub_1100D)           17  Jade
'      4          The Herb of Life                 18  Topaz
'      5          Pirate Treasure                  18  Topaz
'      6          Native Currency                  18  Topaz
'      7          Stones of Wisdom  (-> STDRV)     19  Amethyst
'      8          (unnamed -- sub_110D0)           19  Amethyst
'      9          Test for Knights                 20  Sapphire
'      10         (unnamed -- sub_1111A)           20  Sapphire
'      11         The Guardian                     21  Turquoise
'      12         The Four Jewels                  22  Ruby
'      13         Flight of Fancy                  23  Diamond
'
'  `PRINT "(INSERT "; Item$(chainTargetIdx); ")"` -- same string used for
'  the possession check, so the coin name shown IS the literal gem name
'  (not a random flavour string). Confirmed: Amethyst coin (group idx 19,
'  Stones of Wisdom) matches the save-diff observation of S2(19) going
'  1->0 after one Stones-of-Wisdom play.
'
'
'  ------------------------------------------------------------------------
'  3b.  A SECOND OUT source: eating the museum's fruit                [SOLID]
'  ------------------------------------------------------------------------
'  `eatFruitCommand` ("EAT THE FRUIT", the Herb-of-Life exhibit's action)
'  unconditionally ORs bit 3 (mask 0x08) into questFlagWord -- independent
'  of any coin.  Bit 3 is also part of Topaz's mask (0x38 = bits 3-5), so
'  eating the fruit and holding the Topaz coin both light the same bit.
'
'  ------------------------------------------------------------------------
'  3c.  MUS: a THIRD source -- per-exhibit "you finished THIS one" bits
'                                                                    [SOLID
'       mechanism, one leaf (bit 0x2000) still open]
'  ------------------------------------------------------------------------
'  Independent of the coin system, MUS gives every one of its 16 exhibit
'  slots (`exhibitId` 0..15, tile codes 0xE0..0xEF) its OWN permanent bit
'  in questFlagWord: `sub_11C38` computes
'      mask = INT( 2.0 ^ (exhibitId - 1) )        ' rt_FF2B = `^`, ds:23FC=2.0
'      questFlagWord = questFlagWord OR mask
'  and is called (via the shared wrapper `sub_11D02`) from 9 different
'  per-exhibit "outcome" handlers -- one per exhibit's own arm of the
'  `ON (exhibitId+1) GOSUB` dispatch that follows `enterExhibit`.
'
'  This is independently verified against 5 of the already-known coin
'  bits -- EXACT match every time:
'      exhibitId 4,5,6   (Topaz group)     -> bits 3,4,5   = 0x38  [Topaz]
'      exhibitId 7,8     (Amethyst group)  -> bits 6,7     = 0xC0  [Amethyst]
'      exhibitId 9,10    (Sapphire group)  -> bits 8,9     = 0x0300[Sapphire]
'      exhibitId 12      (Ruby, "Four Jewels")  -> bit 11  = 0x0800[Ruby]
'      exhibitId 13      ("Flight of Fancy")    -> bit 12  = 0x1000[Diamond]
'  i.e. visiting/completing every exhibit in a coin's group, ONE AT A
'  TIME, sets exactly the bits that OUT's "hold the coin" scan sets all
'  at once -- the two systems are two paths to the SAME bits.
'  exhibitId 3 (the 4th, unnamed Jade-group exhibit) additionally sets
'  bit 2 (0x04), which is NOT part of Jade's own mask (0x03) -- a bonus/
'  secret bit with no other known consumer yet.
'
'  exhibitId 14's own name is **"INFORMATION"** (`sub_110A8`) and its
'  outcome-arm is `checkFlag_2000` itself -- i.e. exhibit #14 IS the
'  caretaker's desk.  Its bit under this formula is
'  `2^(14-1) = 2^13 = 0x2000` -- the EXACT bit `checkFlag_2000` tests.
'  **The mechanism is confirmed; the literal first-time SETTER is not.**
'  `checkFlag_2000`'s only call into the setter (`sub_11D02` -> `sub_11C38`)
'  is in its "bit ALREADY set" branch -- circular, cannot be the initial
'  trigger.  Exhaustively ruled out as the setter: `caretakerOffer` (every
'  branch), `caretakerPraise`, `sub_12CAC`, `sub_10D3F`, `sub_11B45`,
'  `sub_12AF4`, and `loc_10B64`/`sub_10B59` -- none of them call the full
'  `sub_11D02` entry or write the bit directly.  `sub_11C38` has exactly
'  ONE call site in the whole file, so the real trigger must be inside
'  `useCommand`'s much larger main tile/object SELECT CASE (the tile-0xE0
'  dispatch leading to `loc_12323`), which this pass did not fully sweep.
'
'  Two OTHER parallel exhibit-bit arrays exist, both tested (not set) via
'  the SAME `2^(exhibitId-1)` formula: `S4(2)` (`enterExhibit`'s own entry
'  gate -- can you even walk into this exhibit) and `S4(35)` (`sub_10D3F`,
'  purpose not traced). All three arrays share one bit-assignment scheme.
'
'  ------------------------------------------------------------------------
'  4.  MUS: THE STORY-BIT GATE PER EXHIBIT GROUP                   [SOLID
'      structure; two leaf sources still open]
'  ------------------------------------------------------------------------
'  A SEPARATE progress counter, S4(10) (ds:1B96 elem 0x14), drives
'  `sub_10B59` ("is the NEXT exhibit group open yet?", called from the
'  museum's U-se command): `ON (S4(10)+1) GOSUB` over 8 arms, one per
'  coin-group RANK (0..6) plus a terminal arm. Each arm's gate --
'  testExhibitFlag does `(questFlagWord AND mask) = mask` (ALL bits
'  required, not "any"):
'
'      rank  group (coin)      gate
'      0     Jade               questFlagWord AND 0x03 = 0x03
'                                (i.e. Jade's own bit -- trivially first)
'      1     Topaz              questFlagWord AND 0x2B = 0x2B
'                                = Jade's bits (0-1) AND part of Topaz's
'                                (3,5) -- PROGRESSIVE: needs Jade's flag
'                                too, not just Topaz's.  (OR: S4(37) <> 0,
'                                an override -- not yet identified.)
'      2     Amethyst           questFlagWord AND 0xD0 = 0xD0
'                                = bit 4 (the DUN "Crown+Sapphire, dungeon
'                                1" side quest) AND Amethyst's own bits
'                                (6-7) -- a PARALLEL prerequisite, not
'                                Topaz's flag
'      3     Sapphire           questFlagWord AND 0x0300 = 0x0300
'                                (Sapphire's own bits only -- not
'                                cumulative this time)
'      4     Turquoise          S4(18) >= 2   (a plain counter threshold,
'                                NOT the story bitfield at all -- matches
'                                Turquoise not setting a bit in step 1)
'      5     Ruby               questFlagWord AND 0x0800 = 0x0800
'                                (Ruby's own bits, OR the DUN dungeon-3
'                                Guard-jewel achievement, same bit)
'      6     Diamond            S2(15) > 0   (hold the Compendium -- a
'                                plain item check, not the bitfield)
'      7     (terminal)         always true
'
'  A SECOND, independent gate exists purely on bit 0x2000 (checkFlag_2000,
'  called directly from useCommand, not through the rank ladder above):
'  if set, show a fixed "already done" line; if NOT set, invoke
'  caretakerOffer (the level-up dialogue). So bit 13 (0x2000) reads as
'  "have you already taken the caretaker's FINAL offer".  **The caretaker
'  / character-level mechanism is now FULLY solved** -- see
'  recovered/mus_caretaker.bas: ds:1AE0 (level) = ds:20B6 (the rank you
'  just qualified for), written in exactly one place (sub_12CAC), with an
'  exact max-HP formula alongside it.  Bit 0x2000's own setter turned out
'  to be a SEPARATE, still-unlocated site -- caretakerOffer/Praise/
'  sub_12CAC/sub_12AF4/sub_12C67 are now fully decoded end-to-end and none
'  of them touch it.
'
'
'  ==========================================================================
'  SOLID
'   * questFlagWord = S4(11), one sticky OR-only 16-bit bitfield, shared
'     verbatim OUT/DUN/MUS (TWNDR/CASDR never touch it)
'   * the 7 gem coins (S2/Item$ 17..23) each map to a fixed bit-mask (or,
'     for Turquoise, to no bit at all) via OUT's per-step re-scan
'   * DUN's climbDownOrExit adds bit 4 (dungeon1, Crown+Sapphire) and
'     unconditionally bit 8 (dungeon2); bit 11 is shared with Ruby
'   * DUN's dungeon-exit Strength floor: 25/40/50 for dungeon 1/2/3,
'     gated on the same conditions
'   * MUS enterExhibit's exhibitId -> required-coin table (14 exhibits,
'     7 coins, verified against the Amethyst/Stones-of-Wisdom save-diff)
'   * MUS's testExhibitFlag is an ALL-BITS-SET test (AND then compare to
'     the mask, not compare to zero) -- corrects the earlier "OR raw"
'     description in mus_exhibits.bas
'   * MUS's per-coin-group unlock ladder (S4(10) rank counter, 8 arms) and
'     its exact gates (mostly bitfield AND-equality, two are plain
'     counter/item checks instead)
'   * eatFruitCommand ORs bit 3 (0x08) unconditionally -- a 2nd non-coin
'     bit source (Turquoise-holding is the 1st, which sets none)
'   * the per-exhibit "own bit" mechanism: sub_11C38 ORs
'     `INT(2.0 ^ (exhibitId-1))` into questFlagWord, called from 9 of the
'     16 exhibits' own outcome-arms -- verified exact against 5 already-
'     known coin bits (Topaz/Amethyst/Sapphire/Ruby/Diamond groups).
'     exhibitId 14 = "INFORMATION" = the caretaker's own desk (its
'     outcome-arm IS checkFlag_2000).
'
'  OPEN
'   * bit 0x2000's literal setter. The MECHANISM is now known (it would be
'     sub_11C38 firing with exhibitId=14, giving exactly 2^13=0x2000) but
'     the trigger is not: checkFlag_2000's only call into the setter is
'     gated behind the bit ALREADY being set (circular -- can't be the
'     first trigger). caretakerOffer (every branch), caretakerPraise,
'     sub_12CAC, sub_10D3F, sub_11B45, sub_12AF4, and loc_10B64/sub_10B59
'     are ALL fully read and NONE of them call the setter or write the
'     bit directly -- sub_11C38 has exactly one call site in the whole
'     file. The real trigger is most likely inside useCommand's much
'     larger main tile/object dispatch (thousands of un-swept bytes
'     leading to loc_12323), not in the caretaker-specific functions.
'   * S4(2) and S4(35): two OTHER arrays tested (not set) via the same
'     2^(exhibitId-1) scheme (entry-permission gate and an unidentified
'     threshold, respectively) -- a third parallel bitfield family,
'     purpose of the latter two not traced
'   * S4(37) override in the Topaz-rank gate
'   * S4(18) threshold semantics for the Turquoise rank (">= 2" of what?)
'   * dungeon-3-exit's item-consumption consequence (mirrors dungeon 1's
'     Crown+Sapphire spend, not confirmed byte-for-byte)
'   * bits 10, 15 of questFlagWord: unaccounted for (bit 14 -- exhibitId
'     15's own bit via sub_11C38 -- and bit 2 -- exhibitId 3's bonus bit
'     -- are now explained by the per-exhibit mechanism above)
'   * exhibitId 3, 8, 10 (sub_1100D/sub_110D0/sub_1111A) -- the other
'     unnamed exhibit slots; not traced
' ==========================================================================

' ==========================================================================
'  OUT.EXE  --  quest flags + found items                            [v1]
'  reconstructed from out.asm ; see recovered/README.md for the model + tags
'
'  SUBs: ApplyGameFlag + the SetFlag_* family, AwardFoundItem
' ==========================================================================
'
'  DGROUP vars:
'     questFlagWord = S4(11) = ds:1B96 elem 0x16  (a 16-bit story bitfield;
'        the SAME word MUS's testExhibitFlag masks)
'     flagMask 2234   flagWork 2238   flagTransitioned 222E
'     droppedItemId 1AEE   S2() = ds:1BC4  (24-item possession counts + spells)


' --------------------------------------------------------------------------
SUB ApplyGameFlag                                     ' asm: out.asm:2901 (applyGameFlag)
' --------------------------------------------------------------------------
' Shared tail of the SetFlag_* family: OR `flagMask` into the quest bitfield.
    questFlagWord = questFlagWord OR flagMask         ' via setTileState(,,1) ' asm:2903-2924
    flagTransitioned = (flagMask <> lastFlagMask)     ' ds:222E             ' asm:2925-2934
END SUB

'   SetFlag_03    ->  questFlagWord OR= &h0003   (+ also ORs a bit into
'                     ds:222E from  S3(3) <> 0  and  S2(10) > 0)   asm:2684
'   SetFlag_38    ->  questFlagWord OR= &h0038                     asm:2727
'   SetFlag_C0    ->  questFlagWord OR= &h00C0   (+ a check on
'                     Intelligence >= 28 -- the potion-wizard gate) asm:2738
'   SetFlag_0300  ->  questFlagWord OR= &h0300                     asm:2786
'   SetFlag_0800  ->  questFlagWord OR= &h0800                     asm:2827
'   SetFlag_1000  ->  questFlagWord OR= &h1000                     asm:2851
'
'   MUS tests the same word (testExhibitFlag) with masks
'   &h03 / &h2B / &hD0 / &h0300 / &h0800 / &h1000 / &h2000 -- so the
'   overworld sets story bits and the museum exhibits react to them.
'   FULL per-bit mapping (which coin sets which bit, which exhibit/rank
'   needs which, DUN's two extra bits) -- see recovered/quest_flags.bas.


' --------------------------------------------------------------------------
SUB AwardFoundItem                                    ' asm: out.asm:8315 (awardFoundItem)
' --------------------------------------------------------------------------
' "YOU FIND A <item>" -- add one of item id `droppedItemId` to the party.

    PRINT "YOU FIND A "                                                   ' asm:8316-8330
    IF droppedItemId = 19 THEN PrintSpecialFindText   ' item 19 -- unique  ' asm:8333-8349
    PRINT UseVerb$(droppedItemId); " "; Item$(droppedItemId); "."         ' asm:8351-8382

    S2(droppedItemId) = S2(droppedItemId) + 1         ' add to inventory   ' asm:8384-8396
    Delay &h1B : StageSfx_Item                                            ' asm:8397-8403
END SUB


' ==========================================================================
'  SOLID
'   * questFlagWord = S4(11) -- one 16-bit story bitfield, shared with MUS
'   * SetFlag_XX OR a fixed mask into it; MUS exhibits gate on the bits
'   * AwardFoundItem: S2(droppedItemId) += 1  ("YOU FIND A <item>")
'
'  OPEN
'   * which creatures / events set droppedItemId, and to what
'   (per-bit story-flag meaning is now SOLVED -- recovered/quest_flags.bas)

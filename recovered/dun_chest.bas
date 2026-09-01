' ==========================================================================
'  DUN.EXE  --  chests / boxes / the quest jewel                      [v1]
'  reconstructed from dun.asm ; see recovered/README.md for the model + tags
'
'  SUBs: OpenChest, RollChestContents, FindJewel
'  DUN DGROUP:0 = DUN.EXE file offset 0x5F00.
' ==========================================================================
'
'  DGROUP vars (DUN):
'     dungeonNumber 1ACA (1..3)   dungeonPos 1AE2 (= level<<8 | cell)
'       -> dungeon level = 1AE2 \ 256   (0..7)
'     partyGold 1AD2:1AD4 (dword)   lastLootMark 1AD6:1AD8 (dword)
'     jewelFound 2150   droppedItemId 1AEE   S2() = ds:1BC4 (item bitmap)
'     ds:2834 = 60


' --------------------------------------------------------------------------
SUB OpenChest                                         ' asm: dun.asm:2145 (openChest)
' --------------------------------------------------------------------------
' Fires on the OPEN command against a chest / box tile (feature code 8 or
' 14 = revealed TREASURE CHEST / BOX).

    PRINT "YOU OPEN THE "; Feature$(containerAhead); "."                  ' asm:2146-2176

    IF dungeonPos >= &h700 THEN FindJewel      ' level 7 only            ' asm:2186-2192
    IF jewelFound THEN jewelFound = 0 : GOTO Done                        ' asm:2196-2204

    ' guard against re-looting the same level: a per-level gold high-water
    ' mark in 1AD6:1AD8 -- if partyGold has grown past it, the chest is spent
    IF partyGold > lastLootMark THEN                                     ' asm:2208-2214
        PRINT "THE CHEST IS EMPTY."                                       ' asm:2218
        GOTO Done
    END IF

    RollChestContents
Done:
END SUB


' --------------------------------------------------------------------------
SUB RollChestContents                                 ' asm: dun.asm:5800/5807 (rollChestContents)
' --------------------------------------------------------------------------
' A chest that isn't empty pays gold that scales with depth.

    chestBase = (10 * dungeonNumber + (dungeonPos \ 256)) * 20 + 20       ' asm:5807-5818
    '   dungeon 1, level 0 -> 220 ; dungeon 3, level 7 -> (30 + 7)*20 + 20 = 760

    goldFound = INT( chestBase * RND(1) + 60.0 )      ' ds:2834 = 60      ' asm:5820-5833
    PRINT "YOU FIND "; goldFound; " GOLD."                                ' asm:5834-5858
    partyGold = partyGold + goldFound                                    ' asm:5860-5865
    lastLootMark = 5                                  ' rtm_FE56 arg      ' asm:5866-5869 '??
END SUB


' --------------------------------------------------------------------------
SUB FindJewel                                         ' asm: dun.asm:5245 (findJewel)
' --------------------------------------------------------------------------
' The quest jewel -- a chest on dungeon level 7, once.

    IF atJewelCell AND S2(20) = 0 THEN                ' don't already hold it ' asm:5252-5279
        jewelFound  = -1                              ' ds:2150           ' asm:5283
        droppedItemId = 20                            ' ds:1AEE           ' asm:5284
        GrantItem 27                                  ' ds:20EA ; sub_12B5A ' asm:5285-5286
        PRINT "YOU FIND A LARGE PULSATING JEWEL"                          ' asm:5288
    END IF
END SUB


' ==========================================================================
'  SOLID
'   * chest gold = INT( chestBase * RND(1) + 60 ),
'     chestBase = (10*dungeonNumber + dungeonLevel) * 20 + 20
'   * a per-level gold high-water mark stops chest re-farming
'   * the quest jewel is a level-7 chest, granted once (S2(20) gate)
'
'  OPEN
'   * lastLootMark exact update (rtm_FE56); atJewelCell position test
'   * whether non-gold items (potions, gear) can drop from a chest

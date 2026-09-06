' ==========================================================================
'  TWNDR.EXE  --  town services                                       [v2]
'  reconstructed from twndr.asm ; see recovered/README.md for the model + tags
'
'  SUBs: SpendGold  (shared "pay N gold"), Bank,
'        WeaponArmorShop / FoodShop / MailDeliveryJob (shops + jobs),
'        GuardAttack / FightGuard / StealGold / OfferGuardBribe /
'        ArrestedByGuards (the town-guard system).
'        townServiceDispatch (ds:1F22) is the ~6 KB SELECT CASE that the
'        shop entries route the actual buy/sell transaction through.
'
'  TWNDR DGROUP:0 = TWNDR.EXE file offset 0xAE60.
' ==========================================================================
'
'  *** CORRECTION to the CHAR.DAT map ***
'  ds:1AC2:1AC4 (CHAR.DAT record +0x10), previously logged as "experience",
'  is the BANK BALANCE.  It is 0 in every fresh save because you have not
'  deposited yet.  LotA appears to have no XP stat at all -- level comes
'  from the museum caretaker, attributes from quests / Stones of Wisdom.
'
'  DGROUP vars:
'     partyGold 1AD2:1AD4 (dword)   bankBalance 1AC2:1AC4 (dword)
'     daysElapsed S4(36) = ds:1B96 elem 0x24 (from ds:1AF4)
'     townServiceId 1F22 (which building: 0/2/4/5/9/0x0A/0x0B = food / weapon
'                         / armour / bank / ...)
'     guardHitPoints 216E


' --------------------------------------------------------------------------
SUB SpendGold                                         ' asm: twndr.asm:517 (spendGold)
' --------------------------------------------------------------------------
' The shared "pay <amount> gold" helper -- every shop, the bank, the
' moneylenders and guardAttack call it.  <amount> arrives on the value stack.
    partyGold = partyGold - amount                                        ' asm:519-531
    FlashGoldGauge                                    ' rtm_FE55          ' asm:532-545
END SUB


' --------------------------------------------------------------------------
SUB Bank                                              ' asm: twndr.asm:5060+ (townServiceDispatch case 5)
' --------------------------------------------------------------------------
' "CONVIENENCE BANK" -- 1. DEPOSIT FUNDS  2. WITHDRAW FUNDS  3. BALANCE INQUIRY

    ' ---- interest accrues on every visit -------------------------- asm:4923-5000
    '   interest = MIN(1500, MIN(5000, bankBalance) * daysElapsed \ 999)
    '   ds:2C3C is an 8-BYTE DOUBLE constant = 999.0 (bank math is double
    '   precision; a 4-byte read of it is 0, which misled an earlier note).
    '   daysElapsed = S4(36), fed from ds:1AF4 (the terrain-wear
    '   accumulator -- a distance-travelled proxy, not literal days).
    capped   = bankBalance : IF capped > 5000 THEN capped = 5000          ' asm:4929-4941
    interest = capped * daysElapsed \ 999                                 ' asm:4944-4970
    IF interest > 1500 THEN interest = 1500                               ' asm:4974-4986
    bankBalance = bankBalance + interest                                  ' asm:4991-5000

    PRINT "CURRENT BALANCE: "; bankBalance                                ' asm:5212-5237

    SELECT CASE Menu3()
    CASE 1  ' DEPOSIT
        IF partyGold < 1 THEN PRINT "YOU HAVE NOTHING TO DEPOSIT" : EXIT SUB ' asm:5243-5256
        n = PromptQuantity("DEPOSIT HOW MUCH?")                           ' asm:5285
        IF n > partyGold THEN n = partyGold                               ' asm:5351-5356 (approx)
        partyGold   = partyGold - n
        bankBalance = bankBalance + n
    CASE 2  ' WITHDRAW
        n = PromptQuantity("WITHDRAW HOW MUCH?")
        IF n > bankBalance THEN n = bankBalance
        bankBalance = bankBalance - n
        partyGold   = partyGold + n
    CASE 3  ' BALANCE INQUIRY -- already shown above
    END SELECT
END SUB


' --------------------------------------------------------------------------
SUB WeaponArmorShop                     ' asm: weaponShopEntry / armorShopEntry
'                                              / townServiceDispatch / sub_11FFA
' --------------------------------------------------------------------------
' Menu: 1 BUY  2 SELL  (3 LEAVE).
'
' ---- BUY ----  townServiceDispatch, case reached from the shop entry.
'   The shop's STOCK is per-slot DATA, not a formula: each menu slot holds
'   an item id  = S4viewArray(0x24 + slot)      [ds:1C7C elem 0x48/2 + n]
'   a condition = S4viewArray(0x2D + slot)      [       elem 0x5A/2 + n]
'   a PRICE     = S4viewArray(0x36 + slot)      [       elem 0x6C/2 + n]
'   -- all loaded from the town's TOWN<n>.BSV shop record.
'     IF townType = &h0C AND RND(1) < 0.3 THEN             ' ds:1E20, ds:2B84
'         PRINT "THE "; Weapon$(itemId); " IS NOT FOR SALE." : EXIT SUB
'     END IF
'     IF partyGold < price THEN NotEnoughGold : EXIT SUB
'     partyGold = partyGold - price                        ' 32-bit
'     S0(freeSlot) = itemId : S1(freeSlot) = condition     ' add to inventory
'
' ---- SELL ----  sub_11F51 (base value) then sub_1200B (haggled price).
'   itemId = ds:1F04   condition = ds:1F06 (0..4)
'   IF itemId <= 8 THEN   ' WEAPON
'       baseValue = INT( ((itemId^1.05 + condition/2.8 + 2) ^ 2.1) * 4 - 10 )
'   ELSE                  ' ARMOUR (itemId 9..13)
'       baseValue = INT( ((itemId^3.2  + condition/3.5 + <k>) ^ <e>) * <m> - 6 )
'       ' (the armour branch's 4 consts: ds:2B66 3.2, ds:2B6A 1.02,
'       '  ds:2B6E 3.5, ds:2B72 -6 -- exact assembly of the polynomial
'       '  not fully pinned; same shape as the weapon branch)
'   END IF
'   raw       = baseValue * (Charm ^ 0.7) / 11             ' ds:2B76, ds:2B7A
'   offer     = INT( MIN(raw, baseValue) * 0.8 )           ' ds:2878 ; cap at base
'   PRINT "I'LL PAY EXACTLY "; offer; " GOLD FOR YOUR "; Item$(itemId)
'   IF YesNo() THEN partyGold = partyGold + offer : <remove item>
'  -- Charm 15 -> ~47% of base ; Charm 30 -> ~77% ; higher Charm haggles better.
END SUB


' --------------------------------------------------------------------------
SUB FoodShop                                          ' asm: twndr.asm:2082 (foodShop)
' --------------------------------------------------------------------------
' The provisioner ("FOOD & WATER", "WE SELL FOOD FOR TRAVEL").
'
' ---- ON ENTRY: deliver mail if this is the destination town -------------
'   currentTown = ds:1F22 (0..11) ; mail job = S4(7) (S4 byte 0x0E ;
'   -1/&hFFFF = no job) ; the mail item = S2(9) (S2 byte 0x12).
'
'     IF S4(7) = currentTown AND locationType(ds:1E20) <> &h0C THEN
'         Delay &h13
'         PRINT "THANKS FOR THE LETTER DELIVERY"
'         payment   = INT( INT(RND(1) * 3) * 15 + 95 )    ' ds:2940/2944/2948
'         partyGold = partyGold + payment                 ' 32-bit add
'         S4(7)  = -1                                      ' clear the job
'         S2(9)  = 0                                       ' drop the letter
'         PRINT "HERE'S "; payment; " GOLD."
'         PressKeyToContinue
'         EXIT SUB
'     END IF
'   -- payment is 95, 110 or 125 gold (INT(RND*3) is 0/1/2 ; the 140 case
'      needs RND exactly 1.0).  Matches the hint-book's 95/110/125.  Since
'      the job only ever routes +-1 town (see MailDeliveryJob) the payout
'      is NOT distance-scaled -- it is just the flat random roll.
'
' ---- otherwise: buy rations -------------------------------------------
'     pricePerDay = INT( 13 - Charm/7 ) * 0.1            ' ds:2A06/2A0A/2A0E
'                   ' Charm 15 -> ~1.0 g/day ; Charm 30 -> ~0.8 g/day
'     PRINT "COST IS "; pricePerDay; " GOLD PER 'DAY'"
'     maxDays = MIN( 1000, INT(partyGold / pricePerDay) )
'     PRINT "MAXIMUM PURCHASE: "; maxDays; " DAYS"
'     n = PromptQuantity()                              ' clamped to maxDays
'     partyGold = partyGold - INT(n * pricePerDay)
'     food      = food + n                              ' runtime-only, not saved
'   The provisioner is also where MailDeliveryJob is offered (a chance
'   roll when you have no job pending).


' --------------------------------------------------------------------------
SUB MailDeliveryJob                                  ' asm: twndr.asm:2722 / loc_11905
' --------------------------------------------------------------------------
' Offered by the provisioner.  "WOULD YOU LIKE TO EARN SOME GOLD?" ->
' "HERE'S SOME MAIL TO DELIVER TO <town>".
'
'     DO
'         destTown = INT( INT(RND(1) * 3) + (currentTown - 1) )   ' ds:2940 = 3.0
'     LOOP WHILE destTown < 0 OR destTown > 10 OR destTown = currentTown
'     ' -> destTown is ALWAYS an adjacent town: currentTown +- 1
'     PRINT "HERE'S SOME MAIL TO DELIVER TO "; Town$(destTown)
'     S4(7) = destTown                          ' the pending job (byte 0x0E)
'     S2(9) = 1                                 ' hold the letter  (byte 0x12)
'     Item$(spare) = Town$(destTown) + " MAIL"  ' inventory display label (ds:1D66)
'     PressKeyToContinue
'
' Payment is credited on arrival -- see FoodShop's on-entry block above
' (INT(RND*3)*15 + 95  =  95 / 110 / 125 gold).


' --------------------------------------------------------------------------
SUB GuardAttack                                      ' asm: twndr.asm:2507 (guardAttack)
' --------------------------------------------------------------------------
' A town guard's turn (you get guards by fighting NPCs / robbing / etc.).
'
'   IF hitPoints >= 90 AND guardCount > 1 AND ds:2194 > 0 THEN
'       ArrestedByGuards            ' "THE GUARDS OVERWHELM YOU!" (jail, not death)
'   END IF
'   PRINT "ATTACKED BY GUARD!"
'   ' ---- to-hit ---------------------------------------- asm:loc_10FCA
'   IF RND(1) * 70 < Dexterity THEN PRINT " -- MISSED" : EXIT SUB   ' ds:285C 70
'   ' ---- damage --------------------------------------- asm:loc_1104B
'   raw = guardAtk * (RND(1) * 25 + 12)                  ' ds:21A2, ds:2870, ds:2874
'   armorTerm = 10 * armorId - 50                        ' with armour (id 9..13)
'   IF armorId = 0 THEN armorTerm = 30                   ' bare
'   dmg = INT( raw \ (armorTerm ^ 0.8 * Endurance ^ 0.8) ) + 3   ' ds:2878 0.8
'   hitPoints = hitPoints - dmg
'   PRINT " -- BLOW "; dmg
' -- defensive DENOMINATOR scaling (armour + Endurance), like the castle.


' --------------------------------------------------------------------------
SUB FightGuard                                        ' asm: twndr.asm:1407 (fightGuard)
' --------------------------------------------------------------------------
' Your attack on a guard.
'   IF attackMode = 0 THEN    ' weapon
'       base = (weaponId + 2) * Strength \ 8 + 4
'       dmg  = INT( base * (RND(1) + 0.5) )
'   ELSE                     ' spell -- as useMagicMenu, not re-derived here
'   END IF
'   guardHitPoints(cell in viewObjectArray) -= dmg
'   PRINT "GUARD STRUCK "; dmg; " H.P. BLOW"   /   "GUARD KILLED" at <= 0


' --------------------------------------------------------------------------
SUB StealGold / OfferGuardBribe / ArrestedByGuards    ' asm: twndr.asm 12x
' --------------------------------------------------------------------------
'   StealGold  ("Rob" a shop till): partyGold += S4(0) ; then S4(0) *= 0.8
'              (the till refills slower each time) ; a spendGold follow-up
'              (the fine if caught) -- exact split *partial*.
'   OfferGuardBribe: the demanded amount is `ds:216E` (set by the caller);
'              if partyGold >= it, partyGold -= it and the guard leaves.
'              Also takes / marks an item (S2 slot from ds:1AEE).  *partial*
'   ArrestedByGuards: "THE GUARDS OVERWHELM YOU!" -> jailPlayer (loses a
'              turn / some gold, teleport to the jail tile -- not a death).


' ==========================================================================
'  SOLID
'   * bank balance lives in ds:1AC2:1AC4 (the CHAR.DAT "experience" slot)
'   * interest per visit = MIN(1500, MIN(5000, balance) * daysElapsed \ 999)
'     (ds:2C3C = an 8-byte DOUBLE 999.0 ; daysElapsed = S4(36) <- ds:1AF4,
'      the terrain-wear / distance accumulator)
'   * moneylender = flat 50% loan (borrow 200 -> owe 300), computed once
'   * SpendGold(amount): partyGold -= amount   (shared by every vendor)
'   * deposit/withdraw move gold between partyGold and bankBalance 1:1
'   * SHOP BUY: prices are per-slot TOWN<n>.BSV data (not a formula); a
'     0.3 "not for sale" roll in townType 0x0C
'   * SHOP SELL: baseValue = INT( ((wid^1.05 + cond/2.8 + 2)^2.1)*4 - 10 )
'     (weapons); offer = INT( MIN(baseValue, baseValue*Charm^0.7/11) * 0.8 )
'   * GUARD -> player: miss when RND*70 < Dex; dmg = INT( guardAtk*(RND*25
'     + 12) \ (armorTerm^0.8 * End^0.8) ) + 3, armorTerm = 10*armorId - 50
'     (or 30 bare)
'   * player -> GUARD: dmg = INT( ((weaponId+2)*Str\8 + 4) * (RND + 0.5) )
'   * MAIL: job (S4(7), -1 = none) always routes to an ADJACENT town
'     (currentTown +- 1, re-rolled while <0 / >10 / == current); the letter
'     is S2(9).  PAID on entering the destination provisioner:
'     payment = INT( INT(RND*3)*15 + 95 )  =  95 / 110 / 125 gold
'     (partyGold += payment ; S4(7) = -1 ; S2(9) = 0) -- NOT distance-scaled
'   * FOOD: pricePerDay = INT(13 - Charm/7) * 0.1  (~1 g/day, less w/ Charm);
'     maxDays = MIN(1000, partyGold / pricePerDay) ; food is runtime-only
'
'  OPEN
'   * armour SELL polynomial exact assembly (consts 3.2/1.02/3.5/-6)
'   * bribe amount origin ; StealGold fine split ; robberyEvent
'   NOTE: twndr.idb has a local coerce of townServiceDispatch that reflows
'   the whole .asm on export -- the .asm is intentionally left un-updated;
'   findings above were read from the coerced idb.  foodShop was coerced +
'   dumped read-only via ida_scripts/dump_twndr_foodshop.py (-NoExport).

' ==========================================================================
'  TWNDR.EXE  --  town services                                       [v2]
'  reconstructed from twndr.asm ; see recovered/README.md for the model + tags
'
'  SUBs: SpendGold  (shared "pay N gold"), Bank,
'        WeaponArmorShop / FoodShop / MailDeliveryJob (shops + jobs),
'        GuardAttack / FightGuard / InitGuardCombat / RobCommand /
'        StealGold / OfferGuardBribe / ArrestedByGuards / JailRelease /
'        RobberyEvent  (the town-guard + crime system).
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
'   itemId = ds:1F04   condition = ds:1F06 (0..4)   -- both branches now exact
'
'   ' --- sub_11F51 : baseValue (-> ds:209E) ---
'   ' WEAPON (itemId 0..8) -- always computed:
'       baseValue = INT( ((itemId^1.05 + condition/2.8 + 2) ^ 2.1) * 4 - 10 )
'       ' consts: ds:2B4E 2.1  2B52 1.05  2B56 2.8  2B5A 2  2B5E 4  2B62 -10
'   ' ARMOUR (itemId > 8, i.e. 9..13) -- OVERWRITES baseValue:
'       baseValue = INT( (itemId^1.02 + condition/3.5 - 6) ^ 3.2 )
'       ' consts: ds:2B66 3.2 (outer exponent)  2B6A 1.02 (inner exponent)
'       '         2B6E 3.5 (condition divisor)  2B72 -6 (offset)
'       ' NOTE the armour branch has NO trailing "* m - k" -- it ends on the
'       ' outer power.  e.g. Studded hide (9) cond 2 -> ~82 ; Mythan plate
'       ' (13) cond 4 -> ~1060.
'
'   ' --- sub_1200B : haggle to the final offer (-> ds:1F12) ---
'   raw   = INT( baseValue * (Charm ^ 0.7) / 11 )          ' ds:2B76 0.7, 2B7A 11
'   raw   = MIN(raw, baseValue)                            ' never above base value
'   offer = INT( raw * 0.8 )                               ' ds:2878 0.8
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
SUB RobCommand                                       ' asm: twndr.asm:15b77 (robCommand)
' --------------------------------------------------------------------------
' The "ROB" command.  DETERMINISTIC -- no probabilistic "caught" roll.
' Dispatches on tileAhead (ds:1F02):
'
'   tile < &hC8           : "NOTHING TO ROB"  (tone, exit)
'   tile = &hD2  (THE MINT):
'       IF mintRobbed(ds:25A8) = 1 THEN PRINT "THE MINT IS EMPTY" : EXIT
'       PRINT "YOU FIND "; (S4(0)\99 + 1); " BAGS OF GOLD!"
'       -> StealGold                          ' grab the mint's stash S4(0)
'   tile = &hCF           : sub_16748 breaks the tile open ; contextMode = 1
'                           -> guard-fight setup
'   tile = &hCC or &hD0   : PRINT "THERE'S NOTHING TO REALLY GRAB HERE."
'   tile &hC8 / &hC9 / &hCB, or ELSE:  MerchantRefuses --
'       IF contextMode(ds:1F2A) > 0 OR heat(ds:20B0) > 0 THEN     ' allowed
'           ELSE case  : grab loose gold --
'               grabbed = INT( RND(1) * 10*10 + 150 )   ' ds:2C26=10, ds:2C2A=150
'               partyGold = partyGold + grabbed         ' -> 150..250 gold
'               PRINT "YOU GET "; grabbed; " GOLD."
'               robAttempts(ds:25A6) = robAttempts + 1
'               IF robAttempts > 1 THEN PRINT "NO ITEMS WITHIN REACH HERE."
'           &hC8..&hCB : just ENTER the shop normally
'       ELSE
'           PRINT "THE MERCHANT WON'T LET YOU ROB."     ' ds:2204 = 1, exit
'       END IF
'
' *** THE "CAUGHT" MECHANISM IS A TURN TIMER, NOT A ROLL. ***  Once a
' robbery is "in progress" (heat = ds:20B0 becomes > 0 -- set in a `db`
' blob on the first successful theft), doWalk ticks it every town-turn,
' and at heat > &h13 (20 turns):
'       sub_1046A ; PRINT "...DISCOVERED!!" ; contextMode = 1 ; heat = 0
'       PlayAlarm &hBB8   ' -> the guards attack (contextMode = 1 gates it)
' While heat > 0 the SPEAK command also answers "NOBODY ANSWERS".


' --------------------------------------------------------------------------
SUB StealGold                                        ' asm: twndr.asm:15cf0 (stealGold)
' --------------------------------------------------------------------------
' The mint-rob payoff ("<n> BAGS OF GOLD!").  The mint's stash is S4(0).
'     Delay &h1B
'     partyGold = partyGold + S4(0)             ' 32-bit ; grab the whole stash
'     PlayCoinFx 8                              ' ds:25B4 = 8 ; rt_FE56
'     S4(0) = INT( S4(0) * 0.8 )                ' refills to 80% (ds:2878)
'     SpendGold                                 ' gold-gauge repaint only
'     ds:25A8 = 1 : contextMode = 1             ' -> exit to the main loop
' rtm_FF22 POPS its operand (confirmed -- leglib_runtime.c), so by the
' `call spendGold` the FP stack is already spent : spendGold subtracts a
' base/zero slot -> ~0.  So the trailing SpendGold is just the gold-gauge
' repaint.  NET: partyGold += the FULL till ; S4(0) drops to 80%.


' --------------------------------------------------------------------------
SUB InitGuardCombat                                  ' asm: twndr.asm:10f37 (initGuardCombat)
' --------------------------------------------------------------------------
' Rolls the guard's hit points AND the bribe demand -- they are the SAME
' number, stored in ds:216E (guardHitPoints).
'     guardHitPoints = INT( (ds:1E22 - 7.5) * 22 * (RND(1) + 1) )
'         ' ds:283C = -7.5   ds:2840 = 22.0
'         ' ds:1E22 = the menu selection that triggered the guards; the
'         '   -7.5 shift is the same one CASDR uses for its spell index.


' --------------------------------------------------------------------------
SUB OfferGuardBribe                                  ' asm: twndr.asm:1549+ (offerGuardBribe)
' --------------------------------------------------------------------------
'   demand = ds:216E                             ' == the guard's rolled HP
'   IF partyGold >= demand THEN
'       partyGold = partyGold - demand           ' the guard pockets it and leaves
'   END IF
'   (also marks / takes an S2 item slot indexed by ds:1AEE)  *partial*


' --------------------------------------------------------------------------
SUB ArrestedByGuards / JailRelease                   ' asm: arrestedByGuards / jailRelease:15607
' --------------------------------------------------------------------------
'   ArrestedByGuards: "THE GUARDS OVERWHELM YOU!" -> sets the jailed flag
'       (ds:2194) and drops you in the cell.  Not a death.
'
'   JailRelease ("I'LL LET YOU OUT FOR A PRICE") -- the bail ladder:
'     IF partyGold > 149 THEN
'         partyGold = partyGold \ 2              ' half your gold
'         PRINT "IT HAS COST "; (partyGold\2); " GOLD TO GET OUT."
'     ELSEIF partyGold > 0 THEN
'         PRINT "I'VE TAKEN ALL YOUR GOLD."
'         partyGold = 0
'         ' then CONFISCATE the first non-empty weapon slot (S0 slots 7..0):
'         S0(slot) = 0                           ' item destroyed
'         weaponSlotCursor = 99 : weaponId = 0   ' unequipped
'         armourCursor     = 99 : armourId = 0
'     ELSE  ' totally broke, nothing to take
'         PRINT "I'VE GOTTEN 100 GOLD FROM THE LENDER - IN YOUR NAME."
'         S4(5) = S4(5) + 100                    ' a FORCED loan (debt slot)
'         S4(6) = INT( terrainWear(ds:1AF4) + 120 )   ' repayment deadline (ds:35F0)
'     END IF
'     ds:2194 = 0                                ' released
'     S4(31) = currentTown                       ' the jail is in this town


' --------------------------------------------------------------------------
SUB RobberyEvent                                     ' asm: twndr.asm:11cac (robberyEvent)
' --------------------------------------------------------------------------
' The " ROBBERY IN PROGRESS " event (from the ROB command on a shop).
' Walks the shop's stock -- viewObjectArray (ds:1C7C) slots 0..8, the same
' id/condition/price triples the buy counter uses (elem +0x4A/+0x5C/+0x6E)
' -- and for each occupied slot calls sub_11F51 to appraise it, writing the
' value back to the price cell, then lists "<n>. <item>" + value: the loot
' manifest.  `ds:1E20 = 0x0C` shops instead show "THE MERCHANT WON'T LET
' YOU ..." (merchantRefuses).  The actual item-grab / caught roll is in the
' ROB dispatch (robCommand), still a `db` blob.  *partial*


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
'   * SHOP SELL baseValue: weapons INT( ((wid^1.05 + cond/2.8 + 2)^2.1)*4
'     - 10 ) ; ARMOUR (id>8) INT( (id^1.02 + cond/3.5 - 6)^3.2 ) -- no
'     trailing scale/offset.  offer = INT( MIN(baseValue,
'     baseValue*Charm^0.7/11) * 0.8 )
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
'   * ROB (robCommand): deterministic, no roll.  Mint (tile &hD2) ->
'     StealGold (grab S4(0), *0.8) ; loose gold at a merchant ->
'     INT(RND*100 + 150) = 150..250 gold ; merchant refuses unless
'     contextMode > 0 or heat(ds:20B0) > 0
'   * CAUGHT = a TURN TIMER: heat (ds:20B0) ticks each town-turn once a
'     robbery is in progress ; at 20 turns -> "DISCOVERED!!" + alarm +
'     contextMode = 1 (guards attack).  Not a die roll.
'   * GUARD HP == BRIBE demand = ds:216E = INT( (ds:1E22 - 7.5) * 22 *
'     (RND + 1) ) ; OfferGuardBribe pays exactly that
'   * JAIL bail: partyGold > 149 -> lose HALF ; 1..149 -> lose ALL + one
'     equipped weapon confiscated ; broke+itemless -> forced 100-gold loan
'     (S4(5) += 100, S4(6) = INT(terrainWear + 120) deadline)
'
'  OPEN
'   * exactly where heat (ds:20B0) is first set to 1 (a `db` blob -- but
'     the timer mechanism above is fully traced)
'   * OfferGuardBribe's S2 item marking (ds:1AEE index)
'   NOTE: twndr.idb has a local coerce of townServiceDispatch that reflows
'   the whole .asm on export -- the .asm is intentionally left un-updated;
'   findings above were read from the coerced idb.  foodShop / stealGold /
'   robberyEvent / initGuardCombat / jailRelease were coerced + dumped
'   read-only via ida_scripts/dump_twndr_foodshop.py + dump_twndr_crime.py
'   (both -NoExport).

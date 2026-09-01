' ==========================================================================
'  TWNDR.EXE  --  town services                                       [v1]
'  reconstructed from twndr.asm ; see recovered/README.md for the model + tags
'
'  SUBs: SpendGold  (shared "pay N gold")
'        TownServiceDispatch  (the ~6 KB SELECT CASE on townServiceId --
'                              bank, item-grab, shop counters, guard)
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
    '   interest = MIN(1500, MIN(5000, bankBalance) * daysElapsed / K)
    '   K (ds:2C3C) is loaded at runtime -- ~1000 per the game guide
    '   ("about 1 gold per 1000 per day").
    capped   = bankBalance : IF capped > 5000 THEN capped = 5000          ' asm:4929-4941
    interest = capped * daysElapsed \ interestDivisor                     ' asm:4944-4970 '??
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
'  TownServiceDispatch -- the other cases (summary; full reconstruction TODO)
' --------------------------------------------------------------------------
'   case 0/0x0B  food shop         -- rations at the S5() price table
'   case 2       weapon shop       -- ShopConfirmBuy-style, price scales
'   case 4       armour shop
'   case 9       item grab / "YOU COULDN'T GRAB THE ..." / "YOU GET n GOLD."
'   case 0x0A    ??? (moneylender?)
'   guard fight  guardAttack/fightGuard -- guardHitPoints (ds:216E) as the
'                accumulator; pay-off prompt via SpendGold
'   mail routes  (guide: 95 / 110 / 125 gold) -- not yet located; may be a
'                townServiceDispatch stage or a per-town NPC


' ==========================================================================
'  SOLID
'   * bank balance lives in ds:1AC2:1AC4 (the CHAR.DAT "experience" slot)
'   * interest per visit = MIN(1500, MIN(5000, balance) * daysElapsed / K)
'   * SpendGold(amount): partyGold -= amount   (shared by every vendor)
'   * deposit/withdraw move gold between partyGold and bankBalance 1:1
'
'  OPEN
'   * interestDivisor K (ds:2C3C, runtime) -- trace it
'   * daysElapsed source (ds:1AF4 -> S4(36))
'   * the shop counters' price rolls, guard combat, mail routes, robberyEvent

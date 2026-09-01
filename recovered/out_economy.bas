' ==========================================================================
'  OUT.EXE  --  overworld shops / provisioning                        [v1]
'  reconstructed from out.asm ; see recovered/README.md for the model + tags
'
'  SUBs: ProvisionerShop (buyFood)   ShopConfirmBuy (shopBuy)
'        AddFoodDays / SpendFoodDays / DrawFoodGauge (rations bookkeeping)
'
'  Altitude: the arithmetic (prices, heal amounts, food) is reconstructed
'  faithfully; the menu / string plumbing is summarised.
' ==========================================================================
'
'  DGROUP vars:
'     partyGold 1AD2 (dword)   hitPoints 1ADA   characterLevel 1AE0
'     food 1ACE (via AddFoodDays)   maxHeal S4(19) = ds:1B96 elem 0x13
'     workInt 1F04    workInt2 2192    S0() ids ds:1B0C   S1() cond ds:1B68
'     ds:285E = 50   ds:2A68 = 40   ds:2A6C = 30   ds:2584 = 3.0
'     ds:280A = 2.0  ds:272A = 0.3  ds:2A64 = 0.2


' --------------------------------------------------------------------------
FUNCTION ShopItemPrice&()                             ' asm: out.asm:5364-5380 (in buyFood)
' --------------------------------------------------------------------------
' The overworld shop price roll.  Scales with the party's level -- a soft
' anti-farming / anti-hoarding curve.
    ShopItemPrice& = INT( RND(1) * characterLevel * 20.0 + 50.0 )
    '  ds:1AE0 * 20, + ds:285E.  Level 1: 50..70 ; level 5: 50..150.
END FUNCTION


' --------------------------------------------------------------------------
SUB ProvisionerShop                                  ' asm: out.asm:4713 (buyFood)
' --------------------------------------------------------------------------
' The potion / healer / gear stop ("DO YOU WANT TO BUY A POTION WORTH ...").
' Also the climbing-gear and raft vendor.

    IF RND(1) < 0.3 THEN                              ' ds:272A          ' asm:4714-4741
        FreeFindOnEntry                  ' rtm_FE38 -- a random freebie / event
        IF haveDroppedItem THEN GOTO SellGear         ' ds:1AEE          ' asm:4739
    END IF

    ' ---- healing potion --------------------------------------------- asm:4744-...
    '   offered when hitPoints is low (cmp 100) with probability ~ 0.2
    IF hitPoints < 100 AND RND(1) < HealOfferChance() THEN  ' ds:2A64   ' asm:4749-4780 '??
        IF maxHeal > hitPoints THEN                                     ' asm:4783-4791
            healRoll = INT( RND(1) * 40.0 + 30.0 )     ' ds:2A68, ds:2A6C ' asm:4794-4806
            hpNeeded = healRoll
            IF hitPoints + healRoll > maxHeal THEN _
                hpNeeded = maxHeal - hitPoints          ' don't overheal  ' asm:4807-4824
            healCost = INT( hpNeeded * (RND(1) + 2.0) / 3.0 )  ' ds:280A,2584 ' asm:4826-4841
            PRINT "DO YOU WANT TO BUY A POTION WORTH"; hpNeeded; _
                  " FOR"; healCost; " GOLD?"                             ' asm:4842-...
            IF YesNo() AND partyGold >= healCost THEN
                partyGold = partyGold - healCost
                hitPoints = hitPoints + hpNeeded
            END IF
        END IF
    END IF

SellGear:                                                                 ' loc_13081
    ' ---- climbing gear / raft ------------------------------------- asm:5364-...
    price& = ShopItemPrice&()                                             ' asm:5364-5380
    PRINT "WOULD YOU LIKE TO BUY A "; gearName$; " FOR"; price&; " GOLD?"
    IF YesNo() AND partyGold >= price& THEN
        partyGold = partyGold - price&
        GrantGear gearId                 ' sets the S2()/S4() flag, spawns
                                         ' the raft tile if it's a raft
    END IF

    ' ---- top up rations ------------------------------------------------
    '   the "buy food" option feeds AddFoodDays; each unit of food is a
    '   fixed price (from the S5 shop price table, char_dat.py) -- TODO.
END SUB


' --------------------------------------------------------------------------
SUB ShopConfirmBuy                                    ' asm: out.asm:5267 (shopBuy)
' --------------------------------------------------------------------------
' Generic "DO YOU WANT TO BUY A <item> FOR <price> GOLD?" for weapons /
' armour.  The price is already in ds:2192 (set by the caller); the item id
' is ds:1F04, its slot ds:231C, its condition ds:1F06.

    PRINT "DO YOU WANT TO BUY A "; Weapon$(ds1F04); " FOR"; ds2192; " GOLD?" ' asm:5268-5333
    IF ds1F04 <= 0 THEN EXIT SUB                                          ' asm:5337-5339
    ' (the Y/N + gold check happen in the caller; on confirm:)
    S0(equipSlot) = ds1F04             ' item id     ds:231C -> S0        ' asm:5342-5350
    S1(equipSlot) = ds1F06             ' condition   ds:1F06 -> S1        ' asm:5351-5356
    RefreshInventoryView              ' rtm_FE4F                          ' asm:5357
    ds2192 = 7                         ' transactionType = "bought"       ' asm:5358
END SUB


' ==========================================================================
'  SOLID
'   * shop item price  = INT( RND(1) * characterLevel * 20 + 50 )
'   * healer restores  = INT( RND(1) * 40 + 30 )  HP  (capped at maxHeal)
'   * heal cost        = INT( hpRestored * (RND(1) + 2) / 3 )  gold
'   * potion offered only when hitPoints < 100
'
'  OPEN
'   * the per-unit food price (S5 table index) and the AddFoodDays maths
'   * HealOfferChance / FreeFindOnEntry exact RND gates
'   * bank interest, mail-route costs (guide: 95/110/125) -- not located yet
'     in out.asm; may be in twndr

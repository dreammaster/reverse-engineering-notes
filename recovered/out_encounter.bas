' ==========================================================================
'  OUT.EXE  --  overworld encounter generation                       [v1]
'  reconstructed from out.asm ; see recovered/README.md for the model + tags
'
'  SUBs: CreatureApproach (the per-step encounter check)
'        BeginEncounterView (picks the creature type + count, sets the view)
'  RollEncounterMod lives in out_combat.bas.
' ==========================================================================
'
'  DGROUP vars:
'     contextMode 1F2A   facing 1E1E   characterLevel 1AE0
'     stepCounter 1AF8   enteredTileType 214A
'     stepScratch 208E (single; = enteredLocationId/20 after a move)
'     creatureIndex 2254   groupSize 2192   creaturesToFight 21FE
'     approachDir 1F2A (reused)   S2() = ds:1BC4
'   region params loaded per-map by loadOverworldData (0 in the EXE image):
'     ds:2092  weak-creature gate      ds:2096  creature-tier gate
'   constants:
'     ds:2790 0.06   ds:2794 3.2   ds:2798 0.83   ds:279C 4.0
'     ds:2482 0.5    ds:24E6 1.0   ds:2552 500


' --------------------------------------------------------------------------
SUB CreatureApproach                                 ' asm: out.asm:2943 (creatureApproach)
' --------------------------------------------------------------------------
' Called once per step from the main loop.  Decides whether a creature
' appears this step, and if so how many and from which direction.

    IF contextMode > 0 THEN EXIT SUB                  ' already busy       ' asm:2944
    IF facing = 0 OR workIntHi <> -9 THEN EXIT SUB    ' no real move        ' asm:2949-2967

    RandomizeStep enteredTileType + 1                 ' rtm_FC             ' asm:2974-2977

    ' ---- the encounter trigger ---------------------------------- asm:2985-3000
    '   an encounter happens this step when
    '       stepScratch  <=  RND(1) * (characterLevel + 9)
    '   stepScratch = enteredLocationId / 20  (set by DoMovement) -- so
    '   rougher terrain (higher id) is SAFER, higher level is more dangerous.
    IF stepScratch > RND(1) * (characterLevel + 9) THEN EXIT SUB           ' asm:2986-2999

    ' ---- rare special encounter -------------------------------- asm:3007-3067
    '   if you hold S2(15) AND stepCounter > 500 AND 2 <= level <= 7
    '   AND RND(1) < 0.06  ->  a scripted / tougher encounter (loc_14AA1)
    IF haveCompendium AND stepCounter > 500 AND characterLevel >= 2 _
       AND characterLevel <= 7 AND RND(1) < 0.06 THEN                      ' asm:3036-3067
        SpecialEncounter : EXIT SUB
    END IF

    contextMode = 10                                                      ' asm:3072

    ' ---- group size (capped at 7) ----------------------------- asm:3073-3092
    groupSize = (stepCounter MOD 2500) \ SomeScale + 1     ' rt_14, TODO   '??
    IF groupSize > 7 THEN groupSize = 7

    ' ---- how many actually engage ---------------------------- asm:3095-3119
    r = RND(1)
    creaturesToFight = INT( (r ^ (3.2 * r + 0.83)) * groupSize + 1 )       ' asm:3095-3119
    '  ds:2794 3.2 ; ds:2798 0.83 .  r^(0.83..4) is skewed low -> usually
    '  1..3 engage even for a big group.

    ' ---- approach direction --------------------------------- asm:3133-3146
    IF RND(1) <= 0.5 THEN                             ' ds:2482            ' asm:3120-3129
        approachDir = INT( RND(1) * 4.0 + 1.0 )       ' ds:279C 4.0        ' asm:3134-3146
    END IF

    PRINT "UNKNOWN CREATURE"; PLURAL$(creaturesToFight)                    ' asm:3149-3176
    PRINT "APPROACHING FROM THE "; Direction$(approachDir)                 ' asm:3179+
    BeginEncounterView
END SUB


' --------------------------------------------------------------------------
SUB BeginEncounterView                               ' asm: out.asm:3772 (beginEncounterView)
' --------------------------------------------------------------------------
' Also called from quitOrTalk (re-enter combat after a parley).

    IF contextMode < 5 THEN backupContextMode = contextMode               ' asm:3774-3781
    contextMode = 11                                                      ' asm:3784

    ' ---- pick (base, range) for the creature-type roll ----------- asm:3785-3861
    IF RND(1) >= regionWeakGate THEN                  ' ds:2092            ' asm:3786-3803
        base = 3 : range = 4                          ' mid-tier: 3..6
        RollCreaturePosition
    ELSE
        base = 3 : range = 4                          ' loc_1238C default
        IF RND(1) < regionTierGate THEN               ' ds:2096            ' asm:3823-3841
            IF RND(1) < 0.5 THEN base = 0 : range = 3 ' weak: 0..2 (pixie/strider/farmer) asm:3846-3861
        END IF
    END IF

    ' ---- the creature type --------------------------------- asm:3863-3895
    creatureIndex = INT( RND(1) * range + base )                          ' asm:3865-3895
    '  then A1(creatureIndex) / A2 / A3 / names are all indexed by this.
    '  creatureHP   = A1(creatureIndex) MOD 256    (out_combat.bas)
    '  creatureWeak = A2(creatureIndex) MOD 256

    RollEncounterMod                                  ' out_combat.bas    ' asm: -> rollCreatureStats
    ' ... lay out the viewObjectArray slots, draw the encounter view ...
END SUB


' ==========================================================================
'  SOLID
'   * encounter trigger : stepScratch <= RND(1) * (characterLevel + 9),
'     stepScratch = enteredLocationId / 20
'   * creaturesToFight  : INT( r^(3.2r + 0.83) * groupSize + 1 )
'   * approach direction: INT( RND(1)*4 + 1 )   (50% of the time)
'   * creature type     : INT( RND(1) * range + base ), (base,range) in
'                         {(3,4) mid, (0,3) weak, ...}
'   * rare special      : S2(15) + stepCounter>500 + level 2..7 + RND<0.06
'
'  OPEN
'   * groupSize's `rt_14` scaling (the 2500 divisor)
'   * regionWeakGate / regionTierGate (ds:2092 / ds:2096) -- per-map,
'     loaded by loadOverworldData; dump from a running game
'   * whether a higher-tier (base >= 7) branch exists for late game

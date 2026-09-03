' ==========================================================================
'  MENU.EXE / SAVER.EXE  --  roster, new game, save                    [v1]
'  reconstructed from menu.asm / saver.asm ; see recovered/README.md
'
'  MENU.EXE is the launcher: title screen, main menu, character roster,
'  new-character creation, and the "poor peasant on Tarmalon" intro, then
'  it chains to OUT.EXE.  SAVER.EXE is a tiny overlay the play modules call
'  to write the roster back to CHAR.DAT.  Neither has combat / economy
'  logic; what matters for a port is the STARTING STATE and the save I/O.
'
'  Resident character state lives in LEGLIB DGROUP ds:1AC0..1B08 (scalars)
'  + 7 BASIC arrays; it survives module chaining and is the thing saved.
'  Slot index = ds:1B0A (0..8).   File = CHAR.DAT (see docs/file-formats.md).
' ==========================================================================


' --------------------------------------------------------------------------
'  New-character starting state
' --------------------------------------------------------------------------
' `startNewGameMenu` -> `promptNewCharacterName` (<= 14 letters) then copies
' the 382-byte NEW-CHARACTER TEMPLATE out of LEGACY.DAT (bytes 0xA03..end,
' decoders/legacy_dat.py) into the chosen CHAR.DAT slot, then
' `playIntroAndLaunchGame` shows the intro and chains to OUT.EXE.
'
' Template values (verified against LEGACY.DAT):
'     name            "empty" placeholder -> replaced by the typed name
'     Strength        15          ' rec +0x56  ds:1B08
'     Dexterity       15          ' rec +0x0E  ds:1AC0
'     Endurance       15          ' rec +0x1A  ds:1ACC
'     Intelligence    15          ' rec +0x3E  ds:1AF0
'     Charm           15          ' rec +0x2C  ds:1ADE
'     hitPoints       200         ' rec +0x28  ds:1ADA   (= level-1 max)
'     level           1
'     partyGold       0           ' rec +0x22  ds:1AD2   (a "poor peasant")
'     bankBalance     0
'     equipped armour Studded hide (id 9)     ' rec +0x3A  ds:1AEC
'     equipped weapon none (slot 99)          ' rec +0x4A  ds:1AFC
'     droppedItemId   0xFF (none)             ' ds:1AEE
'     first-person pos, quest flags, S2 item bitmap, spell counts : all 0
'     S5 shop price table : the 41-entry price list baked into the template
'                           (400,350,350, ... 1200, 300,420, ... 2000)
'
' So: all five attributes start at 15, 200 HP, no gold, weakest armour,
' bare hands.  Everything grows only through play (museum caretaker for
' level, quests / Stones of Wisdom / training school for attributes).


' --------------------------------------------------------------------------
'  MENU flow (menu.asm)
' --------------------------------------------------------------------------
'   menuStartup      loads LEGACY.DAT into resident DGROUP (font + strings +
'                    tables + the new-char template), 3-copyright splash
'   mainMenuLoop     SELECT CASE: Start / Restart / Instructions / Credits /
'                    Title / Erase / Quit  (self-looping)
'   showCharacterRoster / showEmptyCharacterSlots / promptCharacterNumber
'                    -- read CHAR.DAT header + 9 records, list names
'   readCharDat / readLegacyDat   -- random-access GET of a record / the
'                    master table
'   eraseCharacterMenu -> writeCharDat / updateCharDatEntry
'                    -- rewrite a slot to the "empty" template
'   startNewGameMenu -> promptNewCharacterName -> playIntroAndLaunchGame
'                    -- the intro crawl (~13 lines, ds:2C50..2DDE), then
'                       chains OUT.EXE
'   showTitleScreen -> loadTitleImage (TITLE.GLB/GMP -> B800), scroll,
'                    playMusicTick (PLAY MML on the speaker)


' --------------------------------------------------------------------------
'  SAVER.EXE  --  saveRosterToDisk (saver.asm:656)
' --------------------------------------------------------------------------
'   "DO YOU WANT TO SAVE THE GAME NOW IN PROGRESS?"  -> "SAVING TO DISK"
'   OPEN "CHAR.DAT" AS #1 LEN = 382                    ' rt_FE63 resolves disk
'   FIELD #1, ...                                      ' rt_CF
'   LSET record$ = MKI$(...) + ... for ds:1AC0..1B08 + the 7 arrays  ' rt_CD/73
'   PUT #1, currentSlot                                ' rt_AB ; slot = ds:1B0A
'   -- handles "wrong disk in drive" retry (chainBackOrQuit)
'   -- then chains back to the calling play module
'
'  The scalar write is a byte-image of ds:1AC0..1B08 via rtm_FE35 (peek one
'  word) in a loop from ds:1B08 down to ds:1AC0 -- i.e. the save format is
'  literally "dump the resident DGROUP window".  A port keeps its character
'  struct in the same field order and this stays a trivial serialise.


' ==========================================================================
'  SOLID
'   * starting stats: STR/DEX/END/INT/CHARM all 15, HP 200, level 1,
'     gold 0, Studded hide armour, bare hands -- from the LEGACY.DAT
'     new-character template (bytes 0xA03..)
'   * save = image of resident DGROUP ds:1AC0..1B08 + 7 arrays -> CHAR.DAT
'     record ds:1B0A ; see docs/file-formats.md for the field map
'
'  OPEN
'   * a couple of template words (rec +0x10 = 17008, rec +0xC8.. = 1500/
'     3099/31058) are not yet identified -- likely S4 defaults / RNG seed
'   * exact intro text is in ds:2C50..2DDE (not transcribed here)
' ==========================================================================

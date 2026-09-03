' ==========================================================================
'  CELDRV.EXE / CONFIGUR.EXE  --  cinematic + setup utilities           [v1]
'  see recovered/README.md
'
'  Neither has game mechanics.  Documented here only so every shipped
'  executable is accounted for.
' ==========================================================================


' --------------------------------------------------------------------------
'  CELDRV.EXE  --  the endgame ("AGAINST ALL ODDS!") cinematic
' --------------------------------------------------------------------------
'  Reached from CASDR.EXE after the final boss (not a museum exhibit).
'    celdrv_entry:
'      FOR celBank = 0 TO 4                          ' ON celBank GOSUB (rt_FC)
'          BLOAD  "CEL0.BSV" / "CEL1.BSV" / "CEL2.BSV" / "DIS9.BSV" /
'                 "CEL3.BSV"   into the image banks           ' rt_FE07
'          relocate that bank's frame-offset table
'      NEXT
'      show the "AGAINST ALL ODDS!" title
'      scrollStoryText  -- the victory narration, 999 stored lines, "<N>"
'                          placeholders replaced by the hero's name, paced
'                          against PLAY-MML music (serviceMusic / delayWithMusic)
'      fall into runCreditsCrawl -- "IBM VERSION BY" / "ARTWORK BY" / ...
'    then halts (end of game).
'  celAnimStep / blitCelFrame -- the per-panel CGA animation; no logic.


' --------------------------------------------------------------------------
'  CONFIGUR.EXE  --  disk / drive setup
' --------------------------------------------------------------------------
'  NOT compiled BASIC -- a small C program (has fprint / iprint / putbuf /
'  putsign in the disassembly).  Reads DRCONFIG.DAT (decoders/drconfig_dat.py),
'  lets the user set the drive letter for each of the 4 game disks + the
'  save disk, writes the letters back into the record padding.  Does not
'  touch the disk-manifest structure itself.  No relevance to the engine
'  beyond: the LEGLIB file loader consults the same DRCONFIG.DAT to resolve
'  which drive a named file is on (rtm_FE63).


' ==========================================================================
'  Other executables, for completeness:
'    LEGLIB.EXE   -> recovered/leglib_runtime.c + leglib.bas
'    MENU / SAVER -> recovered/menu_saver.bas
'    OUT          -> recovered/out_*.bas
'    DUN          -> recovered/dun_*.bas
'    TWNDR        -> recovered/twndr_services.bas
'    CASDR        -> recovered/casdr_castle.bas
'    MUS          -> recovered/mus_exhibits.bas
'    GMB1 / GMB2  -> recovered/gmb_casino.bas
'    STDRV        -> recovered/stdrv_dice.bas   (Stones of Wisdom, INT)
'    SDEFENDR     -> recovered/sdefendr_training.bas  (training school, END/DEX)
'    CELDRV / CONFIGUR -> this file
' ==========================================================================

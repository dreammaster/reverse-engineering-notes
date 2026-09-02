Begin3
Title:         Wizardry_iii_SourceCode
Author:        Thomas W. Ewers
Version:       
Entered-date:  March 27, 2012  
Description:   
  Pascal (and assembler) source code files and generated listings for
  Wizardry3, Legacy of Llylgamyn.

  I have re-engineered Pascal source code (and assembler code) from the
  Wizardry3 program.  The original executable files can be found on:

    Wizardry_iii_19830820_1.dsk

  In this package:

    /Wizardry.code
       Wiz3A.DSK
       Wiz3B.DSK
       Wiz3C.DSK
       Wiz3D.DSK
       WizAssm6502.txt
       WizardryListing.txt
                      
    /WizUtil.code
       Wiz3U.DSK
       WizUtilCopyProt6502.txt
       WizUtilListing.txt

  The Pascal code found on the enclosed disks is NOT the code that the original
  authors wrote.  However, when you compile (and assemble and link) these
  files, the EXACT executable code in Wizardry.code and WizUtil.code are
  created.
    
  Although I wrote EXACT in the previous sentence, that refers to the 
  actual P-code generated and placed into the executable files.  There are
  two reasons why a binary comparison between the original Wizardry.code and
  this compiled Wizardry.code will not match 100%:

    1. When segments for a file are compiled, they start on even Pascal
       "block" boundaries.  The gap between the end of one segment 
       and the start of the next segment is not "zero filled" by the
       Pascal compiler or linker.  Whatever garbage was on the diskette
       during the compilation is therefore part of the ".code" file.

    2. Segment 1, the main procedure for Wizardry.code, is physically at the
       beginning of Wizardry.code on the original diskette.  When my files
       are compiled, it appears at the end.

       Note, segment 1 for WizUtil appears at the end of WizUtil.code.

  The source code can be compiled using Apple Pascal 1.1 on an Apple][
  computer (or simulator) that has 4 diskette drives.

  To compile WIZUTIL.CODE:

    1. Assemble WIZU:COPYPROT.TEXT (creates COPYPROT.CODE)
    2. Compile  WIZU:WIZUTIL.TEXT  (creates unlinked WIZUTIL.CODE)
    3. Link     WIZU:WIZUTIL.CODE  (creates WIZUTIL.CODE)

  To compile WIZARDRY.CODE:

    1. Assemble WIZD:PRPICCH.TEXT  (creates PRPICCH.CODE)
    2. Assemble WIZD:DRAWSCR.TEXT  (creates DRAWSCR.CODE)
    3. Assemble WIZD:DRAWLINE.TEXT (creates DRAWLINE.CODE)
    4. Assemble WIZD:RANDOM.TEXT   (creates RANDOM.CODE)
    5. Assemble WIZD:CHKKEYBD.TEXT (creates CHKKEYBD.CODE)
    6. Assemble WIZD:SCRNDATA.TEXT (creates SCRNDATA.CODE)
    7. Compile  WIZA:WIZARDRY.TEXT (creates unlinked WIZARDRY.CODE)
    8. Link     WIZA:WIZARDRY.CODE (creates WIZARDRY.CODE)
    

Keywords:      Wizardry, Wizardry3, Legacy of Llylgamyn, SIR-TECH 
Uploader:      ewerst@msn.com   
Primary-site:  ftp://ftp.apple.asimov.net
                 /pub/apple_II/images/games/rpg/wizardry/wizardry_III/

               Wizardry_iii_SourceCode.zip

                   WiziiiSourceCode.ASM.txt

                   /Wizardry.code
                      Wiz3A.DSK
                      Wiz3B.DSK
                      Wiz3C.DSK
                      Wiz3D.DSK
                      WizAssm6502.txt
                      WizardryListing.txt
                      
                   /WizUtil.code
                      Wiz3U.DSK
                      WizUtilCopyProt6502.txt
                      WizUtilListing.txt


Platform:      Apple][ 
End
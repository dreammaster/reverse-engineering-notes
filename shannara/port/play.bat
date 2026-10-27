@echo off
rem Starts the natively recompiled game. Needs (all git-ignored, from your own copy of the game):
rem   game\             the retail game files
rem   game_out\pics     PICS.UHA extracted (SHAN*.PIC)
rem   game_out\flics\FLICS   the FLICS*.UHA archives extracted (cutscenes)
rem   game_out\vocs     Vocs.zip extracted (speech and sound effects as numbered MP3 files)
rem HOST_NOSOUND=1 turns all sound off.
cd /d "%~dp0.."
set HOST_EXTRA_DIRS=game_out\flics\FLICS;game_out\vocs
port\build\shannara_win.exe game\SHANNARA.EXE game game_out\pics game_out\save

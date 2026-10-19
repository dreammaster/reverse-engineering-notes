@echo off
rem Starts the natively recompiled game (needs game\ = the retail game files, game_out\pics = extracted PICS.UHA, game_out\flics\FLICS = extracted FLICS archives)
cd /d "%~dp0.."
set HOST_EXTRA_DIRS=game_out\flics\FLICS
port\build\shannara_win.exe game\SHANNARA.EXE game game_out\pics game_out\save

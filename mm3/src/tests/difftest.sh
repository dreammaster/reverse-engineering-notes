#!/bin/bash
# every readable replacement against its translated original on the live game state (see game_diff.c); run from mm3/src after make mm3game
SDL_VIDEODRIVER=dummy ./mm3game ../data --headless --difftest "$@" 2>&1 | grep -v getFiles

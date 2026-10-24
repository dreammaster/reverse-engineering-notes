"""Portable helper for the Makefile (so it works from cmd.exe, PowerShell and MSYS shells alike).

    python tools/u5tool.py mkdir DIR...
    python tools/u5tool.py rmtree DIR...
    python tools/u5tool.py rundir GAME_DIR RUN_DIR      create/refresh the working directory for the game
    python tools/u5tool.py exec RUN_DIR EXE [ARGS...]   run EXE with RUN_DIR as the working directory
"""
import subprocess
import os
import shutil
import sys

SAVE_FILES = ["BRIT.OOL", "UNDER.OOL", "SAVED.OOL", "SAVED.GAM"]


def rundir(game, run):
    if not os.path.isfile(os.path.join(game, "TITLE.BIT")):
        sys.exit("GAME_DIR=%s does not contain the Ultima V data files" % game)
    os.makedirs(os.path.join(run, "SAVEGAME"), exist_ok=True)
    os.makedirs(os.path.join(run, "U4SAVE"), exist_ok=True)
    for name in os.listdir(game):
        src = os.path.join(game, name)
        dst = os.path.join(run, name)
        if os.path.isfile(src) and not os.path.exists(dst):
            shutil.copy2(src, dst)
    # saves live in SAVEGAME/ (see FILE_ReadFile); seed them from the pristine files once
    for name in SAVE_FILES:
        dst = os.path.join(run, "SAVEGAME", name)
        if not os.path.exists(dst):
            shutil.copy2(os.path.join(game, name), dst)


def main():
    cmd, args = sys.argv[1], sys.argv[2:]
    if cmd == "mkdir":
        for d in args:
            os.makedirs(d, exist_ok=True)
    elif cmd == "rmtree":
        for d in args:
            shutil.rmtree(d, ignore_errors=True)
    elif cmd == "rundir":
        rundir(*args)
    elif cmd == "exec":
        run, exe = args[0], os.path.abspath(args[1])
        sys.exit(subprocess.call([exe] + args[2:], cwd=run))
    else:
        sys.exit("unknown command " + cmd)


main()

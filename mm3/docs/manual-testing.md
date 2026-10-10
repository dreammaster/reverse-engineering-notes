# Manual test plan for the recompiled game (`mm3game`)

Everything below could **not** be checked in the cloud sandbox (it only has dummy SDL video/audio drivers), so it needs a real
machine with a window, a mouse, a keyboard and speakers.  Branch: `might_and_magic`, latest commits at the time of writing:
readable rewrites of `setSpeedTable`, `stopAttack`, `generateItem`, spell price/points, `subPartyTime`, `GiveBankInterest` ...

## 0. Build and automated checks (a few minutes)
```
cd mm3
make derive            # builds src/DGROUP.BIN and IMAGE.BIN from data/MM3.EXE (git-ignored)
cd src
make mm3game           # needs SDL2 dev headers and libm
bash tests/difftest.sh # every readable routine vs the translated original (expect "0 mismatches" on every line)
bash tests/shadowcheck.sh; bash tests/combatwin.sh; bash tests/gamefuzz2.sh 30   # silent/OK = pass
```
Report any line that says MISMATCH, any `rc=` line from the fuzz scripts, or a build error.

## 1. Start-up  (`./mm3game ../data`)
- [ ] Window opens, resizable, **4:3 shape** (320x200 picture stretched like a DOS monitor); `MM3_SQUARE=1` gives square pixels.
- [ ] Alt+Enter and F11 toggle full screen and back; the picture stays sharp and keeps its aspect.
- [ ] With `--intro`: intro animation (starfield, title) runs at a believable speed, not too fast/slow; no flicker or garbage.
- [ ] Roster/save-slot menu appears; with no save a new game from MM3.CUR is created (`./mm3-saves/SAVE00.MM3`).
- [ ] `data/MM3.CUR` is **not** modified (check `git status` afterwards).

## 2. Mouse and keyboard
- [ ] The mouse cursor appears, follows the pointer, and clicks hit the right HUD buttons (also when the window is resized and in full screen).
- [ ] Arrow keys move/turn, Space/Enter/Esc behave as in DOS; keys A/B/C/F/I/Q/R/U work in combat.
- [ ] The boss key (original hotkey) does nothing harmful.
- [ ] Nothing busy-loops: CPU use is reasonable while idle in a menu.

## 3. Exploration and 3D view
- [ ] Walk Sorpigal (town) and a few other maps (indoor and outdoor, e.g. `--at 1,2,5,2`); walls, doors, monsters, objects and sprites look right, no missing pieces.
- [ ] Turning, strafing, bumping into walls; the automap/overhead map; day/night and light changes (Light spell, torches).
- [ ] Monsters move towards you and shoot (monster movement and ranged attacks are now readable C - watch for monsters that stand still, jump, or walk through walls).

## 4. Town, shops, events
- [ ] Inn, tavern, temple, training, guild (buy spells: prices should look sensible; half casters pay double), blacksmith (stock appears), bank (deposit/withdraw; **interest** grows gold/gems by 1% when the bank interest runs).
- [ ] Chests/treasure: items generated look plausible (names, enchantments, charges on spell items).  `generateItem` is new; look for crashes or empty/odd item names.
- [ ] Message boxes, input boxes (typing a name), confirm dialogs, scrolling text.

## 5. Combat  (readable `setSpeedTable`, `stopAttack`, `hitMonster`, damage, saving throws ...)
- [ ] Fight several groups; the turn order is sensible (fast characters/monsters first) and the highlighted character advances correctly each round.
- [ ] Attack, cast, block, run, quick fight (Q/F), use item; damage numbers and kills look right; party death handling; experience and treasure after the fight.
- [ ] Ranged attacks / shooting down corridors work only when the line is clear (walls block them).

## 6. Characters
- [ ] Create a character (all classes), stat roll, class availability list; character sheet, inventory, equip/unequip (class restrictions message), item names.
- [ ] Level up / training cost, experience display, spell lists, casting and SP costs (compare SP spent with the spell list).
- [ ] Party clock/date in the info screens after resting and after time-changing spells.

## 7. Saving and loading
- [ ] Control panel -> save, quit, restart, load: party position, gold, items and map state are restored.
- [ ] Saves go to `./mm3-saves` (or `MM3_SAVE_DIR`), never into `data/`.

## 8. Sound  (new: the original ADLIB.DRV runs in an 8086 interpreter feeding a software OPL2)
- [ ] Intro music plays, in tune and at the right tempo; town and dungeon songs; sound effects (footsteps, attacks, spells).
- [ ] No crackling/underruns at normal window size or in full screen; no stuck notes after changing songs or leaving a menu.
- [ ] Timbres: the OPL emulation is approximate - note anything that sounds clearly wrong (missing channel, wrong instrument, too quiet/loud, drum parts missing).
- [ ] Options menu sound/music toggles actually mute them.
- [ ] Optional: `MM3_WAV=out.wav ./mm3game ../data --intro` dumps the audio so you can send or compare it.

## 9. Known gaps (do not report as bugs)
Digital-sample drivers, PC-speaker/Roland music, rhythm-mode drums, the screen-transition effect (`vdrv_00`: screens just appear), the
original copy-protection/`_main` start-up (replaced by `game_main.c`), cycle-exact OPL.

## What to send back
For each failure: map/x/y/facing (or screen), what you did, what you expected, the console output, and a screenshot
(`--shot f.bmp` or `MM3_SHOT_EVERY=N`).  For a crash, run with `MM3_TRACE=1` and attach the last lines.  If a readable routine is
suspected, run `MM3_SHADOW=1 ./mm3game ../data` - it re-runs each rewritten routine through the translated original and prints a
mismatch with the routine name.

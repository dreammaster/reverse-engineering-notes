# Verification against the original code

`xanth/engine/host/host_test.exe` runs the original program. Everything it needs is scripted from the command line, so a scenario is a short text:

* `HOST_INPUT` keys and `HOST_MOUSE` pointer positions / buttons at given label counts, `play.sh` as the convenient wrapper (see `engine/host/README.md`);
* `HOST_DUMP` writes the 64 KB data segment at given label counts (game state: object records, flags, score, time);
* `HOST_SHOT` / `HOST_SHOT_TEXT` write the 320x200 screen (palette indices and the DAC palette), `HOST_DRAW=1` makes the text appear on it;
* the recorded engine calls (`Font_WriteMessage`, `realsound_play`, ...) give the sequence of messages and sounds.

The new engine takes the same scenario (`--script`) and produces the same artefacts, which `tools/compare_*` diff:

| artefact | compared | tolerance |
|---|---|---|
| state: object records, the variables of `reversing/analysis/xanth_flags.tsv`, the 205 save regions | byte for byte after every command | none |
| messages and sound calls | in order | none |
| screen | palette index per pixel after every command | none for pictures, text and interface; a documented list of exceptions (for example the mouse cursor position) |
| timing | not compared: the oracle's time advances per executed label, the engine's per frame | |

Scenarios: the scripted opening (switch, kitchen, telephone, letter), one scenario per room entry, the `scan` of every handler (objects x phases x verbs), and the
walkthrough (https://www.walkthroughking.com/text/companionsofxanth.aspx) cut into segments that each end in a state dump.

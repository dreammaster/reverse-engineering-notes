# RESOLVED: sub_409B1A = post_script_cleanup

**Update:** resolved by reading the actual disassembled instructions
directly in the exported `.asm` rather than relying on string/callgraph
heuristics alone. `sub_409B1A` is `post_script_cleanup`
(`Engine/AC.CPP:3093`). Its opening sequence decrements a global matching
`num_scripts`, indexes a 0x6C-byte-element array matching `scripts[]`
(`ExecutingScript`), and decrements the already-named global `inside_script`
— an exact match to `post_script_cleanup`'s first few lines. Further down it
directly calls `RestoreGameDialog`, `restore_game_data` +
`Display("Unable to load game (error: %s).", ...)` + `RestartGame`, and
`do_conversation` — matching `post_script_cleanup`'s
`ePSARestoreGameDialog` / `ePSARestoreGame` / `ePSARestartGame` /
`ePSARunDialog` switch cases exactly (`AC.CPP:3116-3157`).

The twist: the `restore_game_data`+`Display`+`RestartGame` sequence is
`load_game_and_print_error`'s logic (`AC.CPP:2988`) **inlined directly**
into `post_script_cleanup` in this 2002 build rather than called as a
separate function. That inlining is exactly what produced the original
misleading single-string match (see below) and made the mutual-recursion
callgraph look contradictory — it wasn't; both call directions are real,
just not through the function first guessed.

**Methodology takeaway:** when string + callgraph evidence disagree, it's
worth spending the time to read the actual instructions in the exported
`.asm` (available directly via `Read`/`Grep` on `rob_blanc_1.asm`, no IDA
needed) rather than stopping at "inconclusive." Global-variable names IDA
already had applied (`inside_script`, `num_scripts`-equivalent) turned out
to be the deciding evidence, not the string.

---

*Original write-up below, kept for reference on how the puzzle looked before
resolution.*

Not matched yet — flagging the contradiction rather than guessing.

**String evidence** points at `load_game_and_print_error` (`Engine/AC.CPP:2988`):
the only reference to `"Unable to load game (error: %s)."` (source line 2995,
inside that function) in the entire disassembly is `sub_409B1A`'s single
`offset aUnableToLoadGa_0` operand.

**But the callgraph contradicts that.** `sub_409B1A`:
- is called from `run_script_function_if_exist+95` (single caller)
- itself calls `run_script_function_if_exist` back, at its own `+1D0`

i.e. genuine mutual recursion between `sub_409B1A` and
`run_script_function_if_exist`. `load_game_and_print_error`'s actual source
body (`load_game(...)` then `Display(...)` on error) has no plausible path
back into `run_script_function_if_exist` — it's a small, self-contained
save-game-loading helper, not something the script-function dispatcher would
call into or be called from.

**Hypotheses, untested:**
1. The string match is coincidental in a way that isn't obvious from source
   grep alone — e.g. `Display()` (called at AC.CPP:2995) itself, or something
   it calls, eventually re-enters the script dispatcher (a plugin hook via
   `RunPluginHooks`? — `_displayspeech`/translation code paths do call into
   plugin callbacks elsewhere in this codebase) and that path was compiled in
   a way that folds back to this address. Needs actual instruction-level
   reading to confirm or rule out.
2. `sub_409B1A` is a different, nearby function and the string reference I'm
   reading is misattributed by `build_leads.py`'s proc/endp tracking (e.g. a
   mislabeled boundary). Worth double-checking function bounds directly in
   IDA rather than via the exported `.asm` text scan.
3. `sub_409B1A` is actually `quit_with_script_error` (called from
   `run_script_function_if_exist` at source line 3289) or a fatal-error path
   that unwinds unusually (note: this binary links `longjmpc.obj` per
   `refmap_symbols.json` — non-local control flow could confuse IDA's static
   callgraph if a `jmp` was used somewhere IDA reads as a call, or vice
   versa).

Left as `new_name: null` in `matches.json` (not yet added) pending someone
reading `sub_409B1A`'s actual instructions in IDA rather than relying on the
exported text alone.

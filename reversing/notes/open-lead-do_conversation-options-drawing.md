# Open lead: sub_41D7F7 (dialog options drawing)

Called twice from `do_conversation` (`+599`, `+5E2`) only. Takes 12 dword
parameters. Loops over an item count (compares an incrementing counter
against `arg_8`), reads `playerchar`'s speech-color field (`playerchar+0x20`,
masked/shifted — matches the high-byte color-encoding pattern used
elsewhere), and calls the already-matched `get_col8_lookup`. Strong
structural signature for "draw the dialog options list, one line per
option, using the character's speech color."

No 1:1 named counterpart found in the 2011 source. Most likely explanation,
consistent with the pattern already seen twice this project (`sub_42B394`/
`cc_run_code`+`call_function`; `do_conversation`/`show_dialog_options`):
this was a distinct helper in the 2002 codebase that got fully inlined into
`show_dialog_options`'s body by 2011 (no trace of a separate function
remains to match against). Given `show_dialog_options` itself is already
believed inlined into `do_conversation` in this build (see the
`do_conversation` match evidence in `matches.json`), this may be a third
layer of the same collapsing: 2002 `do_conversation` → calls a dedicated
options-drawing routine → 2011 collapses everything into one `do_conversation`
body with no internal function boundaries left to compare against.

Not pursued further — flagging as a case where source-comparison hits a
structural dead end rather than a knowledge gap. If it matters later (e.g.
for C reconstruction of the dialog system), the 12-parameter signature and
loop body are worth transcribing directly from the `.asm` rather than
hunting for a source counterpart that may not exist anymore.

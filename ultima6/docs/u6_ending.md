# Ultima VI — the ending (`END.EXE`)

`GAME.EXE` starts the ending with `start_ending` (`116CE`): it works out how long the game has been played — the current game date
(`game_day` `4049E` 1-28, `game_month` `4049F` 1-13, `game_year` `404A0`; they are bytes `+0x1BF5`, `+0x1BF6` and the word `+0x1BF7` of
the `objlist` globals) minus the date a new game starts on (**day 4, month 7, year 161**), borrowing 28 days or 13 months where a
field goes negative — formats years, months and days as decimal strings and `execl`s `END.EXE` with them.

`END.EXE` (`u6/u6_end`) therefore insists on `argc == 5` in `_main`: `argv[1]` is the hardware configuration string `GAME.EXE`'s `make_config_string`
(`1166B`) builds from the current settings (a video-mode letter, `M` or `K` for mouse or keyboard, a sound-device letter and, for
devices that have them, three digits of port/IRQ — the same information `CONFIG.U6` holds), and `argv[2..4]` are the years, months and days,
read with `_atol`. It reads `CONFIG.U6`, loads the video driver and `U6.SET`, the pictures of
`END.*` (`END.SHP` here: 13 full-screen shapes, see `u6_graphics.md`) and the music (`MIDI.DAT`, `END.M`), seeds the generator with the time,
and plays the scenes: the text pages (`show_ending_text` `10BFA`, which includes the scene where the Avatar hands Lord British the lens) and
the closing credits screen, in which the time played is spelled out in words (`number_to_words`, `12D1C`, "twenty-eight", ...).
It then waits for a key and exits.

Dates and the clock are the same in all three games' save files only in layout terms; the day/month/year fields start at `+0x1BF5`
of the Ultima VI `objlist`.

Not analysed: the scene scripting (palette fades, the animations between pages), which is presentation code driven through the graphics
driver like the introduction.

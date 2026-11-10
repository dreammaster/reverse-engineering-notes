# What each game asks of the stand-alone player

The stand-alone players built from `src/<game>` are what the games are played through to find wrongly implemented code before the engine is ported to ScummVM. So the question for each game is: **what does its data use that the player does not do yet?**
Method: `tests/deponia1/censustest.cpp` loads the game's data with the reconstruction (`GAME_PASSW=<password>`, run from the game folder, `tools/tbuild.py censustest.cpp [--game deponia4]`) and counts the records and the fields that need more than pictures, sounds and scripts. `tests/deponia1/datainventory.cpp <dir>` writes the game's Lua scripts to `<dir>` so that the Lua API they use can be counted. Run on 6 Nov 2026.

| | Deponia 1 | Deponia 2 | Deponia 3 | Deponia 4 | Night of the Rabbit |
|---|---|---|---|---|---|
| scenes | 74 | 94 | 131 | 168 | 95 |
| characters | 84 | 166 | 265 | 328 | 64 |
| scenes with a light map (shown as the tint of the objects) | 27 | 38 | 19 | 97 | 61 |
| particle system records (table 22) | 0 | 0 | 0 | 0 | 3 (**no scene, object or script uses them**: 0 of 95 scenes, 0 of 5,522 objects, no script says `particle`; checked 10 Nov 2026) |
| scenes, objects, buttons, characters with a shader set | 0 | 0 | 0 | 0 | 0 |
| shader, audio bus, event, plugin, block records | 0 | 0 | 0 | 0 | 0 |
| model and Spine ("bones") animations | 0 | 0 | 0 | 0 | 0 |
| different commands used by action parts | 82 | 80 | 85 | not counted | 82 |
| Lua scripts in the data | 95 | | | 109 | 309 |
| Lua API of the GL backend used by the scripts (`graphics.*`, shader commands) | `drawSprite`, `loadFromFile`, `addDrawFunc`, `removeDrawFunc`, `getCharacterTextPosition` | | | **the Shader Toolkit**: `shaderCompile`, `shaderSetOptions`, `shaderAddEffect`, `shaderViewport`, `shaderZoom`, `shaderBlur`, `shaderGlow`, `shaderHue`, `shaderLamp`, ... (521 uses) | none found in the 39 scripts that could be dumped |
| Lua that the engine must run | 5.1 | 5.1 | 5.1 | 5.1 (`string.dump` is called by the Shader Toolkit) | **5.4** (bitwise `&`, integers) |

What this says for each game (the engine's code is the same in all of them except where `src/<game>/VERSIONS.md` says otherwise):

- **Deponia 1**: nothing in the data needs a renderer that the player does not have: no shaders, particles, audio busses, models; the light maps are the tint of `TGScene::GetTint()`, as in the original. What is left is to play it and find wrong code.
  Every one of the 82 commands the data uses has its case in `TGAction::Execute` (`vstables/eCommand.h`; command 6, `EndIf`, has none because the original does nothing for it).
- **Deponia 2 and 3** (older engine): the same, with fewer features in the engine; the data loads with the Deponia 1 build (`src/deponia1`, or a copy with the older behaviours of `src/deponia2/VERSIONS.md`).
- **Deponia 4**: the data runs a Shader Toolkit written in Lua (`shaderCompile` and the effects) on the card: **it needs the GL backend** (`TGraphicsSubSystemGL`, `TShader`, `TShaderConstantBuffer`, `TFramebuffer`, the shader commands of `playerCommands`), which the player does not have; without it the effects are not drawn (the scripts keep their shader list but make no shaders, see TODO.md). Needs `string.dump` to fail without stopping the engine (`src/deponia4` does).
- **Night of the Rabbit**: needs **Lua 5.4** (the scripts do not even compile in Lua 5.1), and the engine changes in `src/nightoftherabbit/VERSIONS.md` (the conditions lists, the volumes in decibels, plugins, `TGraphicsInterface` with the batch and texture slots).

Order of work, by what stops a game from being played: (1) play Deponia 1 through and fix what is found; (2) the Lua 5.4 build for Night of the Rabbit (a vendored Lua 5.4 for the stand-alone player only; ScummVM keeps its own), its engine changes; (3) the GL backend for Deponia 4's Shader Toolkit, reimplemented from the original's `TGraphicsSubSystemGL` / `TShader` family and run through SDL's OpenGL; (4) the rest of the functions of `src/deponia1/COVERAGE.md` that the original has and the reconstruction has not (the audio bus system, the picture preloader thread, the debug console), so that every function with a symbol is reimplemented.

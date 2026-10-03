# Text resources in `MM3.CC`

All are NUL-terminated strings (text index n = n-th string, 0-based; in-text control codes as described in `video-module.md`).

| Member | Strings | Content |
|---|---|---|
| `AWARD.BIN` | 24 | award names (character record `awards[]`, `Awards_show`) |
| `QUEST.BIN` | 23 | quest descriptions shown by the quest/awards screen |
| `JESTER.BIN` | 95 | the "Joke of the Day" riddles (`showJoke`) |
| `TAVERN.BIN` | 197 | tavern rumours / hints (`townTavern`) |
| `CORAK.BIN` | 65 | the story text (Corak's history) |
| `COPY.BIN` | 1 | copy-protection prompt template ("Please turn to page ..., go to line ...", manual look-up; `protectionHandler` `39437`) |
| `SPLDESC.BIN` | 77 | spell descriptions, same order as `SPELL_NAMES` (`townGuild`) |
| `TEXTnn.MAZ` | 64 files | per-map strings for event text (see `data-files.md`) |

Awards: 0 Raven's Guild Member; 1 Albatross Guild Member; 2 Falcon's Guild Member; 3 Buzzard's Guild Member; 4 Eagle's Guild Member; 5 Saved Fountain Head; 6 Blessed by the Forces; 7 %u Orbs Given to Zealot; 8 %u Orbs Given to Malefactor; 9 %u Orbs Given to Tumult; 10 Champion of %s; 11 %u Good Artifacts Recovered; 12 %u Evil Artifacts Recovered; 13 %u Neut Artifacts Recovered; 14 %u Shells Given to Athea; 15 Greek Brothers Visited; 16 Greywind Released (645); 17 Blackwind Released (231); 18 %u Skulls Given to Kranion; 19 Icarus Resurrected; 20 Freed Princess Trueberry; 21 %u Arena Wins; 22 %u Pearls to Pirate Queen; 23 Ultimate Adventurer

# Constant-level fingerprint audit

Integer immediates of the original machine code vs integer literals of the DOS16-configured C. *bin-only* = loaded by the original but absent from the C; *C-only* = in the C but never loaded by the original. Overlay string addresses (b21e-relative pools), jump-table case labels and small struct strides are known noise.

## BLCKTHRN: 6 of 11 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 0278 | 114 |  | 52 ac |
| 03ae | 353 | 57a8 b34b b3be | f1 12d 1a0 |
| 051c | 46 | b447 b47d | 229 25f |
| 054a | 196 | b4ba b4fa | 29c 2dc |
| 060e | 770 | 7000 700e 701a b54a |  |
| 0910 | 854 | 3750 70d8 71c2 |  |

## CAST: 4 of 35 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 0114 | 154 | e8 |  |
| 074c | 103 | ef |  |
| 0dba | 1060 | c0cf c6ce |  |
| 1f60 | 517 | 11a |  |

## CAST2: 4 of 20 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 0966 | 957 | 4c1e b58b b5b6 b5de b5ff b62c b669 b692 b6b9 b6d7 | 36d 398 3c0 3e1 40e 44b 474 49b 4b9 |
| 0d24 | 319 | b703 b733 b75c b773 b79d b7ef b82a b89e | 4e5 515 53e 555 57f 5d1 60c 680 |
| 0e76 | 647 | 57c7 5d5a 95fc 960a 9618 aafc b8cc b8f9 | 6ae 6db |
| 10fe | 188 | 967a 9684 968e 9698 96a2 b31e | c114 c124 c128 c174 |

## CMDS: 0 of 26 functions differ

## COMBAT: 3 of 23 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 0000 | 334 | bb18 |  |
| 063e | 1366 | 80ac daac | 44 45 47 |
| 1c66 | 128 | fb |  |

## COMSUBS: 1 of 22 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 0f4a | 915 | f3 6f1 |  |

## DNGLOOK: 1 of 17 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 06a8 | 411 | a9ec a9f4 a9f9 | 16b |

## DUNGEON: 2 of 33 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 06c4 | 285 | 2d3e |  |
| 0948 | 157 | ff |  |

## ENDGAME: 2 of 9 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 0000 | 569 | 81fe |  |
| 0648 | 1183 | 5d5b 8480 848e 84a4 b346 b3e7 b42f b469 b4f3 | 49 ab d3 128 167 1c9 24b 2d5 |

## FLAMES: 0 of 2 functions differ

## FONT: 7 of 10 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 02a2 | 89 |  | 80 100 |
| 02fc | 284 | 5d5a |  |
| 04a4 | 1268 | 5d60 a044 |  |
| 09c8 | 322 | a052 |  |
| 0b0a | 839 | a0a6 a0b4 a0c2 a0cc a0d6 b31e |  |
| 0e52 | 41 | 1800 |  |
| 0e7b | 28 | 1800 |  |

## INTRO: 7 of 21 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 05b0 | 198 | 3105 |  |
| 0986 | 1680 | 3182 318a 3194 319b 31b5 31e6 323f 3249 3252 325c 3266 b31e |  |
| 1016 | 610 | 3278 |  |
| 1278 | 113 |  | 44 46 4d 50 52 53 54 |
| 132a | 2808 | 2320 2322 b31e | 3379 |
| 2090 | 29 | 69 |  |
| 20ae | 25 | 69 |  |

## LOOKOBJ: 4 of 26 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 0000 | 66 | 71f8 7202 |  |
| 06a4 | 83 | 7374 737e |  |
| 07e4 | 439 | 74e6 74f0 |  |
| 10fc | 207 |  | ff |

## MAINOUT: 2 of 35 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 0790 | 333 | 2a4a 2a53 |  |
| 0a84 | 669 | 2b35 2b4f 7a8c |  |

## NPC: 4 of 17 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 0000 | 213 | 6d46 6d50 6d5e 6d6a |  |
| 0adc | 194 |  | 3ff |
| 0d00 | 180 | b0 6af |  |
| 0db4 | 1324 |  | ff |

## OUTSUBS: 5 of 12 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 01b4 | 276 | 3976 3980 |  |
| 0368 | 32 | 3989 3992 |  |
| 0388 | 207 | 399f |  |
| 0458 | 269 | 39de 39e7 39f1 39fb |  |
| 0658 | 836 | 3a32 7760 77e4 77ee bb18 |  |

## SHOPPES: 7 of 25 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 0026 | 339 | b7fa | 5dc |
| 017a | 32 | 7840 |  |
| 0502 | 356 | 97b7 b6e2 | 4c4 1a67 |
| 0666 | 248 | 7a0a | 43 |
| 07be | 493 | 3c48 5d5a |  |
| 0e76 | 238 |  | b21e |
| 0f64 | 845 | 7efa |  |

## SHOPPES2: 0 of 14 functions differ

## SHOPPES3: 0 of 8 functions differ

## SJOG: 9 of 36 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 012a | 200 | ca |  |
| 095c | 590 | 2aca 30ca 5d5a 68ca ca30 ca48 ca68 | a8 ab ad af b2 bc |
| 0baa | 148 |  | ff |
| 112c | 424 | 5d5a bb14 |  |
| 12d4 | 160 | ff |  |
| 1458 | 837 |  | 43 |
| 18ce | 614 | 5d5a |  |
| 203e | 154 | aafc |  |
| 21ce | 136 | fe |  |

## TALK: 5 of 39 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 0054 | 88 |  | 96 97 98 99 9a |
| 04d2 | 8 |  | 80 |
| 04da | 8 |  | 80 |
| 0682 | 166 |  | 43 45 46 47 48 49 4a 4b |
| 0f32 | 489 | cf | 83 84 85 86 87 93 94 95 96 97 98 99 9a 9b 9c 9d |

## TOWN: 2 of 32 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 08d4 | 132 | 303 |  |
| 141e | 629 | 28a9 eb96 |  |

## ZSTATS: 4 of 18 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 0082 | 501 | 96c6 96d0 |  |
| 05e2 | 262 | ff |  |
| 099a | 159 | ba0b |  |
| 0c5c | 721 | 9812 |  |

## ULTIMA: 54 of 197 functions differ

| fn | size | bin-only | C-only |
|---|---|---|---|
| 0892 | 56 | 52ba |  |
| 0991 | 145 | 8000 |  |
| 0bae | 53 | 42 |  |
| 0be4 | 24 | 49 |  |
| 0bfc | 38 | 63 |  |
| 0d4c | 38 | 4b |  |
| 0de0 | 179 | 54 80 fc fd ff 3ba 5322 5fff 8000 a000 f000 |  |
| 0e94 | 149 | 4c 50 400 |  |
| 0f46 | 39 | 66 |  |
| 0fdc | 24 | 49 |  |
| 0ff4 | 57 | 48 |  |
| 102e | 21 | 5a |  |
| 1044 | 35 | 4e |  |
| 1068 | 119 | 66 |  |
| 10e0 | 50 | 51 |  |
| 1112 | 45 | 60 |  |
| 1140 | 24 | 6f |  |
| 1158 | 44 | 1233 1247 |  |
| 11b4 | 96 | 2ee |  |
| 125d | 253 | 49 |  |
| 16a6 | 19 | 41 | 44 |
| 16ba | 314 | 5d |  |
| 1850 | 494 | 42 |  |
| 1a3e | 216 | 540c |  |
| 1d5e | 124 | 44 8e ff |  |
| 2056 | 39 | fff 91eb |  |
| 20fa | 95 | f0 |  |
| 2192 | 169 | 42 61 64 fc |  |
| 223c | 131 | 42 61 64 fc 34de 9248 |  |
| 22e2 | 43 | 42 61 34de |  |
| 230e | 7 | 61 fc |  |
| 2322 | 507 | 1000 2320 2322 | 49 545f 547a 547d 5485 548f 5496 549c |
| 251e | 79 | 1000 2320 2322 |  |
| 25d8 | 147 | 1000 2322 25ca |  |
| 3178 | 938 |  | 48 49 4a 4b |
| 368e | 115 | 5d5a |  |
| 4402 | 181 | 6a07 | 3ff |
| 4552 | 432 |  | b0 |
| 47f4 | 179 | a3ba |  |
| 4a84 | 355 | 2020 |  |
| 4f7c | 547 | 57bf |  |
| 5a28 | 737 | 218 | 101 |
| 5e4a | 313 | 94 |  |
| 5f86 | 358 | 5d5a aafc |  |
| 60ec | 99 | a3f0 |  |
| 6506 | 653 | bb18 |  |
| 6800 | 127 | f7 |  |
| 6936 | 651 | 5d5f | 46 |
| 6f1e | 114 | 7f7f | 7f |
| 6f9e | 29 | 6c |  |
| 6fbc | 26 | 6c |  |
| 6fd6 | 25 | 60 |  |
| 71aa | 32 | 6c |  |
| 7234 | 98 | 42 |  |


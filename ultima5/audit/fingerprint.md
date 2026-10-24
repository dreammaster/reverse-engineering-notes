# Semantic fingerprint audit

**Overall:** 568 of 688 compared functions have no fingerprint differences.

For each C-defined function, the original binary's callees and data-segment references are compared to the decompiled C. *Binary-only* items are things the machine code does that the C never mentions (possible missing logic). *C-only* items are things the C does that the machine code never does (possible wrong variable/call). CRT long-math helper calls are ignored. Expect some noise: macros, constant folding, variables accessed through casts/struct fields.

## BLCKTHRN: 11 functions compared, 2 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 00be | blckthrn.c:73 |  |  | a368 a3d2 a406 |  |
| 02ea | blckthrn.c:215 |  |  | ffee |  |

## CAST: 35 functions compared, 5 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 0dba | cast.c:894 |  |  | aed0 b2ce d0c6 ffd0 |  |
| 11de | cast.c:1219 |  |  | cad1 d2c0 ffd2 |  |
| 135a | cast.c:1319 |  |  | d4a0 |  |
| 1792 | cast.c:1626 |  |  | 54da bed8 dade |  |
| 1f60 | cast.c:2085 |  |  | ff00 ff80 |  |

## CAST2: 20 functions compared, 2 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 00de | cast2.c:57 |  |  | 1b7a fff2 fff3 | 1bfc |
| 10fe | cast2.c:1237 |  | CAST2:10fe |  |  |

## CMDS: 26 functions compared, 0 with differences


## COMBAT: 23 functions compared, 1 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 063e | combat.c:371 |  |  | ac1a acfc |  |

## COMSUBS: 22 functions compared, 1 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 0f4a | comsubs.c:899 |  |  | f3f4 fffe ffff |  |

## DNGLOOK: 17 functions compared, 3 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 0340 | dnglook.c:224 |  |  | a84a a856 a900 bea8 |  |
| 06a8 | dnglook.c:390 | ULTIMA:93ac |  |  | 52ba |
| 117e | dnglook.c:1134 |  |  | fffa |  |

## DUNGEON: 33 functions compared, 3 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 0134 | dungeon.c:92 |  |  | 25fa | 25ea |
| 145c | dungeon.c:1243 |  |  |  | 52ba |
| 1786 | dungeon.c:1421 |  |  |  | 52ba |

## ENDGAME: 9 functions compared, 0 with differences


## FLAMES: 2 functions compared, 0 with differences


## FONT: 10 functions compared, 3 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 04a4 | font.c:274 | ULTIMA:d3f8 |  | eb2a |  |
| 0e52 | font.c:751 | FLAMES:20c0 |  | 520c 5354 |  |
| 0e7b | font.c:754 |  |  | 520c 5354 |  |

## INTRO: 21 functions compared, 7 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 05b0 | intro.c:319 |  |  |  | 25ea |
| 072e | intro.c:406 |  |  |  | 25ea |
| 0986 | intro.c:505 |  |  | ffff |  |
| 1016 | intro.c:932 |  |  | 9380 |  |
| 1278 | intro.c:1065 |  |  | 9482 |  |
| 2090 | intro.c:1699 |  |  | 5350 |  |
| 20ae | intro.c:1707 |  |  | 5350 |  |

## LOOKOBJ: 26 functions compared, 5 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 06f8 | lookobj.c:424 |  |  | ffff |  |
| 0cf4 | lookobj.c:708 |  |  |  | 52ba |
| 0dda | lookobj.c:745 |  |  |  | 52ba |
| 0e16 | lookobj.c:760 |  |  |  | 52ba |
| 0f7e | lookobj.c:828 |  |  | b32e |  |

## MAINOUT: 35 functions compared, 2 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 0a84 | mainout.c:812 |  |  | 8d50 8d88 8d8e |  |
| 1578 | mainout.c:1479 |  |  | 9826 9838 984a 9882 |  |

## NPC: 17 functions compared, 2 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 032c | npc.c:177 |  |  | ffb2 ffd4 |  |
| 0d00 | npc.c:832 |  |  | b021 b02c |  |

## OUTSUBS: 12 functions compared, 0 with differences


## SHOPPES: 25 functions compared, 2 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 0026 | shoppes.c:24 |  |  | 23ea | 24ea |
| 0e76 | shoppes.c:928 |  |  |  | b21e |

## SHOPPES2: 14 functions compared, 0 with differences


## SHOPPES3: 8 functions compared, 0 with differences


## SJOG: 36 functions compared, 3 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 012a | sjog.c:102 | ULTIMA:afff |  | c130 |  |
| 095c | sjog.c:632 |  |  | ca7e |  |
| 1458 | sjog.c:1289 |  |  | bcd5 d69e |  |

## TALK: 39 functions compared, 7 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 0000 | talk.c:26 |  |  | ffff |  |
| 00e6 | talk.c:92 |  |  | c14a |  |
| 0682 | talk.c:484 |  |  | c68e |  |
| 07be | talk.c:589 |  |  | ffff |  |
| 07e4 | talk.c:606 |  |  | ffff |  |
| 0f32 | talk.c:1174 |  |  | d01c ffff |  |
| 127e | talk.c:1459 |  |  | fffe |  |

## TOWN: 32 functions compared, 4 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 0408 | town.c:221 |  |  | ffff |  |
| 0c78 | town.c:788 |  |  | ffff |  |
| 0e34 | town.c:881 |  |  | 67b9 | 6608 |
| 141e | town.c:1268 |  |  | 9702 9722 |  |

## ZSTATS: 18 functions compared, 0 with differences


## ULTIMA: 197 functions compared, 68 with differences

| fn | src | bin-only calls | C-only calls | bin-only data | C-only vars |
|---|---|---|---|---|---|
| 0878 | 0000.c:37 |  |  | 5304 |  |
| 0892 | 0000.c:43 |  | ULTIMA:0892 | 5304 5350 5352 |  |
| 08ca | 0000.c:50 |  |  |  | 52ba |
| 0991 | 0000.c:167 |  |  |  | 52ba |
| 0a22 | 0000.c:239 |  |  |  | 52ba |
| 0a70 | 0000.c:272 |  |  | 5350 | 52ba |
| 0aa6 | 0000.c:288 |  |  | 5350 |  |
| 0ace | 0000.c:300 |  |  | 5350 |  |
| 0b2d | 0000.c:336 |  |  | 5350 |  |
| 0b86 | 0000.c:391 |  |  | 5350 |  |
| 0bae | 0000.c:400 |  | ULTIMA:0bae | 5350 535c 5394 |  |
| 0bfc | 0000.c:424 |  |  | 5350 |  |
| 0c22 | 0000.c:432 |  |  | 5350 |  |
| 0c3c | 0000.c:439 |  |  |  | 52ba |
| 0c64 | 0000.c:448 |  |  | 5350 |  |
| 0c9c | 0000.c:467 |  |  | 5350 |  |
| 0ccd | 0000.c:485 |  |  |  | 52ba |
| 0cf2 | 0000.c:505 |  |  | 5350 |  |
| 0d2b | 0000.c:529 |  |  |  | 52ba |
| 0d4c | 0000.c:541 |  |  | 5350 |  |
| 0de0 | 0000.c:571 |  |  |  | 52ba |
| 0e94 | 0000.c:578 | ULTIMA:125d ULTIMA:15c6 |  | 52ba 52c8 52ef 52f1 52f3 5328 5352 535c 5394 |  |
| 0f2a | 0000.c:586 |  |  | 5350 5354 |  |
| 0f46 | 0000.c:594 |  | ULTIMA:0f46 | 5350 |  |
| 0f6e | 0000.c:602 |  | ULTIMA:0f6e | 5350 |  |
| 0f90 | 0000.c:612 |  |  |  | 52ba |
| 0fae | 0000.c:620 |  | D:5394 ULTIMA:0fae |  |  |
| 0fdc | 0000.c:643 |  | ULTIMA:0fdc |  |  |
| 0ff4 | 0000.c:651 |  | ULTIMA:0ff4 | 5350 535c 5394 |  |
| 102e | 1000.c:24 |  |  | 5350 |  |
| 1044 | 1000.c:31 |  |  | 5350 |  |
| 1068 | 1000.c:39 |  |  | 5350 |  |
| 10e0 | 1000.c:84 |  |  | 5350 |  |
| 1112 | 1000.c:96 |  |  | 5350 |  |
| 11b4 | 1000.c:154 |  |  | 1214 5356 5358 535a |  |
| 125d | 1000.c:173 | ULTIMA:135a ULTIMA:15c6 ULTIMA:160e ULTIMA:1654 | ULTIMA:125d | 53b4 53b6 53b8 53ba 53bc 53be |  |
| 1674 | 1000.c:264 |  | ULTIMA:1674 | 535c 5394 |  |
| 16ba | 1000.c:298 |  |  |  | 52ba |
| 17f4 | 1000.c:416 |  |  |  | 52ba |
| 1d02 | 1000.c:886 | ULTIMA:15c6 ULTIMA:160e ULTIMA:1654 | ULTIMA:0fae ULTIMA:1d02 |  |  |
| 1d5e | 1000.c:897 | ULTIMA:1b24 |  | 538a 538c 540e 5416 |  |
| 1eac | 1000.c:970 |  | D:5394 ULTIMA:1eac |  |  |
| 1f4e | 1000.c:1038 | ULTIMA:1f77 |  | 5350 539a |  |
| 1fa0 | 1000.c:1059 |  |  | ffff |  |
| 20fa | 2000.c:116 |  |  | 2159 5356 5448 544a 544c 544e |  |
| 216c | 2000.c:125 |  |  | ffff |  |
| 2192 | 2000.c:140 |  |  | 5356 5450 5452 5454 a9ce |  |
| 223c | 2000.c:151 |  |  | 5356 5456 5458 545a 545c a9ce |  |
| 2322 | 2000.c:220 |  | ULTIMA:1850 |  |  |
| 256e | 2000.c:388 |  | ULTIMA:2322 |  |  |
| 25d8 | 2000.c:425 |  | ULTIMA:2322 |  |  |
| 266c | 2000.c:461 |  |  | 26fe | 52ba |
| 2c4c | 2000.c:832 |  |  | 2d60 |  |
| 3178 | 3000.c:103 |  |  | 3490 |  |
| 3b9e | 3000.c:715 |  |  | fffa |  |
| 4552 | 4000.c:337 |  |  | 46c2 |  |
| 5a28 | 5000.c:425 |  |  | 5b16 |  |
| 5e4a | 5000.c:724 |  |  | ff74 |  |
| 6150 | 6000.c:34 |  |  | 62d8 |  |
| 6936 | 6000.c:473 |  |  | 6b04 |  |
| 6f1e | 6000.c:796 | ULTIMA:6f90 |  | 6aa4 6aa6 |  |
| 6f9e | 6000.c:847 |  |  | 5350 |  |
| 6fbc | 6000.c:857 |  |  | 5350 |  |
| 71aa | 7000.c:78 |  |  | 5350 |  |
| 71ca | 7000.c:87 |  |  | 5350 |  |
| 7200 | 7000.c:104 |  |  | 5350 |  |
| 7234 | 7000.c:121 |  | D:5394 |  |  |
| 7296 | 7000.c:136 |  | D:5394 |  |  |


# MM3 monsters (decoded from the `MON*.DAT` columns and `MONSTER_NAMES`)

Field meanings: `dmg` = NdS physical damage (`MONDMGN`d`MONDMGS`), `att` = attacks per turn (`MONNUMA`), `type` = damage type (`MONDMGT`: 0 physical, 1 magical, 2 fire, 3 electrical, 4 cold, 5 poison, 6 energy - same numbering as Xeen), `hates` = `MONATTP` (target selection in `doMonsterTurn`: 0 hits the whole party, 1 one random member, 2 cleric, 3 sorcerer, 4 druid, 5 paladin, 6 race 2 - the same idea as Xeen's `_hatesClass`), `spec` = `MONSPEC` special attack (numbering decoded in the "Special attacks" section below; it is NOT Xeen's), `rng` = `MONRANG`, `hit` = `MONHITB`, `tr` = `MONTREA` treasure class, then gold, gems and the percent resistances magic/fire/elec/cold/acid(poison)/energy/physical.

| id | name | HP | AC | spd | dmg | att | exp | type | hates | spec | rng | hit | tr | gold | gems | resist m/f/e/c/a/en/ph |
|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|---|
| 0 | Vampire Bat | 5 | 5 | 20 | 2d2 | 2 | 250 | physical | random | 1 | 0 | 5 | 0 | 0 | 0 | 0/0/0/0/0/0/0 |
| 1 | Bubble Man | 15 | 0 | 15 | 1d6 | 1 | 250 | magical | random | 0 | 1 | 0 | 0 | 0 | 0 | 0/0/50/50/100/0/50 |
| 2 | Goblin | 10 | 0 | 15 | 3d3 | 1 | 400 | physical | random | 0 | 1 | 3 | 1 | 10 | 0 | 0/0/0/0/0/0/0 |
| 3 | Orc Warrior | 25 | 5 | 12 | 2d8 | 1 | 600 | physical | random | 0 | 1 | 5 | 1 | 20 | 0 | 0/50/50/50/0/0/0 |
| 4 | Skeleton | 20 | 2 | 18 | 2d6 | 2 | 1000 | physical | cleric | 0 | 0 | 4 | 0 | 0 | 5 | 0/50/50/50/0/0/80 |
| 5 | Screamer | 10 | 10 | 25 | 2d4 | 1 | 1750 | energy | whole party | 3 | 0 | 0 | 0 | 0 | 5 | 0/0/0/0/0/100/0 |
| 6 | Oh No Bug | 40 | 8 | 30 | 3d3 | 3 | 1000 | physical | random | 0 | 0 | 6 | 0 | 0 | 0 | 0/0/60/80/80/0/0 |
| 7 | Moose Rat | 40 | 4 | 16 | 2d8 | 2 | 1200 | physical | random | 0 | 0 | 8 | 0 | 0 | 0 | 0/30/30/30/30/0/0 |
| 8 | Wild Fungus | 25 | 0 | 5 | 3d4 | 1 | 2000 | electrical | whole party | 0 | 0 | 0 | 0 | 0 | 10 | 25/0/100/100/50/50/0 |
| 9 | Zombie | 35 | 2 | 2 | 3d6 | 2 | 1800 | physical | cleric | 4 | 0 | 5 | 0 | 0 | 6 | 0/0/75/75/0/0/80 |
| 10 | Candle Creep | 70 | 5 | 8 | 2d5 | 2 | 3000 | fire | random | 0 | 1 | 0 | 1 | 0 | 5 | 0/0/100/0/50/0/50 |
| 11 | Mad Dwarf | 75 | 10 | 16 | 4d5 | 1 | 2500 | physical | race2 | 0 | 0 | 10 | 2 | 100 | 15 | 20/50/50/50/50/50/0 |
| 12 | Ninja | 45 | 15 | 35 | 2d4 | 4 | 3000 | physical | random | 0 | 0 | 15 | 1 | 30 | 0 | 25/20/20/20/20/20/20 |
| 13 | Magic Mantis | 50 | 12 | 30 | 2d10 | 2 | 3500 | physical | random | 1 | 0 | 8 | 0 | 0 | 10 | 50/0/0/0/0/0/0 |
| 14 | Ogre | 60 | 10 | 15 | 2d16 | 1 | 2500 | physical | random | 7 | 1 | 10 | 2 | 50 | 5 | 0/50/30/30/20/0/0 |
| 15 | Bugaboo | 60 | 15 | 22 | 2d12 | 2 | 4000 | magical | sorcerer | 0 | 0 | 0 | 2 | 0 | 8 | 80/50/50/50/50/50/0 |
| 16 | Phase Head | 20 | 10 | 25 | 2d4 | 1 | 4000 | physical | whole party | 5 | 0 | 10 | 0 | 0 | 5 | 0/75/75/75/75/75/0 |
| 17 | Giant Spider | 30 | 14 | 25 | 2d4 | 8 | 3000 | physical | random | 1 | 0 | 12 | 0 | 0 | 3 | 0/0/50/30/50/50/0 |
| 18 | Sprite | 15 | 13 | 18 | 2d3 | 2 | 2500 | electrical | random | 6 | 0 | 0 | 2 | 40 | 10 | 60/25/25/25/25/25/50 |
| 19 | Dino Beetle | 70 | 10 | 18 | 3d5 | 6 | 4000 | physical | random | 0 | 0 | 20 | 0 | 200 | 0 | 0/25/25/25/25/25/0 |
| 20 | Cobra Fiend | 50 | 15 | 25 | 2d15 | 1 | 4000 | physical | random | 2 | 0 | 25 | 1 | 0 | 5 | 50/20/20/20/0/0/0 |
| 21 | Scorpia | 50 | 5 | 10 | 3d4 | 1 | 5000 | poison | whole party | 1 | 0 | 0 | 2 | 50 | 10 | 10/0/0/0/0/0/0 |
| 22 | Cryo Spore | 40 | 3 | 16 | 4d4 | 1 | 6000 | cold | whole party | 0 | 0 | 0 | 0 | 0 | 20 | 20/0/80/100/80/0/0 |
| 23 | Cursed Fool | 40 | 8 | 15 | 3d3 | 3 | 3500 | physical | sorcerer | 6 | 0 | 20 | 2 | 100 | 10 | 25/0/0/0/0/0/0 |
| 24 | Mini Dragon | 150 | 20 | 30 | 50d1 | 1 | 18000 | fire | whole party | 0 | 1 | 0 | 3 | 2500 | 100 | 10/100/20/0/20/0/0 |
| 25 | Plasmoid | 100 | 5 | 17 | 4d3 | 3 | 8000 | poison | random | 7 | 0 | 0 | 2 | 500 | 10 | 0/60/60/0/60/60/50 |
| 26 | Carnage Hand | 40 | 25 | 20 | 60d2 | 1 | 10000 | physical | random | 0 | 0 | 25 | 0 | 0 | 30 | 80/0/0/0/0/0/0 |
| 27 | Ghoul | 100 | 15 | 16 | 3d6 | 4 | 16000 | physical | random | 8 | 0 | 27 | 2 | 250 | 0 | 0/0/0/0/0/0/80 |
| 28 | Castle Guard | 75 | 10 | 20 | 2d40 | 1 | 10000 | physical | random | 0 | 0 | 30 | 2 | 0 | 0 | 20/0/0/0/0/0/0 |
| 29 | Phantom | 50 | 12 | 20 | 4d4 | 1 | 16000 | magical | random | 11 | 0 | 0 | 2 | 0 | 15 | 20/0/0/0/0/0/90 |
| 30 | Pirana | 40 | 20 | 30 | 3d3 | 8 | 10000 | physical | random | 0 | 0 | 20 | 0 | 0 | 0 | 0/0/0/0/0/0/0 |
| 31 | Evil Ranger | 100 | 20 | 20 | 4d6 | 3 | 12000 | physical | druid | 0 | 1 | 25 | 2 | 1000 | 25 | 0/20/20/20/0/0/0 |
| 32 | Shadow Rogue | 50 | 15 | 22 | 3d6 | 2 | 12000 | physical | random | 1 | 1 | 20 | 3 | 500 | 50 | 60/0/0/0/0/0/0 |
| 33 | Tree Golem | 150 | 10 | 6 | 2d25 | 2 | 16000 | physical | cleric | 0 | 0 | 25 | 2 | 200 | 10 | 90/0/0/0/0/0/60 |
| 34 | Wicked Witch | 50 | 8 | 16 | 4d4 | 1 | 16000 | magical | whole party | 16 | 1 | 0 | 3 | 300 | 15 | 50/0/0/0/0/0/0 |
| 35 | Iron Wizard | 200 | 30 | 50 | 50d1 | 2 | 25000 | energy | random | 0 | 1 | 0 | 0 | 0 | 30 | 0/80/80/80/80/0/50 |
| 36 | Death Locust | 100 | 20 | 30 | 4d8 | 4 | 16000 | physical | random | 4 | 0 | 30 | 0 | 0 | 0 | 0/0/40/40/40/0/0 |
| 37 | Archer | 100 | 15 | 35 | 5d6 | 4 | 20000 | physical | random | 0 | 1 | 35 | 3 | 2000 | 40 | 15/15/15/15/15/0/0 |
| 38 | Mystic Cloud | 50 | 18 | 40 | 4d4 | 1 | 30000 | magical | whole party | 9 | 1 | 0 | 0 | 0 | 25 | 90/0/0/0/0/0/0 |
| 39 | Barbarian | 175 | 15 | 30 | 2d30 | 2 | 25000 | physical | random | 0 | 1 | 30 | 2 | 600 | 0 | 10/50/50/50/50/0/0 |
| 40 | Cleric of Moo | 100 | 10 | 20 | 2d18 | 1 | 32000 | electrical | whole party | 0 | 1 | 0 | 2 | 1500 | 30 | 0/20/20/20/20/0/0 |
| 41 | Fire Lizard | 150 | 10 | 30 | 2d25 | 2 | 25000 | fire | random | 0 | 1 | 0 | 0 | 0 | 0 | 5/100/50/0/50/0/0 |
| 42 | Fire Stalker | 75 | 20 | 40 | 3d10 | 3 | 30000 | fire | random | 0 | 0 | 0 | 0 | 100 | 10 | 0/100/80/0/0/0/100 |
| 43 | Gargoyle | 125 | 15 | 30 | 3d15 | 4 | 30000 | physical | random | 10 | 0 | 30 | 3 | 800 | 10 | 10/0/0/0/0/0/0 |
| 44 | Ghost | 100 | 13 | 25 | 10d10 | 1 | 32000 | energy | random | 11 | 0 | 0 | 3 | 0 | 25 | 15/0/0/0/0/0/100 |
| 45 | Draconi | 125 | 10 | 20 | 3d20 | 2 | 20000 | physical | random | 0 | 0 | 30 | 3 | 800 | 0 | 25/0/0/0/0/0/0 |
| 46 | Sonic Ninja | 75 | 20 | 20 | 3d10 | 8 | 20000 | physical | random | 0 | 0 | 40 | 3 | 500 | 0 | 10/20/20/20/20/80/0 |
| 47 | Evil Eye | 100 | 25 | 35 | 50d1 | 4 | 60000 | magical | random | 3 | 1 | 0 | 4 | 0 | 30 | 90/0/0/0/0/0/0 |
| 48 | Guardian | 250 | 20 | 15 | 75d2 | 1 | 40000 | physical | random | 0 | 0 | 35 | 3 | 0 | 10 | 0/80/80/80/80/0/0 |
| 49 | Paladin | 175 | 30 | 30 | 3d30 | 5 | 50000 | physical | random | 0 | 1 | 40 | 3 | 4000 | 25 | 50/50/50/50/50/0/0 |
| 50 | Dark Pegasus | 125 | 20 | 40 | 2d20 | 4 | 40000 | physical | sorcerer | 5 | 0 | 35 | 3 | 0 | 0 | 10/0/0/0/0/0/30 |
| 51 | Reaper | 150 | 15 | 25 | 4d20 | 1 | 50000 | magical | cleric | 0 | 1 | 0 | 3 | 0 | 20 | 0/0/0/0/0/0/90 |
| 52 | Sorcerer | 100 | 10 | 40 | 8d10 | 1 | 50000 | cold | whole party | 0 | 1 | 0 | 4 | 2000 | 100 | 25/0/0/0/0/0/0 |
| 53 | Lich | 200 | 12 | 50 | 5d5 | 1 | 120000 | magical | whole party | 15 | 1 | 0 | 5 | 10000 | 100 | 50/0/0/0/0/0/70 |
| 54 | Spirit Shield | 100 | 35 | 80 | 6d20 | 2 | 60000 | physical | random | 0 | 0 | 40 | 0 | 0 | 0 | 0/0/0/0/0/0/80 |
| 55 | Troll | 125 | 15 | 25 | 3d15 | 3 | 50000 | physical | race2 | 0 | 0 | 35 | 3 | 2500 | 20 | 0/0/0/0/0/0/0 |
| 56 | Major Demon | 333 | 16 | 33 | 2d20 | 6 | 100000 | physical | random | 10 | 0 | 40 | 4 | 3333 | 33 | 80/100/0/0/0/0/0 |
| 57 | Dinosaur | 500 | 10 | 12 | 5d100 | 2 | 80000 | physical | random | 0 | 0 | 60 | 0 | 0 | 0 | 0/0/80/80/80/0/0 |
| 58 | ED-409 | 400 | 40 | 75 | 50d2 | 3 | 120000 | energy | random | 0 | 1 | 0 | 0 | 0 | 100 | 0/80/80/80/80/20/60 |
| 59 | Black Knight | 375 | 30 | 50 | 4d40 | 7 | 100000 | physical | paladin | 8 | 1 | 50 | 4 | 8000 | 0 | 20/60/60/60/60/0/0 |
| 60 | Death Agent | 300 | 15 | 30 | 10d10 | 2 | 70000 | poison | random | 1 | 0 | 0 | 3 | 0 | 20 | 10/0/0/0/0/0/0 |
| 61 | Mummy | 250 | 15 | 30 | 2d40 | 2 | 120000 | physical | druid | 4 | 0 | 40 | 4 | 1000 | 25 | 0/0/80/80/80/100/80 |
| 62 | Priest of Moo | 200 | 20 | 40 | 4d15 | 1 | 120000 | electrical | whole party | 0 | 1 | 0 | 3 | 6000 | 45 | 10/40/40/40/40/0/0 |
| 63 | Toxic Worm | 300 | 25 | 60 | 2d30 | 2 | 90000 | physical | random | 1 | 0 | 40 | 0 | 0 | 0 | 5/30/30/30/30/30/0 |
| 64 | Dragon Worm | 400 | 35 | 45 | 100d1 | 1 | 150000 | poison | whole party | 1 | 0 | 0 | 4 | 5000 | 60 | 20/0/0/0/0/0/0 |
| 65 | Cyclops | 500 | 25 | 40 | 6d25 | 2 | 150000 | physical | random | 5 | 0 | 50 | 4 | 10000 | 0 | 30/0/0/0/0/0/0 |
| 66 | Major Devil | 666 | 33 | 66 | 2d40 | 4 | 250000 | physical | random | 12 | 0 | 50 | 5 | 6666 | 66 | 90/100/0/0/0/0/0 |
| 67 | Green Dragon | 800 | 40 | 60 | 250d1 | 1 | 500000 | cold | whole party | 0 | 1 | 0 | 5 | 25000 | 500 | 25/0/50/100/0/0/0 |
| 68 | Jouster | 600 | 35 | 50 | 20d20 | 1 | 180000 | physical | random | 0 | 0 | 80 | 5 | 10000 | 0 | 20/0/0/0/0/0/0 |
| 69 | Wizard | 250 | 20 | 80 | 232d1 | 1 | 240000 | magical | random | 0 | 1 | 0 | 5 | 15000 | 200 | 50/70/70/70/70/30/0 |
| 70 | Death Snake | 500 | 25 | 90 | 4d50 | 1 | 150000 | physical | random | 10 | 0 | 50 | 0 | 0 | 0 | 0/0/0/0/0/0/0 |
| 71 | Vampire | 400 | 30 | 45 | 10d10 | 3 | 250000 | physical | cleric | 9 | 0 | 60 | 5 | 7500 | 50 | 10/0/80/100/100/0/90 |
| 72 | Werewolf | 500 | 30 | 40 | 8d15 | 2 | 150000 | physical | random | 4 | 0 | 50 | 4 | 0 | 25 | 20/20/20/20/20/0/0 |
| 73 | Terminator | 1000 | 100 | 200 | 232d4 | 1 | 3000000 | energy | random | 13 | 1 | 0 | 0 | 0 | 200 | 0/100/100/100/100/0/100 |
| 74 | Great Hydra | 5000 | 60 | 75 | 12d12 | 12 | 4000000 | physical | random | 1 | 0 | 100 | 6 | 50000 | 100 | 10/100/50/0/50/0/0 |
| 75 | Vulture Roc | 2000 | 50 | 100 | 5d50 | 2 | 2000000 | physical | random | 10 | 0 | 100 | 6 | 25000 | 0 | 20/50/50/0/50/0/0 |
| 76 | Kudo Crab | 2500 | 80 | 80 | 8d30 | 4 | 2000000 | physical | random | 7 | 0 | 100 | 6 | 40000 | 0 | 15/50/50/100/50/0/0 |
| 77 | Medusa | 1000 | 40 | 60 | 8d8 | 1 | 3000000 | magical | whole party | 14 | 1 | 0 | 6 | 10000 | 200 | 80/0/0/0/0/0/0 |
| 78 | Minotaur | 1000 | 90 | 80 | 3d100 | 2 | 3000000 | physical | random | 15 | 0 | 150 | 6 | 100000 | 100 | 100/0/0/0/0/0/0 |
| 79 | Octobeast | 3000 | 40 | 100 | 5d50 | 8 | 3000000 | physical | random | 8 | 0 | 150 | 6 | 50000 | 50 | 0/90/80/80/90/0/0 |
| 80 | Dragon Lord | 10000 | 75 | 150 | 232d1 | 1 | 10000000 | energy | whole party | 0 | 1 | 0 | 6 | 250000 | 1000 | 100/100/100/100/100/100/0 |
| 81 | Rat Overlord | 250 | 4 | 16 | 2d8 | 6 | 8000 | physical | random | 0 | 0 | 15 | 0 | 0 | 0 | 0/30/30/30/30/0/0 |
| 82 | Mummy King | 500 | 15 | 30 | 2d40 | 3 | 250000 | physical | druid | 4 | 0 | 40 | 5 | 1000 | 500 | 0/0/80/80/80/100/80 |
| 83 | Cyclops King | 1000 | 25 | 40 | 6d25 | 3 | 300000 | physical | random | 5 | 0 | 50 | 5 | 100000 | 0 | 80/0/0/0/0/0/0 |
| 84 | Minotaur King | 2500 | 90 | 80 | 3d100 | 3 | 6000000 | physical | random | 15 | 0 | 150 | 6 | 100000 | 30000 | 100/0/0/0/0/0/0 |
| 85 | Vampire King | 1000 | 30 | 45 | 10d10 | 4 | 500000 | physical | cleric | 9 | 0 | 60 | 6 | 25000 | 5000 | 40/0/80/100/100/0/90 |
| 86 | Moo Master | 400 | 20 | 40 | 5d15 | 1 | 250000 | electrical | whole party | 0 | 1 | 0 | 5 | 6000 | 200 | 10/40/40/40/40/0/0 |
| 87 | Top Jouster | 1000 | 35 | 50 | 20d20 | 2 | 300000 | physical | random | 0 | 0 | 80 | 6 | 10000 | 0 | 60/0/0/0/0/0/0 |
| 88 | Eye Master | 200 | 25 | 35 | 75d1 | 4 | 200000 | magical | random | 3 | 1 | 0 | 6 | 0 | 350 | 90/0/0/0/0/0/0 |
| 89 | Cult Leader | 300 | 20 | 20 | 4d6 | 5 | 30000 | physical | druid | 0 | 1 | 25 | 5 | 10000 | 25 | 20/20/20/20/0/0/0 |

## Special attacks (`doCharDamage`, `4A779`; `MONSPEC` value = switch case 1..16)

After a successful hit the character gets a saving throw (`charSavingThrow`); if it fails the special attack (value of `Mon_spec[monster]`) is applied.
Condition index `c` is the byte at char `+113h + c` (condition counter, saturating at FFh; order as in `character.h`):

| MONSPEC | effect | MONSPEC | effect |
|---|---|---|---|
| 1 | poison (c=3) | 9 | spell points set to 0 |
| 2 | sleep (c=8) | 10 | paralyse (c=11) |
| 3 | insane (c=5) | 11 | age +5 years (char +26h) |
| 4 | disease (c=4) | 12 | knocked unconscious (c=12, hit points set to 0) |
| 5 | confusion (c=10) | 13 | eradicate (c=15, hit points 0) |
| 6 | curse (c=0) | 14 | stone (c=14, hit points 0) |
| 7 | breaks one worn body armour (item ids 34-41 get flag 80h, `broken`) | 15 | death (c=13, hit points 0) |
| 8 | weakness (c=2) | 16 | curses every inventory item (flag 40h on ids < 70) |

The condition-name mapping assumes the Xeen condition order (cursed, heart broken, weak, poisoned, diseased, insane, in love, drunk, asleep, depressed, confused, paralysed, unconscious, dead, stone, eradicated), which agrees with the two verified entries (unconscious 0Ch at +11Fh, dead 0Dh at +120h).
The effect is skipped when the damage type 6-way switch earlier in the routine says the character resists (not decoded in detail).

## Choosing the target (`doMonsterTurn`, `4AB68`)

`MONATTP` 0 = hit every party member, 1 = random single target, 2/3/4/5 = prefers (the first) living member of class Cleric / Sorcerer /
Druid / Paladin, 6 = prefers a character of race 2 (Gnome in the name table; the table column below says `race2`); if no preferred character is in the party it falls back to the random choice.
The random single target depends on the combat party size `n`: for 1 member it is slot 0; for 2-5 `rnd(0, n-1)`; for 6, 7, 8 members the
extra (last) member is hit only on a top roll: `rnd(1,8) == 8` -> slot 5 (6 members), `rnd(1,10) == 10` -> slot 6 (7), `rnd(1,12) == 12` -> slot 7 (8); otherwise `rnd(0, n-2)`.
Characters whose worst condition is 0Bh-0Fh (paralysed, unconscious, dead, stone, eradicated) are skipped for the preferred-class search.

## Monster to-hit (`doMonsterTurn`, per attack; `MONNUMA` attacks per turn)

For a physical attack (`MONDMGT` = 0) on an awake character: roll d20 -- 20 always hits, 1 always misses (message "miss"), otherwise
`score = d20 + rnd(1, MONHITB) + MONHITB / 4` and the attack hits when `score >= AC + D`, where `AC = getArmorClass(char)` and
`D = 10`, or `15 + level / 2` when the character has taken the **block** action this round (flag byte array at DGROUP `C4CBh`
by party slot, set by `block`, `4B422`).  Non-physical attacks and attacks on sleeping characters always hit; a hit then runs `doCharDamage`.

## Combat round structure (`doCombat`, `4B7B5`)

* Party order comes from `setSpeedTable` / `nextChar`; each character's command (from `getCommand`): attack, block (sets the guard flag used by monster to-hit), cast
  (`spellsDialog`), use item (`characterInfoInventory`), run (`run`, success chance = page header byte 07h), quick fight (`quickFight` = repeat attack for the rest of the round), character info.
* After the party has acted (`allHaveGone`), the round ends with: Trolls (monster id 55) have their hit points reset to full (`Mon_hp[55]`) -- they regenerate completely every round;
  `monstersRecover` (`15235`) gives every monster with a non-zero state (put there by Sleep/Immobilize/Paralyze/... spells; `Maze_monState`) a `monsterSavingThrow` to shake it off;
  then `moveMonsters`, `scanMonstersAhead`, `changeTime(1)` (one minute per round) and `drawView` / `checkPartyDead`.
* Victory: `giveTreasure` (and experience via `giveExperience`), then `updateAutomap`.

`monsterSavingThrow(n)` (`4B3AB`): succeeds when `rnd(1, n + 50) <= n`; `monstersRecover` passes the **monster id** as `n`, so higher-numbered (later, tougher) monsters shake off Sleep/Paralyse etc. more easily (as the code stands).
`run` (`4B3D1`): rolls d100 for the acting character against page header byte 07h (run chance, e.g. 32h = 50%); on success that single character is removed from the combat party (`Combat_partySize--`, `sortCombatParty`) and the round continues -- there is no whole-party escape roll.

## Which monsters the status spells can affect (`attack`, `4A130`)

For the status effects (`SpellAttack_type` 7-16) the spell only works on monsters whose id is in a per-spell list (byte tables in the data segment); a listed monster then gets a `monsterSavingThrow`, and if it fails its state (`Maze_monState`) becomes the spell type (Finger of Death instead reduces the hit points). Every other monster is immune. Lists (tables at DGROUP `4B2Ah` sleep, `4B2Fh` immobilize, `4B3Ch` feeble mind, `4B49h` paralyze, `4B61h` finger of death, `4B77h` silence):

* **Sleep (type 7)**: 2 Goblin, 3 Orc Warrior, 7 Moose Rat, 14 Ogre, 22 Cryo Spore
* **Immobilize (8)**: 2 Goblin, 3 Orc Warrior, 4 Skeleton, 7 Moose Rat, 8 Wild Fungus, 9 Zombie, 11 Mad Dwarf, 13 Magic Mantis, 15 Bugaboo, 19 Dino Beetle, 20 Cobra Fiend, 21 Scorpia, 22 Cryo Spore
* **Feeble Mind (9)**: 2 Goblin, 3 Orc Warrior, 12 Ninja, 21 Scorpia, 24 Mini Dragon, 28 Castle Guard, 31 Evil Ranger, 32 Shadow Rogue, 37 Archer, 40 Cleric of Moo, 45 Draconi, 49 Paladin, 52 Sorcerer
* **Paralyze (10)**: 2 Goblin, 3 Orc Warrior, 12 Ninja, 21 Scorpia, 24 Mini Dragon, 28 Castle Guard, 31 Evil Ranger, 32 Shadow Rogue, 37 Archer, 40 Cleric of Moo, 45 Draconi, 46 Sonic Ninja, 49 Paladin, 50 Dark Pegasus, 52 Sorcerer, 55 Troll, 57 Dinosaur, 59 Black Knight, 62 Priest of Moo, 63 Toxic Worm, 65 Cyclops, 68 Jouster, 69 Wizard, 78 Minotaur
* **Finger of Death (11)**: 0 Vampire Bat, 2 Goblin, 3 Orc Warrior, 7 Moose Rat, 11 Mad Dwarf, 12 Ninja, 14 Ogre, 18 Sprite, 20 Cobra Fiend, 21 Scorpia, 23 Cursed Fool, 24 Mini Dragon, 28 Castle Guard, 30 Pirana, 31 Evil Ranger, 32 Shadow Rogue, 34 Wicked Witch, 37 Archer, 39 Barbarian, 41 Fire Lizard, 46 Sonic Ninja, 49 Paladin
* **Silence (16)**: 5 Screamer, 23 Cursed Fool, 34 Wicked Witch, 40 Cleric of Moo, 52 Sorcerer

Other special spell types in `attack`:
* 11 Finger of Death: a monster in its list takes damage equal to its **current** hit points (instant kill, no saving throw).
* 12 Holy Word and 14 Turn Undead only work on the nine undead listed in the first nine bytes of table `4BD8h` (4 Skeleton, 9 Zombie, 27 Ghoul, 29 Phantom, 44 Ghost, 51 Reaper, 53 Lich, 61 Mummy, 71 Vampire): Holy Word does damage equal to their full `MONHP`, Turn Undead 25 damage.
* 13 Mass Distortion: half of each monster's current hit points (at least 1).
* 15 Disintegrate: damage equal to the monster's current hit points, but a monster with more than 150 gets only 50 energy damage (type 6).
* Any other damage type (0-6) is plain damage (`SpellAttack_damage`) reduced by the monster's resistance (`getMonsterResistance`).

## Monster movement and ranged attacks (`moveMonsters` 1B2xx, `moveMonsterBy` 1B2A3)

Monsters do move.  `moveMonsters` rebuilds `Maze_occupancy` (32x32 bytes at DGROUP `8EF4h`: each cell holds the sum of the `MONSTER_SIZE` values of the monsters in it), then, for the cells around the party,
lets monsters step towards it with `moveMonsterBy(index, dx, dy)`: the step is made only when the target cell's occupancy plus the monster's size is below 4 (so e.g. four small monsters or one big one fit in a cell), the
monster's state is 0 (not asleep/held) and `byte_2884C` allows movement; it updates the occupancy grid and `Maze_monX/Y` and flags the monster as moved.
Monsters with a ranged attack (`MONRANG` != 0) standing in the party's row or column (and not already shown in one of the three near rows) fire at it once (`monstersAttack` via the stub `sub_27F5E`, flag byte array at `EE1Eh`/`ED74h`).
After the movement pass the near-row bytes (`byte_34B92/93/94`) are refreshed; combat starts from `drawView` when they are non-zero (`engine-loop.md`).
Cell capacity sizes (`MONSTER_SIZE`, DGROUP `1B20h`): every monster has size 1 except the large ones with size 3 (so only one of them plus at most one small monster share a cell): 17 Giant Spider, 24 Mini Dragon, 25 Plasmoid, 67 Green Dragon, 68 Jouster, 74 Great Hydra, 76 Kudo Crab, 80 Dragon Lord, 87 Top Jouster.
A monster's ranged attack (`monstersAttack`) loads the projectile animation `pow%d.icn` (by `MONDMGT`), plays a sound and then simply runs a normal `doMonsterTurn` for that monster, i.e. the same to-hit, damage and special-attack rules as melee.

## What a killed monster gives (`attack2`, `49B8B`)

When a monster's hit points reach 0: the party gets `MONEXP` (`giveExperience`, split over the living party), the monster's `MONGOLD` and `MONGEMS` are added to the pending treasure
(`word_376EA` gold, `word_376F8` gems; shown and paid by `giveTreasure` after the fight), and, if its treasure class `MONTREA` is `c` > 0, with probability `TREASURE_CHANCE[c]` percent
(table at DGROUP `4B23h`: class 1 = 1%, 2 = 2%, 3 = 5%, 4 = 10%, 5 = 20%, 6 = 100%) one item is generated with `generateItem(level c)` (at most 10 items per fight, `byte_37710`).
No gold, gems or items are given on map 106 (6Ah, the arena).  The dead monster is removed by moving it to position (128,128) (off the map).
`giveExperience(xp)` (`49FA3`): two passes over the members of the combat party (or the whole party outside combat): the first only counts them, the second adds `xp / count` (integer division) to each character's stored experience (+12Bh). The disassembly calls `worstCondition` but ignores its result, so no explicit exclusion of dead members is visible in this routine.

Turn order (`setSpeedTable` `4B5AF`, `nextChar` `4AFAB`, `charsCantAct` `4AF57`; inferred from the calls): at the start of a round the party members (by `getStat(char, speed)`) and the up to three visible monster groups (by `MONSPD`) are put in a table sorted by speed (arrays at DGROUP `AD54h` and a 24-byte companion); `nextChar` hands out the next acting character (`highlightChar`) and, when the next entry is a monster group, runs `doMonsterTurn` for it (`byte_2883F` = current party slot, FFh = none yet); characters whose `worstCondition` makes them unable to act are skipped (`charsCantAct` tests the condition class with a 5-way switch on `worstCondition - 0Bh`) and `checkPartyDead` runs after every monster turn.

## The Arena (`arenaEvent`, `3E113`; event `DoTownEvent` building for map 106)

`combat.m` / "The Arena": the event is called with a difficulty value `d`.  It fills the live monster table (`Maze_mon*` arrays) with fresh monsters in the 6..12 x 6..12 area of the arena map (map 106, 6Ah):
a first group of `rnd(1, party size)` monsters of type `d` (the chosen opponent), a second group of `rnd(1, party size)` monsters of a random type `rnd(1, d - 1)` (capped at 80), a third group of type `rnd(1, d - 1) / 2` and so on; picture
files are loaded for each type with `sprintf("%s.mon", MON_PIC_NAMES[type])`.  After the fight the "%u Arena Wins" award counter (award 21) is incremented; no loot is paid in the arena (see `attack2`).
(Structure by code reading; the exact number of groups and the reward amounts were not traced.)

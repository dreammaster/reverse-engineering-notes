/* Hosts that replace translated routines with the readable versions of logic.c (the names are listed in gen/readable.txt). */
#include "game.h"
#include "logic.h"

void call_sub_27F5E(Cpu *c);
void call_monstersAttack(Cpu *c);
static Cpu *hook_cpu; /* the CPU of the host call in progress: the hooks run translated routines on its stack */
static void hook_ranged_attack(unsigned type_offset, unsigned x, unsigned y) { uint16_t args[3] = { (uint16_t)type_offset, (uint16_t)x, (uint16_t)y }; game_call(call_sub_27F5E, hook_cpu, args, 3); }
static void hook_monsters_attack(void) { game_call(call_monstersAttack, hook_cpu, NULL, 0); }
static const Mm3Hooks hooks = { hook_ranged_attack, hook_monsters_attack };
static Mm3Game game(void) { Mm3Game g = { DG, MEM, &hooks }; return g; }
static const Mm3Character *near_char(Cpu *c, int n) { return (const Mm3Character *)(DG + host_arg(c, n)); }
static void ret32(Cpu *c, uint32_t v) { c->ax = (uint16_t)v; c->dx = (uint16_t)(v >> 16); }

static void impl_getCurrentExperience(Cpu *c) { Mm3Game g = game(); ret32(c, mm3_experience_total(&g, near_char(c, 0))); }
static void impl_nextExperienceLevel(Cpu *c) { Mm3Game g = game(); ret32(c, mm3_experience_for_next_level(&g, near_char(c, 0))); }
static void impl_experienceToNextLevel(Cpu *c) { Mm3Game g = game(); ret32(c, mm3_experience_needed(&g, near_char(c, 0))); }
static void impl_giveExperience(Cpu *c) { Mm3Game g = game(); mm3_give_experience(&g, host_arg(c, 0) | ((uint32_t)host_arg(c, 1) << 16)); }

/* ---- maze */
static void impl_mazeNeighbourSlot(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_maze_neighbour_slot(&g, host_arg(c, 0)); }
static void impl_mazeGetWordRel(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_maze_word(&g, (int16_t)host_arg(c, 0), (int16_t)host_arg(c, 1), host_arg(c, 2)); }
static void impl_mazeGetWordWrap(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_maze_word_wrapped(&g, (int16_t)host_arg(c, 0), (int16_t)host_arg(c, 1), host_arg(c, 2)); }
static void impl_mazeGetFlagsRel(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_maze_flags(&g, (int16_t)host_arg(c, 0), (int16_t)host_arg(c, 1), host_arg(c, 2)); }
static void impl_mazeSetBits(Cpu *c) { Mm3Game g = game(); mm3_maze_set_bits(&g, (int16_t)host_arg(c, 0), (int16_t)host_arg(c, 1), host_arg(c, 2), host_arg(c, 3)); }
static void impl_markCellVisited(Cpu *c) { Mm3Game g = game(); mm3_maze_mark_visited(&g, (int16_t)host_arg(c, 0), (int16_t)host_arg(c, 1)); }
static void impl_isCellVisited(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_maze_is_visited(&g, (int16_t)host_arg(c, 0), (int16_t)host_arg(c, 1)); }
static void impl_setBit(Cpu *c) { mm3_set_bit(DG + host_arg(c, 0), host_arg(c, 1), host_arg(c, 2) != 0); }
static void impl_isBitSet(Cpu *c) { c->ax = (uint16_t)mm3_is_bit_set(DG + host_arg(c, 0), host_arg(c, 1)); }

static void impl_worstCondition(Cpu *c) { c->ax = (uint16_t)mm3_worst_condition(near_char(c, 0)); }
static void impl_checkPartyDead(Cpu *c) { Mm3Game g = game(); (void)c; mm3_check_party_dead(&g); }
static void impl_allHaveGone(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_all_have_gone(&g); }
static void impl_charsCantAct(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_chars_cant_act(&g); }
static void impl_subtractHitPoints(Cpu *c) { Mm3Game g = game(); mm3_subtract_hit_points(&g, (Mm3Character *)(DG + host_arg(c, 0)), (int16_t)host_arg(c, 1)); }

static void impl_getWeaponDamage(Cpu *c) { Mm3Game g = game(); mm3_weapon_damage(&g, (const Mm3Character *)(DG + host_arg(c, 0)), host_arg(c, 1) != 0); }
static void impl_hitMonster(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_hit_monster(&g, (const Mm3Character *)(DG + host_arg(c, 0)), host_arg(c, 1) != 0); }
static void impl_charSavingThrow(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_saving_throw(&g, (const Mm3Character *)(DG + host_arg(c, 0)), (int16_t)host_arg(c, 1)); }

static void impl_checkClasses(Cpu *c) { mm3_check_classes((const int8_t *)(DG + host_arg(c, 0)), DG + host_arg(c, 1)); }
static void impl_rollAttributes(Cpu *c) { mm3_roll_attributes((int8_t *)(DG + host_arg(c, 0)), DG + host_arg(c, 1)); }
static void impl_getThievery(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_thievery(&g, near_char(c, 0)); }

static void impl_itemPrice(Cpu *c) { Mm3Game g = game(); ret32(c, mm3_item_price(&g, near_char(c, 0), (int16_t)host_arg(c, 1), (int16_t)host_arg(c, 2), host_arg(c, 3))); }

static void impl_getMonsterResistance(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_monster_resistance(&g, (int16_t)host_arg(c, 0)); }
static void impl_Spells_subSpellCost(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_spend_spell_cost(&g, (Mm3Character *)(DG + host_arg(c, 0)), (int16_t)host_arg(c, 1)); }
static void impl_moveMonsterBy(Cpu *c) { Mm3Game g = game(); mm3_move_monster_by(&g, (int16_t)host_arg(c, 0), (int16_t)host_arg(c, 1), host_arg(c, 2)); }

static void impl_stopAttack(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_line_clear(&g, (int16_t)host_arg(c, 0), (int16_t)host_arg(c, 1)); }
static void impl_setSpeedTable(Cpu *c) { (void)c; Mm3Game g = game(); mm3_set_speed_table(&g); }
static void impl_moveMonsters(Cpu *c) { Mm3Game g = game(); hook_cpu = c; mm3_move_monsters(&g); }

/* host entry points: with MM3_SHADOW=1 every call is also run through the translated original and the results compared (game_diff.c) */
void host_getCurrentExperience(Cpu *c) { game_shadow("getCurrentExperience", impl_getCurrentExperience, c, 1, 2); }
void host_nextExperienceLevel(Cpu *c) { game_shadow("nextExperienceLevel", impl_nextExperienceLevel, c, 1, 2); }
void host_experienceToNextLevel(Cpu *c) { game_shadow("experienceToNextLevel", impl_experienceToNextLevel, c, 1, 2); }
void host_giveExperience(Cpu *c) { game_shadow("giveExperience", impl_giveExperience, c, 2, 0); }
void host_mazeNeighbourSlot(Cpu *c) { game_shadow("mazeNeighbourSlot", impl_mazeNeighbourSlot, c, 1, 1); }
void host_mazeGetWordRel(Cpu *c) { game_shadow("mazeGetWordRel", impl_mazeGetWordRel, c, 3, 1); }
void host_mazeGetWordWrap(Cpu *c) { game_shadow("mazeGetWordWrap", impl_mazeGetWordWrap, c, 3, 1); }
void host_mazeGetFlagsRel(Cpu *c) { game_shadow("mazeGetFlagsRel", impl_mazeGetFlagsRel, c, 3, 1); }
void host_mazeSetBits(Cpu *c) { game_shadow("mazeSetBits", impl_mazeSetBits, c, 4, 0); }
void host_markCellVisited(Cpu *c) { game_shadow("markCellVisited", impl_markCellVisited, c, 2, 0); }
void host_isCellVisited(Cpu *c) { game_shadow("isCellVisited", impl_isCellVisited, c, 2, 1); }
void host_setBit(Cpu *c) { game_shadow("setBit", impl_setBit, c, 3, 0); }
void host_isBitSet(Cpu *c) { game_shadow("isBitSet", impl_isBitSet, c, 2, 1); }
void host_worstCondition(Cpu *c) { game_shadow("worstCondition", impl_worstCondition, c, 1, 1); }
void host_checkPartyDead(Cpu *c) { game_shadow("checkPartyDead", impl_checkPartyDead, c, 0, 0); }
void host_allHaveGone(Cpu *c) { game_shadow("allHaveGone", impl_allHaveGone, c, 0, 1); }
void host_charsCantAct(Cpu *c) { game_shadow("charsCantAct", impl_charsCantAct, c, 0, 1); }
void host_subtractHitPoints(Cpu *c) { game_shadow("subtractHitPoints", impl_subtractHitPoints, c, 2, 0); }
void host_getWeaponDamage(Cpu *c) { game_shadow("getWeaponDamage", impl_getWeaponDamage, c, 2, 0); }
void host_hitMonster(Cpu *c) { game_shadow("hitMonster", impl_hitMonster, c, 2, 1); }
void host_charSavingThrow(Cpu *c) { game_shadow("charSavingThrow", impl_charSavingThrow, c, 2, 1); }
void host_checkClasses(Cpu *c) { game_shadow("checkClasses", impl_checkClasses, c, 2, 0); }
void host_rollAttributes(Cpu *c) { game_shadow("rollAttributes", impl_rollAttributes, c, 2, 0); }
void host_getThievery(Cpu *c) { game_shadow("getThievery", impl_getThievery, c, 1, 1); }
void host_itemPrice(Cpu *c) { game_shadow("itemPrice", impl_itemPrice, c, 4, 2); }
void host_getMonsterResistance(Cpu *c) { game_shadow("getMonsterResistance", impl_getMonsterResistance, c, 1, 1); }
void host_Spells_subSpellCost(Cpu *c) { game_shadow("Spells_subSpellCost", impl_Spells_subSpellCost, c, 2, 1); }
void host_moveMonsterBy(Cpu *c) { game_shadow("moveMonsterBy", impl_moveMonsterBy, c, 3, 0); }
void host_stopAttack(Cpu *c) { game_shadow("stopAttack", impl_stopAttack, c, 2, 1); }
void host_setSpeedTable(Cpu *c) { game_shadow("setSpeedTable", impl_setSpeedTable, c, 0, 0); }
void host_moveMonsters(Cpu *c) { game_shadow("moveMonsters", impl_moveMonsters, c, 0, 0); }

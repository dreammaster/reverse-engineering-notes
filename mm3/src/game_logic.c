/* Hosts that replace translated routines with the readable versions of logic.c (the names are listed in gen/readable.txt). */
#include "game.h"
#include "logic.h"

static Mm3Game game(void) { Mm3Game g = { DG }; return g; }
static const Mm3Character *near_char(Cpu *c, int n) { return (const Mm3Character *)(DG + host_arg(c, n)); }
static void ret32(Cpu *c, uint32_t v) { c->ax = (uint16_t)v; c->dx = (uint16_t)(v >> 16); }

void host_getCurrentExperience(Cpu *c) { Mm3Game g = game(); ret32(c, mm3_experience_total(&g, near_char(c, 0))); }
void host_nextExperienceLevel(Cpu *c) { Mm3Game g = game(); ret32(c, mm3_experience_for_next_level(&g, near_char(c, 0))); }
void host_experienceToNextLevel(Cpu *c) { Mm3Game g = game(); ret32(c, mm3_experience_needed(&g, near_char(c, 0))); }
void host_giveExperience(Cpu *c) { Mm3Game g = game(); mm3_give_experience(&g, host_arg(c, 0) | ((uint32_t)host_arg(c, 1) << 16)); }

/* ---- maze */
void host_mazeNeighbourSlot(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_maze_neighbour_slot(&g, host_arg(c, 0)); }
void host_mazeGetWordRel(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_maze_word(&g, (int16_t)host_arg(c, 0), (int16_t)host_arg(c, 1), host_arg(c, 2)); }
void host_mazeGetWordWrap(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_maze_word_wrapped(&g, (int16_t)host_arg(c, 0), (int16_t)host_arg(c, 1), host_arg(c, 2)); }
void host_mazeGetFlagsRel(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_maze_flags(&g, (int16_t)host_arg(c, 0), (int16_t)host_arg(c, 1), host_arg(c, 2)); }
void host_mazeSetBits(Cpu *c) { Mm3Game g = game(); mm3_maze_set_bits(&g, (int16_t)host_arg(c, 0), (int16_t)host_arg(c, 1), host_arg(c, 2), host_arg(c, 3)); }
void host_markCellVisited(Cpu *c) { Mm3Game g = game(); mm3_maze_mark_visited(&g, (int16_t)host_arg(c, 0), (int16_t)host_arg(c, 1)); }
void host_isCellVisited(Cpu *c) { Mm3Game g = game(); c->ax = (uint16_t)mm3_maze_is_visited(&g, (int16_t)host_arg(c, 0), (int16_t)host_arg(c, 1)); }
void host_setBit(Cpu *c) { mm3_set_bit(DG + host_arg(c, 0), host_arg(c, 1), host_arg(c, 2) != 0); }
void host_isBitSet(Cpu *c) { c->ax = (uint16_t)mm3_is_bit_set(DG + host_arg(c, 0), host_arg(c, 1)); }

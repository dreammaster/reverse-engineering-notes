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

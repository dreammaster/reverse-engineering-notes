/*
 * describe.h - the class-specific part of a look text (1000:84D9): spells, potions, weapons and ammunition; verified against the original (tests/vectors_describe.h).
 */
#ifndef CADAVER_DESCRIBE_H
#define CADAVER_DESCRIBE_H

#include <stddef.h>
#include <stdint.h>
#include "lng.h"

/* The 100 + 100 name table entries of a level's MP1 (spell and potion name -> LNG string id; see docs/formats.md). Returns 1 when the table was found. */
int describe_load_tables(const uint8_t *mp1, size_t len, uint16_t spell_str[100], uint16_t potion_str[100]);

/* cls = sprite header byte 16h, a = the item's A struct; for class 4 `ammo_a` is the A struct of the ammunition thing A[0..1] and `ammo_name` its name string id.
 * Returns 0 when the class has no description of its own. English word order (ds:70B = 0). */
int item_description(const LngTable *lng, const uint16_t spell_str[100], const uint16_t potion_str[100], int cls, const uint8_t *a,
                     const uint8_t *ammo_a, int ammo_name, char *out, size_t cap);

#endif

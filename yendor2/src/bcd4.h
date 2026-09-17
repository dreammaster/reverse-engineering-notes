#ifndef YENDOR2_BCD4_H
#define YENDOR2_BCD4_H

#include <stdbool.h>
#include <stdint.h>

/*
 * 4-byte (8-digit) packed BCD counter, most-significant digit pair first.
 * Reimplements SW.EXE's AddBCD4/SubBCD4/CompareBCD4/ConvertWordToBCD4/
 * AddToBCDCounter/SubtractFromBCDCounter/IsBCDCounterAtLeast family
 * (yendor2.asm:16803-17382) -- the engine behind gold and other large
 * in-game counters. Confirmed packed-BCD via the original's DAA/DAS
 * opcodes.
 */
typedef uint8_t Bcd4[4];

/* dst += src, packed-BCD addition (was AddBCD4). */
void bcd4Add(Bcd4 dst, const Bcd4 src);

/* dst -= src, packed-BCD subtraction (was SubBCD4). */
void bcd4Sub(Bcd4 dst, const Bcd4 src);

/* Returns <0, 0, >0 as a < b, a == b, a > b (was CompareBCD4). */
int bcd4Compare(const Bcd4 a, const Bcd4 b);

/* Converts a 16-bit binary value to packed BCD (was ConvertWordToBCD4). */
void bcd4FromU16(Bcd4 out, uint16_t value);

/* counter += amount (was AddToBCDCounter). */
void bcd4AddU16(Bcd4 counter, uint16_t amount);

/* counter -= amount (was SubtractFromBCDCounter). */
void bcd4SubU16(Bcd4 counter, uint16_t amount);

/* True if counter >= threshold (was IsBCDCounterAtLeast). */
bool bcd4AtLeastU16(const Bcd4 counter, uint16_t threshold);

#endif

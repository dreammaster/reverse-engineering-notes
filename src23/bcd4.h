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

/*
 * dst -= src, clamped at 0 rather than underflowing if src >= dst
 * (was SpendMaterialCounterClamped, yendor2.asm:13939, instruction-
 * identical in Chapter 3): a material-counter (gold/ore) spend that
 * can't be fully covered zeroes the counter outright instead of going
 * negative. Returns true if the counter was clamped, false if the
 * normal subtraction was used. The original's own gate is strictly
 * dst > src (a `ja`, not `jae`) -- an exact match (dst == src) is
 * still treated as "can't cover it" and takes the clamp path, even
 * though a plain subtraction would land on the same all-zero result
 * either way; reproduced exactly rather than smoothed over, since it
 * changes which path a real caller's "resource depleted" UI hook
 * fires on.
 */
bool bcd4SubClamped(Bcd4 dst, const Bcd4 src);

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

/*
 * Shifts the whole 8-digit value left one digit, i.e. x10; the top digit
 * is discarded on overflow (was ShiftBCD4LeftNibble).
 */
void bcd4ShiftLeftNibble(Bcd4 value);

/* Shifts right one digit, i.e. truncating /10 (was ShiftBCD4RightNibble). */
void bcd4ShiftRightNibble(Bcd4 value);

/*
 * value = value * percent / 100, rounded half-up on the low three digits
 * only (was MulBCD4ByWord). Used for barter discounts/markups with
 * percent = 100 +/- a skill-tiered adjustment. Faithful to the original's
 * quirks: each digit*percent partial product is truncated to 16 bits
 * (so percent above 7281 can overflow), and digits carried past the top
 * of the 8-digit range are silently lost. Chapter 2 and Chapter 3's
 * copies are identical. The original also left scratch globals
 * (word_3293E etc.) behind, which nothing reads afterwards.
 */
void bcd4MulPercent(Bcd4 value, uint16_t percent);

#endif

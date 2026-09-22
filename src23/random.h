#ifndef YENDOR23_RANDOM_H
#define YENDOR23_RANDOM_H

#include <stdint.h>

/*
 * A faithful port of RandomInRange (yendor2.asm:41476), the game's only
 * random source. It is a small linear-congruential generator seeded from the
 * DOS clock on its first call (INT 21h AH=2Ch, seconds and hundredths).
 *
 * From the disassembly (not checked by running the original): the result is
 * in 0..bound *inclusive*, not 0..bound-1. It masks a 16-bit draw down to the
 * smallest power-of-two range covering bound, keeps it if it is <= bound, and
 * otherwise subtracts bound, so 1..(mask-bound) come up slightly more often.
 * bound 0 returns 0. This matches how the data uses it (an effect defined as
 * 1..10 is RandomInRange(9) + 1, which reaches 10).
 *
 * The engine port is free to use its own RNG; this exists so the exact
 * behavior is documented and testable.
 */
typedef struct {
    uint16_t seed;      /* word_3CC76; 0 until the first call */
    uint16_t clockWord; /* (seconds << 8) | hundredths, read on the first call */
} RandomState;

/* Starts a fresh generator that will seed itself from the given clock reading on first use. */
void randomStart(RandomState *rng, uint8_t seconds, uint8_t hundredths);

uint16_t randomInRange(RandomState *rng, uint16_t bound);

#endif

#include "random.h"

void randomStart(RandomState *rng, uint8_t seconds, uint8_t hundredths) {
    rng->seed = 0;
    rng->clockWord = (uint16_t)((seconds << 8) | hundredths);
}

uint16_t randomInRange(RandomState *rng, uint16_t bound) {
    if (bound == 0) {
        return 0;
    }

    uint16_t ax = rng->seed != 0 ? rng->seed : rng->clockWord;
    ax = (uint16_t)(0u - ax);
    uint32_t product = (uint32_t)ax * 0x805u;
    uint16_t high = (uint16_t)(product >> 16);
    uint16_t low = (uint16_t)product;
    ax = (uint16_t)((low << 8) | (low >> 8)); /* xchg ah, al */
    rng->seed = ax;

    /* sub dx, seed / js: skip the +1 when the 16-bit difference is negative. */
    if (!((uint16_t)(high - rng->seed) & 0x8000)) {
        ax = (uint16_t)(ax + 1);
    }

    /* The original counts shl steps until a carry-out, i.e. the leading zeros of bound. */
    unsigned leadingZeros = 0;
    for (uint16_t probe = bound; !(probe & 0x8000); probe = (uint16_t)(probe << 1)) {
        leadingZeros++;
    }

    ax = (uint16_t)((uint16_t)(ax << leadingZeros) >> leadingZeros);
    if (ax <= bound) {
        return ax;
    }
    return (uint16_t)(ax - bound);
}

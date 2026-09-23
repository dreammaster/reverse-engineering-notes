#include "globalflags.h"

/*
 * Mirrors GetGlobalFlagBitAndWord exactly: index is 1-based, wordIndex =
 * (index-1)/16, bitInWord (0=MSB) = (index-1)%16. Deriving it this way
 * (rather than porting the original's zero-remainder special case
 * literally) gives the same word/bit for every index, including the
 * multiples of 16 the original steps back a word to reach.
 */
static bool flagBitAndWord(size_t size, unsigned index, size_t *outWordOffset, uint16_t *outMask) {
    if (index == 0) {
        return false;
    }
    unsigned zeroBased = index - 1;
    size_t wordOffset = (zeroBased / 16) * 2;
    unsigned bitInWord = zeroBased % 16;
    if (wordOffset + 2 > size) {
        return false;
    }
    *outWordOffset = wordOffset;
    *outMask = (uint16_t)(0x8000 >> bitInWord);
    return true;
}

bool globalFlagTest(const uint8_t *buffer, size_t size, unsigned index) {
    size_t wordOffset;
    uint16_t mask;
    if (!flagBitAndWord(size, index, &wordOffset, &mask)) {
        return false;
    }
    uint16_t word = (uint16_t)(buffer[wordOffset] | (buffer[wordOffset + 1] << 8));
    return (word & mask) != 0;
}

void globalFlagSet(uint8_t *buffer, size_t size, unsigned index) {
    size_t wordOffset;
    uint16_t mask;
    if (!flagBitAndWord(size, index, &wordOffset, &mask)) {
        return;
    }
    uint16_t word = (uint16_t)(buffer[wordOffset] | (buffer[wordOffset + 1] << 8));
    word |= mask;
    buffer[wordOffset] = (uint8_t)word;
    buffer[wordOffset + 1] = (uint8_t)(word >> 8);
}

void globalFlagClear(uint8_t *buffer, size_t size, unsigned index) {
    size_t wordOffset;
    uint16_t mask;
    if (!flagBitAndWord(size, index, &wordOffset, &mask)) {
        return;
    }
    uint16_t word = (uint16_t)(buffer[wordOffset] | (buffer[wordOffset + 1] << 8));
    word &= (uint16_t)~mask;
    buffer[wordOffset] = (uint8_t)word;
    buffer[wordOffset + 1] = (uint8_t)(word >> 8);
}

void globalFlagApplySigned(uint8_t *buffer, size_t size, int16_t signedIndex) {
    if (signedIndex > 0) {
        globalFlagSet(buffer, size, (unsigned)signedIndex);
    } else if (signedIndex < 0) {
        globalFlagClear(buffer, size, (unsigned)(-signedIndex));
    }
}

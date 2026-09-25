#include "bcd4.h"

void bcd4Add(Bcd4 dst, const Bcd4 src) {
    int carry = 0;
    for (int i = 3; i >= 0; i--) {
        /* ADD/ADC followed by DAA, per the x86 definition of DAA. */
        unsigned sum = (unsigned)dst[i] + src[i] + carry;
        int cf = sum > 0xFF;
        int af = ((dst[i] & 0x0F) + (src[i] & 0x0F) + carry) > 0x0F;
        uint8_t al = (uint8_t)sum;
        uint8_t oldAl = al;
        int oldCf = cf;

        if ((al & 0x0F) > 9 || af) {
            al = (uint8_t)(al + 0x06);
        }
        if (oldAl > 0x99 || oldCf) {
            al = (uint8_t)(al + 0x60);
            cf = 1;
        } else {
            cf = 0;
        }
        dst[i] = al;
        carry = cf;
    }
}

void bcd4Sub(Bcd4 dst, const Bcd4 src) {
    int borrow = 0;
    for (int i = 3; i >= 0; i--) {
        int rawLowNibble = (dst[i] & 0x0F) - (src[i] & 0x0F) - borrow;
        int raw = dst[i] - src[i] - borrow;
        int cf = (raw < 0) ? 1 : 0;
        int af = (rawLowNibble < 0) ? 1 : 0;
        uint8_t al = (uint8_t)raw;

        if ((al & 0x0F) > 9 || af) {
            al = (uint8_t)(al - 6);
            af = 1;
        }
        if (al > 0x9F || cf) {
            al = (uint8_t)(al - 0x60);
            cf = 1;
        }
        dst[i] = al;
        borrow = cf;
    }
}

bool bcd4SubClamped(Bcd4 dst, const Bcd4 src) {
    if (bcd4Compare(dst, src) > 0) {
        bcd4Sub(dst, src);
        return false;
    }
    dst[0] = dst[1] = dst[2] = dst[3] = 0;
    return true;
}

int bcd4Compare(const Bcd4 a, const Bcd4 b) {
    /*
     * The original compares each byte's high nibble then low nibble
     * separately, but since packed-BCD nibbles only ever hold 0-9, that
     * is equivalent to a plain byte-wise comparison, MSD-first.
     */
    for (int i = 0; i < 4; i++) {
        if (a[i] != b[i]) {
            return (a[i] < b[i]) ? -1 : 1;
        }
    }
    return 0;
}

void bcd4FromU16(Bcd4 out, uint16_t value) {
    uint8_t digit[8] = {0};
    for (int i = 7; i >= 0 && value > 0; i--) {
        digit[i] = (uint8_t)(value % 10);
        value = (uint16_t)(value / 10);
    }
    for (int i = 0; i < 4; i++) {
        out[i] = (uint8_t)((digit[i * 2] << 4) | digit[i * 2 + 1]);
    }
}

void bcd4AddU16(Bcd4 counter, uint16_t amount) {
    Bcd4 encoded;
    bcd4FromU16(encoded, amount);
    bcd4Add(counter, encoded);
}

void bcd4SubU16(Bcd4 counter, uint16_t amount) {
    Bcd4 encoded;
    bcd4FromU16(encoded, amount);
    bcd4Sub(counter, encoded);
}

bool bcd4AtLeastU16(const Bcd4 counter, uint16_t threshold) {
    Bcd4 encoded;
    bcd4FromU16(encoded, threshold);
    return bcd4Compare(counter, encoded) >= 0;
}

static uint32_t bcd4Load(const Bcd4 value) {
    return ((uint32_t)value[0] << 24) | ((uint32_t)value[1] << 16) |
           ((uint32_t)value[2] << 8) | (uint32_t)value[3];
}

static void bcd4Store(Bcd4 value, uint32_t bits) {
    value[0] = (uint8_t)(bits >> 24);
    value[1] = (uint8_t)(bits >> 16);
    value[2] = (uint8_t)(bits >> 8);
    value[3] = (uint8_t)bits;
}

void bcd4ShiftLeftNibble(Bcd4 value) {
    bcd4Store(value, bcd4Load(value) << 4);
}

void bcd4ShiftRightNibble(Bcd4 value) {
    bcd4Store(value, bcd4Load(value) >> 4);
}

static void bcd4AddShiftedProduct(Bcd4 acc, unsigned digit, uint16_t percent, int shift) {
    Bcd4 term;
    bcd4FromU16(term, (uint16_t)(digit * percent));
    for (int i = 0; i < shift; i++) {
        bcd4ShiftLeftNibble(term);
    }
    bcd4Add(acc, term);
}

void bcd4MulPercent(Bcd4 value, uint16_t percent) {
    unsigned digit[8];
    for (int i = 0; i < 4; i++) {
        digit[i * 2] = value[i] >> 4;
        digit[i * 2 + 1] = value[i] & 0x0F;
    }

    /*
     * The low three digits (weights 1, 10, 100) are accumulated first,
     * with +50 rounding folded into the units term, then shifted right two
     * digits (/100). The five higher digits (weights 10^3..10^7) already
     * include the /100, so they're added afterwards at weights 10^1..10^5.
     */
    Bcd4 acc;
    bcd4FromU16(acc, (uint16_t)(digit[7] * percent + 50));
    bcd4AddShiftedProduct(acc, digit[6], percent, 1);
    bcd4AddShiftedProduct(acc, digit[5], percent, 2);
    bcd4ShiftRightNibble(acc);
    bcd4ShiftRightNibble(acc);
    for (int shift = 1; shift <= 5; shift++) {
        bcd4AddShiftedProduct(acc, digit[5 - shift], percent, shift);
    }

    for (int i = 0; i < 4; i++) {
        value[i] = acc[i];
    }
}

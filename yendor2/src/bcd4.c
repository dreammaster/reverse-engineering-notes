#include "bcd4.h"

void bcd4Add(Bcd4 dst, const Bcd4 src) {
    int carry = 0;
    for (int i = 3; i >= 0; i--) {
        int sum = dst[i] + src[i] + carry;
        carry = 0;
        if ((sum & 0x0F) > 9) {
            sum += 0x06;
        }
        if (sum > 0x9F) {
            sum += 0x60;
            carry = 1;
        }
        dst[i] = (uint8_t)sum;
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

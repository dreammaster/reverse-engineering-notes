// The Wizardry PRNG.
//
// Ported from the Apple assembly primitive RANDOM (P01001C,
// sources/WizardryCode/wiz1d/random.txt): a 32-bit shift register held in
// four bytes ($47A,$4FA,$57A,$5FB), advanced 7 bit-positions per call with a
// two-tap XOR feedback, returning a 15-bit value 0..32767.
//
// The DOS SYSTEM.INTERP services RANDOM as UNITREAD(consoleUnit, buf, 0,
// block=10, 0) and its x86 code is a faithful port of the same LFSR (feedback
// bytes at 0x622/0x623, state at 0x630/0xE21C) -- an exact-sequence
// cross-check against the running interpreter is still TODO.
#pragma once
#include "wiz/types.h"

namespace wiz {

class Rng {
public:
    explicit Rng(u32 seed = 0x1D8B2F41u) { reseed(seed); }

    void reseed(u32 seed) {
        b_[0] = u8(seed);
        b_[1] = u8(seed >> 8);
        b_[2] = u8(seed >> 16);
        b_[3] = u8(seed >> 24);
        if ((b_[0] | b_[1] | b_[2] | b_[3]) == 0) b_[0] = 1;   // avoid all-zero
    }

    // One RANDOM call: 0..32767.
    u16 next() {
        for (int i = 0; i < 7; ++i) {
            int tapLo = (b_[0] >> 6) & 1;          // N after `ASL $47A`
            int tapHi = (b_[3] >> 6) & 1;          // N after `ROL $5FB`
            int c = (b_[0] >> 7) & 1;
            b_[0] = u8(b_[0] << 1);
            int c1 = (b_[1] >> 7) & 1; b_[1] = u8((b_[1] << 1) | c); c = c1;
            int c2 = (b_[2] >> 7) & 1; b_[2] = u8((b_[2] << 1) | c); c = c2;
            b_[3] = u8((b_[3] << 1) | c);
            if (tapLo ^ tapHi) b_[0] |= 1;         // `INC $47A` feedback
        }
        return u16(((b_[0] >> 1) << 8) | b_[2]);
    }

    // RANDOM MOD n, the game's idiom for a bounded roll.
    int mod(int n) { return n > 0 ? int(next() % u16(n)) : 0; }

private:
    u8 b_[4];
};

} // namespace wiz

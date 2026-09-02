// The DOS Wizardry PRNG -- reverse-engineered from SYSTEM.INTERP.
//
// RANDOM is WIZARDRY p-code proc 34, which calls UNITREAD(unit 13, buf, 0,
// subfn 10, ...).  That SBIOS entry (SYSTEM.INTERP @ 0x221E) is:
//
//   s0 = s0*0x6A2D + 0x3619        s1 = s1*0xFFF1 + 0xFF8B     (mod 2^16)
//   s2 = s2*0xFFAF + 0x0183        s3 = s3*0xFFD9 + 0x7FC9
//
//   bx  = (s0 << 4) & 0xFF00
//   bx ^= (s2 >> 4) & 0x00FF
//   bx ^= s1 & 0x0FF0
//   ax  = byteswap(s3) & 0xF00F                     ; `rol ax,8`
//   bx ^= ax                                        ; bx now = the full mix
//   ax &= 0x7FFF
//   result := ax                                    ; *** stores AX, not BX ***
//
// The interpreter computes a 4-state mix in BX and then returns AX -- a
// shipped bug.  The effective output is therefore  byteswap(s3) & 0x700F  :
// only s3 matters, giving 128 distinct values on a period-65536 cycle.  This
// is the notoriously weak PC-Wizardry RNG; we reproduce it exactly.
//
// No keyboard / timer entropy feeds this path (that is unit 1/2, used by
// GETKEY).  The image ships fixed initial state {0x5BAB,0xD02B,0x7E15,0x7351}.
#pragma once
#include "wiz/types.h"

namespace wiz {

class Rng {
public:
    Rng() { reseed(); }
    // Only s3 affects the output; pass just that to explore the 65536 cycle.
    explicit Rng(u16 s3) { reseed(0x5BAB, 0xD02B, 0x7E15, s3); }

    void reseed(u16 s0 = 0x5BAB, u16 s1 = 0xD02B, u16 s2 = 0x7E15, u16 s3 = 0x7351) {
        s_[0] = s0; s_[1] = s1; s_[2] = s2; s_[3] = s3;
    }

    // One RANDOM call.  All four LCGs advance (their cross-terms feed BX,
    // which the interpreter discards), but only s3 reaches the result:
    // 0..32767 nominal, 128 distinct values, period 65536.
    u16 next() {
        s_[0] = u16(s_[0] * 0x6A2Du + 0x3619u);
        s_[1] = u16(s_[1] * 0xFFF1u + 0xFF8Bu);
        s_[2] = u16(s_[2] * 0xFFAFu + 0x0183u);
        s_[3] = u16(s_[3] * 0xFFD9u + 0x7FC9u);
        u16 ax = u16(((s_[3] << 8) | (s_[3] >> 8)) & 0xF00F);   // rol s3,8
        return u16(ax & 0x7FFF);                                // == ax & 0x700F
    }

    // RANDOM MOD n -- the game's idiom for a bounded roll.
    int mod(int n) { return n > 0 ? int(next() % u16(n)) : 0; }

    // The "intended" mix (store BX): kept for comparison / a future toggle.
    u16 nextIntended() {
        s_[0] = u16(s_[0] * 0x6A2Du + 0x3619u);
        s_[1] = u16(s_[1] * 0xFFF1u + 0xFF8Bu);
        s_[2] = u16(s_[2] * 0xFFAFu + 0x0183u);
        s_[3] = u16(s_[3] * 0xFFD9u + 0x7FC9u);
        u16 bx = u16((s_[0] << 4) & 0xFF00);
        bx ^= u16((s_[2] >> 4) & 0x00FF);
        bx ^= u16(s_[1] & 0x0FF0);
        bx ^= u16(((s_[3] << 8) | (s_[3] >> 8)) & 0xF00F);
        return u16(bx & 0x7FFF);
    }

private:
    u16 s_[4];
};

} // namespace wiz

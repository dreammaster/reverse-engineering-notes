#include "wiz/character.h"

#include <algorithm>
#include <cstring>

namespace wiz {

namespace {
// word view over the record
inline u16 w(const u8 *p, int word) { return rd16(p + 2 * word); }
inline void setw(u8 *p, int word, u16 v) { p[2 * word] = u8(v); p[2 * word + 1] = u8(v >> 8); }

std::string readStr(const u8 *p, int wordOff, int cap) {   // UCSD STRING[cap]
    const u8 *s = p + 2 * wordOff;
    int len = std::min<int>(s[0], cap);
    std::string out;
    for (int i = 1; i <= len; ++i)
        if (s[i] >= 32 && s[i] < 127) out.push_back(char(s[i]));
    return out;
}
void writeStr(u8 *p, int wordOff, int cap, const std::string &v) {
    // UCSD MoveLeft/SAS semantics: write the length byte + chars only, leave
    // the rest of the field untouched (the game does not clear stale bytes).
    u8 *s = p + 2 * wordOff;
    int len = std::min<int>(int(v.size()), cap);
    s[0] = u8(len);
    for (int i = 0; i < len; ++i) s[1 + i] = u8(v[i]);
}

// PACKED ARRAY OF 0..31, IXP 3,5  -> 3 fields/word, 5 bits, from bit 0
int getPacked5(const u8 *p, int baseWord, int idx) {
    u16 word = w(p, baseWord + idx / 3);
    return (word >> ((idx % 3) * 5)) & 0x1F;
}
void setPacked5(u8 *p, int baseWord, int idx, int val) {
    int wi = baseWord + idx / 3, sh = (idx % 3) * 5;
    u16 word = w(p, wi);
    word = u16((word & ~(0x1Fu << sh)) | ((val & 0x1F) << sh));
    setw(p, wi, word);
}
// PACKED ARRAY OF BOOLEAN, IXP 16,1
bool getBit(const u8 *p, int baseWord, int idx) {
    return (w(p, baseWord + idx / 16) >> (idx % 16)) & 1;
}
void setBit(u8 *p, int baseWord, int idx, bool b) {
    int wi = baseWord + idx / 16, sh = idx % 16;
    u16 word = w(p, wi);
    word = u16(b ? (word | (1u << sh)) : (word & ~(1u << sh)));
    setw(p, wi, word);
}
} // namespace

void Character::read(Bytes rec) {
    std::memcpy(raw.data(), rec.p, kRecordBytes);
    const u8 *p = raw.data();

    name     = readStr(p, 0, 15);
    password = readStr(p, 8, 15);
    inMaze   = w(p, 16) != 0;
    race     = Race(w(p, 17) & 0xFF);
    cls      = Class(w(p, 18) & 0xFF);
    age      = i16(w(p, 19));
    status   = Status(w(p, 20) & 0xFF);
    align    = Align(w(p, 21) & 0xFF);

    for (int i = 0; i < ATTR_COUNT; ++i) attrib[i] = getPacked5(p, 22, i);
    for (int i = 0; i < 5; ++i)          luckSkill[i] = getPacked5(p, 24, i);

    gold.set(WizLong::read(p + 2 * 26));
    exp.set(WizLong::read(p + 2 * 62));

    possCount = i16(w(p, 29));
    for (int i = 0; i < 8; ++i) {
        int b = 30 + 4 * i;
        poss[i].equipped   = w(p, b + 0) != 0;
        poss[i].cursed     = w(p, b + 1) != 0;
        poss[i].identified = w(p, b + 2) != 0;
        poss[i].itemIndex  = i16(w(p, b + 3));
    }

    maxLevelAcquired = i16(w(p, 65));
    charLevel        = i16(w(p, 66));
    hpLeft           = i16(w(p, 67));
    hpMax            = i16(w(p, 68));
    armorClass       = i16(w(p, 88));

    for (int i = 1; i <= 50; ++i) spellKnown[i] = getBit(p, 69, i);
    for (int i = 1; i <= 7; ++i) mageSpells[i]   = i16(w(p, 73 + i - 1));
    for (int i = 1; i <= 7; ++i) priestSpells[i] = i16(w(p, 80 + i - 1));

    hpCalcMd   = i16(w(p, 87));
    healPts    = i16(w(p, 89));
    critHitM   = w(p, 90) != 0;
    swingCnt   = i16(w(p, 91));
    hpDamRc[0] = i16(w(p, 92)); hpDamRc[1] = i16(w(p, 93)); hpDamRc[2] = i16(w(p, 94));
    wepSlay    = w(p, 98);
    poison     = i16(w(p, 99));
}

std::array<u8, Character::kRecordBytes> Character::write() const {
    auto out = raw;                       // start from the original bytes
    u8 *p = out.data();

    writeStr(p, 0, 15, name);
    writeStr(p, 8, 15, password);
    setw(p, 16, inMaze ? 1 : 0);
    setw(p, 17, u16(race));
    setw(p, 18, u16(cls));
    setw(p, 19, u16(i16(age)));
    setw(p, 20, u16(status));
    setw(p, 21, u16(align));

    for (int i = 0; i < ATTR_COUNT; ++i) setPacked5(p, 22, i, attrib[i]);
    for (int i = 0; i < 5; ++i)          setPacked5(p, 24, i, luckSkill[i]);

    WizLong g = gold.pack(), e = exp.pack();
    setw(p, 26, u16(g.low)); setw(p, 27, u16(g.mid)); setw(p, 28, u16(g.high));
    setw(p, 62, u16(e.low)); setw(p, 63, u16(e.mid)); setw(p, 64, u16(e.high));

    setw(p, 29, u16(i16(possCount)));
    for (int i = 0; i < 8; ++i) {
        int b = 30 + 4 * i;
        setw(p, b + 0, poss[i].equipped ? 1 : 0);
        setw(p, b + 1, poss[i].cursed ? 1 : 0);
        setw(p, b + 2, poss[i].identified ? 1 : 0);
        setw(p, b + 3, u16(i16(poss[i].itemIndex)));
    }

    setw(p, 65, u16(i16(maxLevelAcquired)));
    setw(p, 66, u16(i16(charLevel)));
    setw(p, 67, u16(i16(hpLeft)));
    setw(p, 68, u16(i16(hpMax)));
    setw(p, 88, u16(i16(armorClass)));

    for (int i = 1; i <= 50; ++i) setBit(p, 69, i, spellKnown[i]);
    for (int i = 1; i <= 7; ++i) setw(p, 73 + i - 1, u16(i16(mageSpells[i])));
    for (int i = 1; i <= 7; ++i) setw(p, 80 + i - 1, u16(i16(priestSpells[i])));

    setw(p, 87, u16(i16(hpCalcMd)));
    setw(p, 89, u16(i16(healPts)));
    setw(p, 90, critHitM ? 1 : 0);
    setw(p, 91, u16(i16(swingCnt)));
    setw(p, 92, u16(i16(hpDamRc[0]))); setw(p, 93, u16(i16(hpDamRc[1]))); setw(p, 94, u16(i16(hpDamRc[2])));
    setw(p, 98, wepSlay);
    setw(p, 99, u16(i16(poison)));

    return out;
}

} // namespace wiz

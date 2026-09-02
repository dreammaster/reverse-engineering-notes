// TCHAR -- a Wizardry character.
//
// Native C++ model plus a reader/writer for the 208-byte (104-word) roster
// record.  Field word-offsets recovered from the DOS ROLLER p-code
// (docs/file-formats.md): NAME 0, PASSWORD 8, INMAZE 16, RACE 17, CLASS 18,
// AGE 19, STATUS 20, ALIGN 21, ATTRIB 22-23 (packed IXP 3,5), LUCKSKIL 24-25
// (packed), GOLD 26-28, POSS 29 (POSSCNT) + 30-61 (8 x 4-word slots),
// EXP 62-64, MAXLEVAC 65, CHARLEV 66, HPLEFT 67, HPMAX 68, SPELLKN 69-72
// (packed IXP 16,1), MAGESP 73-79, PRIESTSP 80-86, ARMORCL 88.  The combat
// tail (words 87,89-103) is carried through verbatim in `raw`.
#pragma once
#include "wiz/types.h"
#include <array>

namespace wiz {

enum Attr { STR = 0, IQ, PIETY, VIT, AGI, LUCK, ATTR_COUNT };

// TWIZLONG, base-10000: value = low + mid*1e4 + high*1e8.
struct WizLongV {
    int64_t v = 0;
    void set(WizLong w) { v = w.value(); }
    WizLong pack() const {
        int64_t x = v < 0 ? 0 : v;
        return {i16(x % 10000), i16((x / 10000) % 10000), i16(x / 100000000LL)};
    }
};

struct Possession {
    bool equipped = false;
    bool cursed = false;
    bool identified = false;
    int  itemIndex = 0;            // -1 / 0xFFFF = empty
};

struct Character {
    static constexpr int kRecordBytes = 208;

    std::string name;
    std::string password;
    bool inMaze = false;
    Race  race  = Race::Human;
    Class cls   = Class::Fighter;
    int   age   = 0;                // weeks
    Status status = Status::OK;
    Align align   = Align::Good;

    int attrib[ATTR_COUNT] = {};    // 0..18
    int luckSkill[5] = {16, 16, 16, 16, 16};

    WizLongV gold;
    WizLongV exp;

    int possCount = 0;
    Possession poss[8];

    int maxLevelAcquired = 1;
    int charLevel = 1;
    int hpLeft = 0;
    int hpMax  = 0;
    int armorClass = 10;

    bool spellKnown[51] = {};       // 1..50 (the game's SPELLSKN is 1-indexed;
                                    // slot 0 is unused, matching the source)
    int  mageSpells[8]  = {};       // [1..7] casts per level
    int  priestSpells[8] = {};

    // combat tail (TCHAR words 87-99) -- derived from level/str/class/gear by
    // deriveStats() (roller.h); the shipped roster records already carry them.
    int  hpCalcMd = 0;              // word 87  to-hit modifier
    int  healPts  = 0;              // word 89  per-turn regen in the maze
    bool critHitM = false;          // word 90  weapon can instant-kill
    int  swingCnt = 1;              // word 91  melee attacks per round
    int  hpDamRc[3] = {2, 2, 0};    // words 92-94  THPREC {dice, sides, +}
    u16  wepSlay  = 0;              // word 98  WEPVSTYP: slays monster class bit
    int  poison   = 0;              // word 99  LOSTXYL.POISNAMT[1]

    // the untouched 208-byte record; fields above are re-encoded over it on
    // write so the combat tail and any not-yet-modelled bits survive.
    std::array<u8, kRecordBytes> raw{};

    void read(Bytes rec);          // rec.size() must be >= kRecordBytes
    std::array<u8, kRecordBytes> write() const;

    bool lost() const { return status == Status::Lost; }
};

} // namespace wiz

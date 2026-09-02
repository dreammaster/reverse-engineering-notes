// Reader for SCENARIO.DATA -- the Wizardry scenario database (mazes, monsters,
// items, rewards, roster, xp table).  Container format decoded in
// docs/file-formats.md; record field layouts follow the Apple structs (+289
// global-word rule) and are still being validated.
#pragma once
#include "wiz/types.h"

namespace wiz {

class Scenario {
public:
    enum Type { Zero = 0, Maze, Monster, Reward, Object, Char, SpcChr, Exp, TypeCount };

    bool load(std::vector<u8> scenarioData);

    const std::string &gameName() const { return gameName_; }
    int count(Type t) const { return count_[t]; }
    int recSize(Type t) const { return recSize_[t]; }

    // Raw bytes of record `r` of type `t` (empty if out of range).
    Bytes record(Type t, int r) const;

    // TOC string tables.
    const std::vector<std::string> &races() const { return race_; }
    const std::vector<std::string> &classes() const { return class_; }
    const std::vector<std::string> &statuses() const { return status_; }
    const std::vector<std::string> &aligns() const { return align_; }

    static const char *typeName(Type t);

private:
    std::vector<u8> data_;
    std::string gameName_;
    u16 recPer2b_[TypeCount] = {};
    u16 count_[TypeCount] = {};
    u16 recSize_[TypeCount] = {};
    u16 bloff_[TypeCount] = {};
    std::vector<std::string> race_, class_, status_, align_;
};

// ---- record views -------------------------------------------------------
// Thin accessors over the raw record bytes; only the fields that are pinned
// down so far. Extend as docs/file-formats.md fills in.

struct MonsterRec {
    Bytes b;
    int pic() const { return rd16s(b.p + 0); }       // portrait index in *.MONSTERS
    // name comes from the string pool: StringPool::monsterNameKey(index)
};

// TOBJREC -- 46 bytes / 23 words.  The DOS record drops Apple's two STRING[15]
// name fields (they live in ASCII.KRN: objectNameKey(idx, 0)=unidentified,
// objectNameKey(idx, 1)=identified), so every offset is Apple's minus 16.
// Verified against DOS SHOPS procs 17/20 (BOLTAC).
struct ObjectRec {
    Bytes b;
    u16  w(int i) const   { return rd16(b.p + i * 2); }
    ObjType type() const  { return ObjType(w(0) & 0xFF); }         // word 0
    Align align() const   { return Align(w(1) & 0xFF); }           // word 1
    bool cursed() const   { return w(2) != 0; }                    // word 2
    int  special() const  { return i16(w(3)); }                    // word 3
    int  changeTo() const { return i16(w(4)); }                    // word 4
    int  chgChance() const{ return i16(w(5)); }                    // word 5
    WizLong price() const { return WizLong::read(b.p + 6 * 2); }   // words 6-8
    int  boltacXX() const { return i16(w(9)); }                    // word 9  (-1 = unlimited)
    int  spellPwr() const { return i16(w(10)); }                   // word 10
    bool classUse(int cls) const { return (w(11) >> (cls & 15)) & 1; }  // word 11
    int  healPts() const  { return i16(w(12)); }                   // word 12
};

// TEXP = ARRAY[Fighter..Ninja] OF ARRAY[0..12] OF TWIZLONG
struct ExpTable {
    Bytes b;
    WizLong threshold(int cls, int level) const {   // level 0..12
        return WizLong::read(b.p + (cls * 13 + level) * 6);
    }
};

} // namespace wiz

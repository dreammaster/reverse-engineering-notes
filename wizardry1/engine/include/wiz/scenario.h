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

// TENEMY -- 94 bytes / 47 words.  DOS drops Apple's four leading STRING[15]
// names (they live in ASCII.KRN: monsterNameKey(idx, {0 unk-sing, 1 unk-plur,
// 2 name-sing, 3 name-plur})), so every offset is Apple's minus 32.
struct MonsterRec {
    Bytes b;
    u16 w(int i) const { return rd16(b.p + i * 2); }
    int  pic() const      { return i16(w(0)); }                    // word 0
    // CALC1 = the group-count dice (THPREC: count = minad + dice x (1..fac))
    int  cntDice() const  { return i16(w(1)); }
    int  cntFac() const   { return i16(w(2)); }
    int  cntAdd() const   { return i16(w(3)); }
    // HPREC = per-monster HP dice
    int  hpDice() const   { return i16(w(4)); }
    int  hpFac() const    { return i16(w(5)); }
    int  hpAdd() const    { return i16(w(6)); }
    int  cls() const      { return i16(w(7)); }                    // word 7
    int  ac() const       { return i16(w(8)); }                    // word 8
    int  recsN() const    { return i16(w(9)); }                    // word 9  attacks/round
    // RECS[1..7] THPREC at words 10..30 -- damage per attack
    void atkDice(int k, int &dice, int &fac, int &add) const {     // k = 1..7
        const u8 *q = b.p + (10 + (k - 1) * 3) * 2;
        dice = rd16s(q); fac = rd16s(q + 2); add = rd16s(q + 4);
    }
    // words 31-33 (EXPAMT) are 0 in this scenario -- XP comes from the ZREWARD
    // record indexed by reward2().  reward2() confirmed from DOS REWARDS proc 4
    // (ENMYREWD); the rest of the tail follows the Apple field order.
    WizLong expAmt() const { return WizLong::read(b.p + 31 * 2); } // words 31-33
    int  drainAmt() const { return i16(w(34)); }                   // level drain
    int  healPts() const  { return i16(w(35)); }                   // regen
    int  reward1() const  { return i16(w(36)); }
    int  reward2() const  { return i16(w(37)); }                   // ZREWARD index
    int  enmyTeam() const { return i16(w(38)); }                   // allied group (provisional)
    int  teamPerc() const { return i16(w(39)); }
    int  magSpels() const { return i16(w(40)); }                   // mage spell level
    int  priSpels() const { return i16(w(41)); }
    int  unique() const   { return i16(w(42)); }                   // -1 = directly encounterable
    int  breathe() const  { return i16(w(43)); }
    int  unaffct() const  { return i16(w(44)); }                   // % magic resist
    u16  wepVsty3() const { return w(45); }
    u16  sppc() const     { return w(46); }   // attack status bits: 0 stone, 1 poison, 2 paralyze, 3 crit
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

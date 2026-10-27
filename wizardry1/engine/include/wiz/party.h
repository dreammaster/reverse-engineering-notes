// The active party -- up to 6 characters pulled from the Roster.
//
// The DOS game keeps the party in CHARACTR[0..5] (global word 363) with
// PARTYCNT and CHARDISK[0..5] (roster record indices).  A member is a working
// copy of the roster record with INMAZE set; leaving town writes it back.
// Ports the party-assembly bits of CASTLE/SHOPS (see docs/town.md).
#pragma once
#include "wiz/character.h"
#include <string>

namespace wiz {

class Roster;

class Party {
public:
    static constexpr int kMax = 6;

    int count() const { return count_; }
    bool full() const { return count_ >= kMax; }
    bool empty() const { return count_ == 0; }

    Character &member(int i) { return chars_[i]; }
    const Character &member(int i) const { return chars_[i]; }
    int rosterSlot(int i) const { return disk_[i]; }

    // GETALIGN: the alignment of the last non-neutral member, else Neutral.
    Align align() const;

    // ADDPARTY: copy roster slot `slot` in as the next member (INMAZE := true)
    // and write the flagged record back to the roster.  Caller has already
    // checked name/password/alignment.  No-op if full.
    void add(Roster &roster, int slot);

    // REMOVE: member `i` leaves -- clear INMAZE, write back to the roster,
    // close the gap in the party array.
    void remove(Roster &roster, int i);

    // UPDCHARS: everyone leaves (used by Edge of Town -> Training / Leave).
    void disband(Roster &roster);

    // ROLLER:17 -- hand one character to CAMP (PARTYCNT := 1, CHARDISK[0] := slot) and take
    // it back afterwards (ROLLER:15) without touching any roster flags.
    void setSingle(const Character &c, int slot) {
        chars_[0] = c; disk_[0] = slot; count_ = 1;
        for (int i = 1; i < kMax; ++i) disk_[i] = -1;
    }
    // SPECIALS:6: a recovered character joins the party as it is (no roster flags touched here)
    void appendRaw(const Character &c, int slot) { if (count_ < kMax) { chars_[count_] = c; disk_[count_] = slot; ++count_; } }
    // SHOPS:41: the characters who are not OK (still in the dungeon) leave the party
    void keepOnlyOk();
    void disbandQuiet() { count_ = 0; for (int &d : disk_) d = -1; }

    // SHOPS:12 -- write every member's working copy back over its roster record.
    void writeBack(Roster &roster) const;

    // REORDER: swap two members (character copy + roster index together).
    void swapMembers(int a, int b);

    // After the Temple heals a roster record, refresh any party member that
    // maps to it -- keeping the party copy's gold (tithe deductions) and
    // in-maze flag.  (DOS CANT brackets its work with proc 12 / proc 13.)
    void resyncFromRoster(const Roster &roster);

    // Persist the party roster indices so a session can resume mid-town.
    bool load(const std::string &path, const Roster &roster);
    bool save(const std::string &path) const;

private:
    Character chars_[kMax];
    int disk_[kMax] = {-1, -1, -1, -1, -1, -1};
    int count_ = 0;
};

} // namespace wiz

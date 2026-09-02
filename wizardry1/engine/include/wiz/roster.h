// The character roster -- 20 TCHAR slots.
//
// The DOS game keeps this in SCENARIO.DATA (ZCHAR records).  The standalone
// engine keeps it in its own file so it never touches the user's game data;
// an unseeded roster is filled from a Scenario's shipped records.
#pragma once
#include "wiz/character.h"

namespace wiz {

class Scenario;

class Roster {
public:
    static constexpr int kSlots = 20;

    bool load(const std::string &path);            // false if the file is absent
    void seedFrom(const Scenario &sc);             // copy the scenario's ZCHAR records
    bool save(const std::string &path) const;

    Character &slot(int i) { return chars_[i]; }
    const Character &slot(int i) const { return chars_[i]; }
    int count() const { return kSlots; }

    int findLost() const;                          // first LOST slot, or -1
    int findByName(const std::string &name) const; // first live match, or -1
    int liveCount() const;

private:
    Character chars_[kSlots];
};

} // namespace wiz

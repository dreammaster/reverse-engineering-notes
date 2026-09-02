// TCHAR -- a Wizardry character.
//
// This is a native C++ model, not the on-disk layout.  The scenario/roster
// record is 208 bytes (104 words); field offsets within it are being pinned
// down from the p-code (docs/file-formats.md, "TCHAR record").  Serialisation
// to/from the 208-byte record will live in scenario.cpp once verified.
#pragma once
#include "wiz/types.h"

namespace wiz {

enum Attr { STR = 0, IQ, PIETY, VIT, AGI, LUCK, ATTR_COUNT };

struct WizLongV {                 // in-memory TWIZLONG
    int64_t v = 0;
    void addBase10000(int64_t n) { v += n; }   // matches ADDLONGS/MULTLONG net effect
};

struct Character {
    std::string name;
    std::string password;
    bool inMaze = false;
    Race  race  = Race::Human;
    Class cls   = Class::Fighter;
    int   age   = 0;                // weeks
    Status status = Status::OK;
    Align align   = Align::Good;

    int attrib[ATTR_COUNT] = {};    // 0..18 as rolled/kept
    int luckSkill[5] = {16, 16, 16, 16, 16};

    WizLongV gold;
    WizLongV exp;

    int maxLevelAcquired = 1;
    int charLevel = 1;
    int hpLeft = 0;
    int hpMax  = 0;
    int armorClass = 10;

    bool spellKnown[50] = {};
    int  mageSpells[8]  = {};       // [1..7] casts per level
    int  priestSpells[8] = {};
};

} // namespace wiz

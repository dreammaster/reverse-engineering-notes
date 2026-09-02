// Temple of Radiant Cant rules -- cure / resurrection fees and rolls.
//
// Ports CANT (P010202) from the Apple source, cross-checked against DOS SHOPS
// procs 2/6-11.  The Temple treats *roster* characters (found by name across
// the whole roster), paid for by a *party* member.  See docs/town.md.
#pragma once
#include "wiz/character.h"
#include "wiz/rng.h"
#include "wiz/inn.h"          // InnLog
#include <string>

namespace wiz {

// GETPAYER: base donation by status (x the patient's level).  DOS/Apple only
// price PLYZE / STONED / DEAD / ASHES.
inline int64_t templeFee(Status s, int charLevel) {
    int base;
    switch (s) {
        case Status::Paralyzed: base = 100; break;
        case Status::Stoned:    base = 200; break;
        case Status::Dead:      base = 250; break;
        case Status::Ashes:     base = 500; break;
        default: return 0;
    }
    return int64_t(base) * (charLevel < 1 ? 1 : charLevel);
}

// WELCOME's eligible set: statuses the Temple can price and treat.
inline bool templeTreatable(Status s) {
    return s == Status::Paralyzed || s == Status::Stoned ||
           s == Status::Dead || s == Status::Ashes;
}

enum class CantResult { Cured, Worsened };   // Worsened: DEAD->ASHES, ASHES->LOST

// DOCANT (P010208) + ASHLOST (P010209).  `who` is the roster record.
// Resurrection roll: DEAD succeeds when rand%100 <= 50 + 3*VIT,
//                    ASHES succeeds when rand%100 <= 40 + 3*VIT.
// PLYZE / STONED always succeed.  Every visit ages the patient rand%52 + 1
// weeks (the DOS DOCANT also zeroes the poison/lost-location words, which the
// standalone Character model does not track yet).
inline CantResult doCant(Character &who, Rng &rng, InnLog &log) {
    bool worsened = false;
    if (who.status == Status::Dead) {
        if (rng.mod(100) > 50 + 3 * who.attrib[VIT]) worsened = true;
        else who.hpLeft = 1;
    } else if (who.status == Status::Ashes) {
        if (rng.mod(100) > 40 + 3 * who.attrib[VIT]) worsened = true;
        else who.hpLeft = who.hpMax;
    }

    if (worsened) {
        who.status = (who.status == Status::Dead) ? Status::Ashes : Status::Lost;
        who.inMaze = false;
        log.push_back(who.name + (who.status == Status::Lost ? " WILL BE BURIED"
                                                            : " NEEDS KADORTO NOW"));
        return CantResult::Worsened;
    }

    who.age += rng.mod(52) + 1;
    who.status = Status::OK;
    log.push_back(who.name + " IS WELL");
    return CantResult::Cured;
}

} // namespace wiz

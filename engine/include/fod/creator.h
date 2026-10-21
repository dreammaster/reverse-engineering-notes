#pragma once
// FOD.EXE: title picture and party creation screen.
#include "fod/game.h"

namespace fod {

// Shows the title picture, then lets the player build the party (A)dd / R)emove / E)dit / P)lay).
// Returns false if the window was closed before "Play" was chosen. On success `disk` holds the new game record
// (KEH writeInitialDisk1 applied).
bool runPartyCreator(const GameData &data, const Assets &assets, Platform &platform, Disk1 &disk);

// A fresh-game save record: empty roster ready for creation.
Disk1 emptyRoster();

// KEH writeInitialDisk1: save counter 1, roster ids (i - 6), in-party flags.
void finishNewParty(Disk1 &disk);

} // namespace fod

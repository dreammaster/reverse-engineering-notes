// Not yet assert-confirmed to a specific file; stays at the top level.
// A single savegame slot's metadata/IO. Confirmed constructor signature and
// call shapes only (TGameControl::SavegameExists/DeleteSavegame,
// Deponia_Linux.asm lines 462562-462773): constructed with (isNumberedSlot,
// slot, b, c, game) - b/c always passed 0 at every call site seen so far,
// meaning unresolved. SavegameExists() is called without a real "this"
// object at its one call site, so modeled as static (matching the
// TGAction::AddRunningAction/ClearActions pattern elsewhere).
#pragma once

class TVisionaireGame;

class TMSavegame {
public:
    TMSavegame(bool isNumberedSlot, int slot, int b, int c, TVisionaireGame* game);
    virtual ~TMSavegame() = default;

    bool Exists() const;
    bool Delete();
    static bool SavegameExists();
};

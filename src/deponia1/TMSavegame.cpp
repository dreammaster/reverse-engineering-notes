#include "TMSavegame.h"

TMSavegame::TMSavegame(bool /*isNumberedSlot*/, int /*slot*/, int /*b*/, int /*c*/, TVisionaireGame* /*game*/) {
}

bool TMSavegame::Exists() const {
    return false;
}

bool TMSavegame::Delete() {
    return false;
}

bool TMSavegame::SavegameExists() {
    return false;
}

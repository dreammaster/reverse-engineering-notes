// Not yet assert-confirmed to a specific file; stays at the top level.
// The concrete "scene" drawable: TSceneControl::GetScene() actually returns
// a TGScene* (confirmed - callers immediately use it as TGScene::IsMenu(),
// TGScene::GetObject(), etc., not just TPaintControl's interface), so it
// must derive from TPaintControl the same way TCursorControl/TLoadingControl
// do.
#pragma once

#include <vector>

#include "TPaintControl.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"

class TGCharacter;
class TManagedObject;
class TMSavegame;

class TGScene : public TPaintControl {
public:
    bool IsMenu() const;
    // Confirmed TManagedObject* (TGameControl::ReattachSceneObjectTexts
    // calls TManagedObject::SetText() directly on the result, asm lines
    // 461599-461666) - the manifest's void* was a placeholder guess.
    TManagedObject* GetObject(const TVisObjRef& object) const;

    // Confirmed call shapes only (TGameControl::SavegameExists/
    // DeleteSavegame, asm lines 462562-462773) - a savegame-slot-picker
    // scene (a "load game" menu) apparently tracks which slot is currently
    // selected/hovered.
    TMSavegame* GetSelectedSavegame(bool flag);
    TMSavegame* GetSavegameAt(const wxPoint& pos) const;
    void DeleteSelectedSavegame();

    // Confirmed a by-value std::vector<TGCharacter*> (TGameControl::
    // UpdateRandomTimers copies it and iterates the copy, asm lines
    // 463363-463466).
    std::vector<TGCharacter*> GetCharacters() const;
};

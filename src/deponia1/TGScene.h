// Not yet assert-confirmed to a specific file; stays at the top level.
// The concrete "scene" drawable: TSceneControl::GetScene() actually returns
// a TGScene* (confirmed - callers immediately use it as TGScene::IsMenu(),
// TGScene::GetObject(), etc., not just TPaintControl's interface), so it
// must derive from TPaintControl the same way TCursorControl/TLoadingControl
// do.
#pragma once

#include "TPaintControl.h"
#include "datastruct/visobjref.h"

class TGScene : public TPaintControl {
public:
    bool IsMenu() const;
    void* GetObject(const TVisObjRef& object) const;
};

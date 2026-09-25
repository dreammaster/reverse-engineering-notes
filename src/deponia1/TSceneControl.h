// Not yet assert-confirmed to a specific file; stays at the top level.
#pragma once

#include "TGScene.h"

class TSceneControl {
public:
    // Confirmed TGScene*, not just TPaintControl* (callers use it as
    // TGScene::IsMenu()/GetObject() directly - see TGScene.h). Confirmed
    // const (mangled name _ZNK13TSceneControl8GetSceneEv).
    TGScene* GetScene() const;
    void Draw();

private:
    TGScene m_scene;
};

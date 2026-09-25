// Not yet assert-confirmed to a specific file; stays at the top level.
#pragma once

#include "TGScene.h"

class TSceneControl {
public:
    // Confirmed TGScene*, not just TPaintControl* (callers use it as
    // TGScene::IsMenu()/GetObject() directly - see TGScene.h).
    TGScene* GetScene();
    void Draw();

private:
    TGScene m_scene;
};

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

    // Confirmed call shape only (TGameControl::ChangeCharacter, asm lines
    // 465849-466072) - not reversed beyond that.
    void ShowScene(TVisObjRef& scene, bool immediate, bool flag);

    // Confirmed call shape only (TGameControl::Save, asm lines
    // 462781-462971) - out-params, not reversed beyond that.
    void GetLastPlayableSceneParams(TVisObjRef& outScene, wxPoint& outPos) const;

private:
    TGScene m_scene;
};

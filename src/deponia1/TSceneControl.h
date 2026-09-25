// Not yet assert-confirmed to a specific file; stays at the top level.
#pragma once

#include "TPaintControl.h"

class TSceneControl {
public:
    TPaintControl* GetScene();
    void Draw();
};

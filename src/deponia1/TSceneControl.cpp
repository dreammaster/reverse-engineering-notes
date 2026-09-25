#include "TSceneControl.h"

TPaintControl* TSceneControl::GetScene() {
    static TPaintControl scene;
    return &scene;
}

void TSceneControl::Draw() {
}

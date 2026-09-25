// Not yet assert-confirmed to a specific file; stays at the top level.
// A scene object with an attached, reattachable text (confirmed:
// TGScene::GetObject returns one of these, and TGameControl::
// ReattachSceneObjectTexts calls SetText(TGText*) on it - Deponia_Linux.asm
// lines 461599-461666) - nothing else about this class has been reversed.
#pragma once

class TGText;

class TManagedObject {
public:
    void SetText(TGText* text);
};

// Not yet assert-confirmed to a specific file; stays at the top level.
// An on-screen text instance. TGameControl holds these in two
// std::list<TGText*> members (confirmed via sentinel-initialization in the
// constructor and traversal in IsTalking/ReattachSceneObjectTexts/
// ClearObjectText, Deponia_Linux.asm lines 461601-462228): m_activeTexts
// (every currently-displayed text) and m_sceneTexts (texts attached to a
// scene object, reattached when the scene reloads).
//
// Derives from TSText (see its header): both expose a TVisObjRef target
// field at the same offset with no accessor in the original, and share the
// same mystery virtual slot (0x28) that ClearCurrentText/ClearObjectText
// call right before dropping a text - the simplest explanation is a shared
// base rather than coincidence.
#pragma once

#include "TSText.h"

class TGCharacter;

class TGText : public TSText {
public:
    TGCharacter* GetSpeaker() const;
};

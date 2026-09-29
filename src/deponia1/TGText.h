// Not yet assert-confirmed to a specific file; stays at the top level.
// An on-screen text instance. TGameControl holds these in two
// std::list<TGText*> members (confirmed via sentinel-initialization in the
// constructor and traversal in IsTalking/ReattachSceneObjectTexts/
// ClearObjectText, Deponia_Linux.asm lines 461601-462228): _activeTexts
// (every currently-displayed text) and _sceneTexts (texts attached to a
// scene object, reattached when the scene reloads).
//
// Derives from TSText (see its header): both expose a TVisObjRef target
// field at the same offset with no accessor in the original, and share the
// same mystery virtual slot (0x28) that ClearCurrentText/ClearObjectText
// call right before dropping a text - the simplest explanation is a shared
// base rather than coincidence.
#pragma once

#include "TSText.h"
#include "WxStub.h"

class TGCharacter;

class TGText : public TSText {
public:
	TGCharacter *GetSpeaker() const;

	// Confirmed called with a constant 1.0f at every DisplayTexts() call
	// site (asm lines 455890-456037) - not reversed beyond that call shape.
	void Draw(float scale);

	// Confirmed non-virtual (a direct call, not through the vtable) on
	// TGText specifically, not inherited from TSText (TGameControl::Save,
	// asm lines 462781-462971) - not reversed beyond that call shape.
	void Save();
	// Confirmed non-virtual, same shape as Save() above (TGameControl::Load,
	// Deponia_Linux.asm line 476852, called on a freshly-constructed THText*)
	// - not reversed beyond that call shape.
	void Load();

	// Confirmed static call shapes only (TGameControl::SaveEventHandlers,
	// Deponia_Linux.asm lines 457805-457833) - same "registered handler name
	// stored on the class itself, not in TGameControl's containers" pattern
	// as TGAnimation::GetEventHandlerAnimStarted/Stopped above; not reversed
	// beyond that call shape.
	static wxString GetEventHandlerTextStarted();
	static wxString GetEventHandlerTextStopped();
};

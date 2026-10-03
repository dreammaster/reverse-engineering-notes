// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed in full except ToScene() (Deponia_Linux.asm lines 453942-
// 455638, reaching all 15 manifest-listed methods): owns two scenes - the
// current one and, during a cross-fade transition, the one being faded
// out - plus the game-data reference and scroll position to restore when a
// save next needs "what was the last playable (non-menu) scene showing".
//
// The real class constructs two heap-allocated `THScene` instances (a
// concrete TGScene subclass, by the same `operator new` + default-ctor
// shape as THCharacter's own relationship to TGCharacter - Deponia_Linux.
// asm lines 60E93A-60E966) rather than a TGScene by value, as the
// manifest's original guess had it. Modeled here as plain `TGScene*`
// instead of introducing a near-empty THScene subclass, since nothing this
// class does needs anything beyond TGScene's own confirmed interface
// (IsMenu()/GetRef()/SetRef()/GetScrollPos()/Draw()) - THScene's own extra
// behavior (e.g. RegisterEvents(), called from ToScene()) isn't reversed
// and isn't needed here.
//
// ToScene() is the actual scene-transition implementation (sound handling,
// swapping the current/old scene pointers, registering the new scene's
// events, cursor/animation bookkeeping) - by itself longer than every
// other method in this class combined, and pulling in several more
// not-yet-modeled dependencies (THScene::RegisterEvents(),
// TSoundInterface, TMasterControl::GetSoundManager()). Left as a
// confirmed-signature stub; every other method here is implemented in
// full around it, since their own logic doesn't depend on what it
// actually does internally.
#pragma once

#include "TGScene.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"

class TGCharacter;

class TSceneControl {
public:
	TSceneControl();
	~TSceneControl();

	// Confirmed TGScene*, not just TPaintControl* (callers use it as
	// TGScene::IsMenu()/GetObject() directly - see TGScene.h). Confirmed
	// const (mangled name _ZNK13TSceneControl8GetSceneEv): returns the
	// scene currently mid-fade-out instead of the current one, for the
	// duration of Draw()'s own call into it (see _drawingOldScene below).
	TGScene *GetScene() const;
	// A second, non-const overload (Deponia_Linux.asm line 455632) that
	// always returns the current scene outright, skipping the
	// _drawingOldScene redirect above - confirmed distinct bodies, not
	// just a const/non-const pair over the same logic.
	TGScene *GetScene();
	void Draw();
	// Confirmed call shape only (TGameControl::Init, asm lines 467226-
	// 467624) - sets the initial scene from a TVisObjRef link; not
	// reversed beyond that.
	void Set(const TVisObjRef &scene);

	// Confirmed call shape only (TGameControl::ChangeCharacter, asm lines
	// 465849-466072) - not reversed beyond that.
	void ShowScene(TVisObjRef &scene, bool immediate, bool flag);

	// Confirmed call shapes only for ToScene()'s own part (see this
	// class's own header comment); the rest - resolving a default
	// character/scene via the game's own links (0x1D4) when the caller
	// didn't supply one, and scrolling to the game's "current" character
	// (link 0x263) afterwards - is implemented in full (Deponia_Linux.asm
	// lines 454942-455340).
	void ChangeScene(const TVisObjRef &character, const TVisObjRef &target, bool immediate, int direction);
	void ChangeScene(const TVisObjRef &character, TVisObjRef &scene, bool immediate, const wxPoint &pos,
	                 int direction);

	// Confirmed call shape only (TGameControl::Save, asm lines
	// 462781-462971) - out-params, not reversed beyond that.
	void GetLastPlayableSceneParams(TVisObjRef &outScene, wxPoint &outPos) const;

	// Confirmed call shape only (TGameControl::HandleKeyEvent,
	// Deponia_Linux.asm line 471018) - not reversed beyond that.
	bool FadingToNewScene() const;
	// Confirmed (Deponia_Linux.asm lines 60F330-60F33B): the scene
	// currently mid-fade-out, not the current one.
	bool OldSceneIsMenu() const;

	// Confirmed call shape only (TGameControl::Load, Deponia_Linux.asm line
	// 477024) - not reversed beyond that.
	void SetNextStartScrollPos(const wxPoint &pos);

	// Confirmed accessed directly as private fields from TGScene::
	// InitialiseBackground() (Deponia_Linux.asm lines 168670-169184) -
	// modeled as public accessors instead of a cross-class friendship,
	// matching TGScene::SetRef()'s own already-established pattern for the
	// same kind of access.
	const wxPoint &GetNextStartScrollPos() const {
		return _nextStartScrollPos;
	}
	void SetLastPlayableScrollPos(const wxPoint &pos) {
		_lastPlayableScrollPos = pos;
	}
	void SetNextStartScrollPosFromLastPlayable() {
		_nextStartScrollPos = _lastPlayableScrollPos;
	}
	void SetCurrentSceneRef(const TVisObjRef &ref) {
		_ref = ref;
	}
	TGScene *GetOldScene() const {
		return _oldScene;
	}

private:
	// Confirmed call shape only (TSceneControl::ChangeScene/ShowScene,
	// Deponia_Linux.asm lines 454129-454879) - see this class's own header
	// comment.
	void ToScene(bool immediate, TVisObjRef &scene, const TVisObjRef &character, const wxPoint &pos,
	             int direction, bool flag);

	// Confirmed (Deponia_Linux.asm lines 60E8F0-60E9B6): {-1, -1}.
	wxPoint _nextStartScrollPos{-1, -1};
	TVisObjRef _ref;
	wxPoint _lastPlayableScrollPos;
	bool _fadingToNewScene = false;
	// Confirmed (Deponia_Linux.asm line 60F922): gates whether Draw() below
	// also draws _oldScene at all - distinct from both _fadingToNewScene
	// above and _drawingOldScene below (three separate confirmed flags, not
	// one reused three ways).
	bool _hasOldScene = false;
	// Confirmed set for the duration of the old scene's own Draw() call
	// inside this class's own Draw() (Deponia_Linux.asm lines 60F936-
	// 60F94C) - GetScene() (the const overload) and
	// GetLastPlayableSceneParams() both redirect to _oldScene while this
	// is true, so that code running synchronously during that draw call
	// still resolves "the scene" to the one actually being drawn.
	bool _drawingOldScene = false;
	TGScene *_currentScene = nullptr;
	TGScene *_oldScene = nullptr;
};

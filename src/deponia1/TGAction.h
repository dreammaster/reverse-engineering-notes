// Not yet assert-confirmed to a specific file; stays at the top level.
// The game-action/cutscene-scripting subsystem. Only the entry points
// TGameControl calls are declared here (confirmed call shapes, not
// reversed bodies): AddRunningAction (TGameControl::HandleMouseHolding,
// Deponia_Linux.asm lines 455671-455746) and ClearActions
// (~TGameControl, asm line 474249). Both are called without an object of
// this type ever being constructed at the call site, so modeled as static
// methods rather than instance methods.
#pragma once

#include "datastruct/visobjref.h"

enum class TMouseMessageEnum;
struct t_SkipCutsceneInfo;

// Confirmed call shape only (TGameControl::HandleMouseUp, Deponia_Linux.asm
// lines 472809-472810 and elsewhere in the same function) - the opaque event
// type TGObjectManager::HandleEvent() takes; ConvertToEvent() produces one
// from a TMouseMessageEnum, and TGObjectManager::ExecuteSavedObject() passes
// one literal value, 1 (Deponia_Linux.asm line 188894) - neither resolved to
// a real meaning, so named by raw value like this project's TypeOrder/
// eVisionaireTable enums.
enum class TMouseEventEnum { kValue1 = 1 };

class TGAction {
public:
	// Confirmed to return the action it started (TGScene::EndScene()/
	// BeginScene() test the result for null before calling Execute()) - not
	// reversed beyond that call shape.
	static TGAction *AddRunningAction(const TVisObjRef &action);
	// Confirmed call shape only (TGScene::EndScene()/BeginScene(),
	// Deponia_Linux.asm lines 167139, 172880) - executes this action; the
	// real body is the engine's whole scripted-action interpreter (tens of
	// thousands of lines), not reversed here.
	void Execute(bool flag, t_SkipCutsceneInfo *skipInfo);
	// Confirmed call shapes only (TGScene::InitialiseBackground(), asm lines
	// 168670-169184) - suspend/resume every running action on entering/
	// leaving a menu scene; not reversed beyond that.
	static void StopRunningActions();
	static void ContinueStoppedActions();
	static void ContinueRunningActions(bool flag);
	static void ClearActions();
	// Confirmed static (TGameControl::Save, asm line 462895) - not
	// reversed beyond that call shape.
	static void SaveActions();
	// Confirmed static (TGameControl::HandleKeyEvent, Deponia_Linux.asm line
	// 471550) - not reversed beyond that call shape.
	static void SkipCutscene();
	// Confirmed static (TGameControl::Load, Deponia_Linux.asm line 476980) -
	// the load-side counterpart to SaveActions() above; not reversed beyond
	// that call shape.
	static void LoadActions();
	// Confirmed static (TGameControl::Update, Deponia_Linux.asm line 469725)
	// - called once per frame, right after TMasterControl::
	// ContinueRunningActions(); not reversed beyond that call shape.
	static void DeleteFinishedActions();
	// Confirmed static call shape only (TGameControl::HandleMouseUp,
	// Deponia_Linux.asm line 472810 and others) - not reversed beyond that.
	static TMouseEventEnum ConvertToEvent(TMouseMessageEnum msg);
};

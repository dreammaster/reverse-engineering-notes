// Confirmed (Deponia_Linux.asm lines 194786-212169, vsplayer/actionGame.cpp): the running scripted
// actions of the game - the cutscenes, the answers of the dialogs, what the commands of the
// interface do. An action (the data object, TTAction) is a list of action parts (TTActionPart);
// the part has a command (eCommand) and its arguments (links and integers). While an action runs,
// its state (which part is next, whether a part was started, how deep inside an IF that is not
// true it is, how long it has been paused) is kept in a record of its own (the "ActiveAction",
// TSAction), which is what the saved game keeps.
//
// Execute() goes through the parts from the one that is next: a part that can be done at once is
// done and the next one follows (in the same call, up to 10000 parts), a part that waits (for an
// animation, for a time, for the text to be read ...) ends the call and is looked at again on the
// next frame. The parts IF ... ELSE ... END IF are done by the depth that is kept in the record:
// while it is above 0 the parts of a branch that is not taken are only counted. When Execute() is
// called with the skip flag (the player skips a cutscene) the effects of the parts that were
// started are ended, and the rest of the action is done with the commands' own fast versions,
// which also tell `t_SkipCutsceneInfo` where the cutscene ends (the scene and the character).
//
// Original layout: +0x00 vtable (virtual destructors), +0x08 the active record (TSAction), +0x10
// whether the action is paused (by the scripts), +0x11 whether it is stopped (the game stops
// while a menu is shown), +0x12 whether it is over (it is deleted by DeleteFinishedActions()),
// +0x13 whether it is being executed (an action does not call itself), +0x18 the timer, +0x28
// the data object.
//
// All running actions are in a list (RunningActions), and there is a table from the id of the
// data object to the place in the list (ActionIdToRunningAction): an action cannot run twice.
#pragma once

#include <list>
#include <unordered_map>
#include <vector>

#include "TTimer.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"
#include "datastruct/vlist.h"
#include "vstables/records.h"

enum class TMouseMessageEnum;
class TGameControl;

/** What the commands tell about the end of a cutscene that is skipped: the character that is the
 *  current one, the scene that is shown, and the objects (scenes) whose music was changed on the
 *  way. */
struct t_SkipCutsceneInfo {
	TVisObjRef character;   // +0x00
	TVisObjRef scene;       // +0x08
	TVList objects;         // +0x10
};

// Confirmed call shape only (TGameControl::HandleMouseUp, Deponia_Linux.asm
// lines 472809-472810 and elsewhere in the same function) - the opaque event
// type TGObjectManager::HandleEvent() takes; ConvertToEvent() produces one
// from a TMouseMessageEnum, and TGObjectManager::ExecuteSavedObject() passes
// one literal value, 1 (Deponia_Linux.asm line 188894) - neither resolved to
// a real meaning, so named by raw value like this project's TypeOrder/
// eVisionaireTable enums.
enum class TMouseEventEnum {
	kValue0 = 0,
	kValue1 = 1,
	kValue2 = 2,
	kValue3 = 3,
	kValue4 = 4,
	kValue7 = 7,
	kValue8 = 8,
	kValue9 = 9
};

class TGAction {
	friend class TGActionPartExecutor;

public:
	TGAction();
	/** `active` is the record of the state, `data` the action that runs. */
	TGAction(const TVisObjRef &active, const TVisObjRef &data);
	virtual ~TGAction();

	/** (not allowed: an action's record is made with the action) */
	TGAction &operator=(TVisObjRef &ref);
	bool operator==(const TGAction &other) const;
	bool operator==(const TVisObjRef &ref) const;

	/** The action that is run (its data object). */
	TVisObjRef GetDataObject() const;
	/** The scripts pause and resume an action. */
	void SetPaused(bool paused);
	/** A stopped action goes on: the time that the game was stopped does not count. */
	void ContinueStoppedAction();
	/** Does the action's parts: see the top of this file. */
	void Execute(bool skip, t_SkipCutsceneInfo *skipInfo);
	/** Saves the state: how long the action has been paused, and whether the record is saved at
	 *  all (a finished action is only kept in the saved game while a cutscene it ended is still
	 *  to be skipped). */
	void Save();
	/** Restores the state from the record of a saved game. */
	void Load(const TVisObjRef &active);

	/** Starts an animation (an object's or a character's) for an action part; the animation that
	 *  was started (null: none). */
	static class TGAnimation *ShowAnimation(TVisObjRef &animation, bool reverse);
	/** Hides an animation again. */
	static void HideAnimation(const TVisObjRef &animation);
	/** Whether the animation is still running. */
	static bool WaitOnAnimation(TVisObjRef &animation);

	// The running actions.
	static std::vector<TGAction *> &GetRunningActions();
	/** Starts the action `action`, once: the one that is running already is returned when it is
	 *  (null: the action is empty). */
	static TGAction *AddRunningAction(const TVisObjRef &action);
	/** Stops the action that has the id of `id` (of the active record or, with `byData`, of the
	 *  data object); it is deleted by DeleteFinishedActions(). */
	static void DeleteRunningAction(const TVisObjRef &id, bool byData);
	/** Deletes the actions that are over (and not being executed). */
	static void DeleteFinishedActions();
	/** Deletes all actions. */
	static void ClearActions();
	/** Ends all actions. */
	static void FinishAllActions();
	/** The game stops (a menu): all actions that are running are stopped with it. */
	static void StopRunningActions();
	/** The game goes on: the stopped actions go on. */
	static void ContinueStoppedActions();
	/** Executes all running actions (each frame, not while a video is playing). */
	static void ContinueRunningActions(bool skip);
	static TVisObjRef GetActionByName(const wxString &name);
	static TVisObjRef GetActionById(int id);
	static void GetActions(TVList &outActions);
	/** For the console: the running actions, and the action part each is at. */
	static void PrintRunningActions(std::list<wxString> &outLines);
	/** The player skips the cutscene that is running. */
	static void SkipCutscene();
	/** All running actions save their state (the saved game), and are made again (a loaded game). */
	static void SaveActions();
	static void LoadActions();
	static TMouseEventEnum ConvertToEvent(TMouseMessageEnum msg);
	/** The action that is saving the game (set by its save command for the save: the saved game
	 *  is then of the action as it is after the save). */
	static const TVisObjRef &GetSaveAction() {
		return s_saveAction;
	}

protected:
	TSAction _active;   // +0x08
	bool _paused;       // +0x10
	bool _stopped;      // +0x11
	bool _finished;     // +0x12
	bool _executing;    // +0x13
	TTimer _timer;      // +0x18
	TVisObjRef _data;   // +0x28

private:
	static std::vector<TGAction *> s_runningActions;
	static std::unordered_map<int, int> s_actionIdToRunningAction;
	static bool s_deletedActions;
	static TVisObjRef s_saveAction;

	/** Puts the action into the list and the table (the place at the end). */
	static void registerAction(TGAction *action);
};

// Not an original class: the commands of TGAction::Execute() (Deponia_Linux.asm lines
// 198485-210995). The original has them all in one function of 12,000 lines, with the
// locals of the loop shared by all of them; here the locals are the members of this class and
// each command is a method, so the loop (TGActionExecute.cpp) stays readable. Every method is
// the code of one case of the command switch, with its own recovered behavior; the names of the
// methods are invented (see vstables/eCommand.h).
//
// What a command tells the loop is in `_wait` (the part is not done yet: leave Execute() and look
// at the same part on the next frame), `_stay` (do the same part again at once - a command can
// change which part is next) and `_quit` (the game is quitting: leave without any cleanup).
#pragma once

#include "TGAction.h"
#include "vstables/eCommand.h"

class TGCharacter;
class TGameControl;

class TGActionPartExecutor {
public:
	/** `skip`: the player is skipping the cutscene - the commands then do the end result of what
	 *  they do at once and tell `skipInfo` where the cutscene ends. */
	TGActionPartExecutor(TGAction &action, const TVisObjRef &part, const TVisObjRef &game, const TVList &parts,
	                     bool skip, t_SkipCutsceneInfo *skipInfo, bool &dialogEnded);

	/** Does the command of the part (a command that is not known does nothing). */
	void run(int command);

	bool _wait;
	bool _stay;
	bool _quit;
	/** The part the loop goes on at (a command that jumps sets it; -1: not changed). */
	int _newPosition;

private:
	TGAction &_action;
	TVisObjRef _part;
	TVisObjRef _game;
	const TVList &_parts;
	bool _skip;
	t_SkipCutsceneInfo *_skipInfo;
	bool &_dialogEnded;

	TSAction &active() {
		return _action._active;
	}
	/** The part's values: the link (0xB4), the second link (0x122) and the integers (0xF2, 0x123,
	 *  0x24F). */
	TVisObjRef link() const;
	TVisObjRef altLink() const;
	/** The current character's record. */
	TVisObjRef currentCharacter() const;
	/** `ref`, or the current character's record when it is empty. */
	TVisObjRef orCurrentCharacter(const TVisObjRef &ref) const;
	/** `ref`, or the record of the scene that is shown when it is empty. */
	TVisObjRef orCurrentScene(const TVisObjRef &ref) const;
	/** The action part is not done until the thing that was started has ended: `started` is kept in
	 *  the record (kActionActionPartStarted). */
	bool started() const;
	void setStarted(bool started);
	/** The action's data object and its id, for the messages. */
	void logPartNotDone(const wchar_t *format);
	/** The if of the part is false: skip to its else or its end. */
	void ifFalse();

	void cmdElse();
	void cmdChangeScene();
	void cmdChangeCharacter();
	void cmdStartDialog();
	void cmdEndDialog();
	void cmdEndAction();
	void cmdShowAnimation();
	void cmdWaitAnimation();
	void cmdShowText();
	void cmdPlaySound(bool loop);
	void cmdWait();
	void cmdCharacterGoToObject();
	void cmdWaitCharacter();
	void cmdSetCommand();
	void cmdShowScene();
	void cmdCharacterGoTo();
	void cmdScrollToObject();
	void cmdScrollToPoint();
	void cmdPlaceCharacter();
	void cmdChangeOutfit();
	void cmdSetCommentSet();
	void cmdSetLanguage();
	void cmdShowCharacter();
	void cmdShowHideInterface();
	void cmdTurnCharacter();
	void cmdSceneMusic();
	void cmdScrollToCharacter();
	void cmdPlayVideo();
	void cmdIfValue();
	void cmdSetValue();
	void cmdShowTextAt();
	void cmdStopSound();
	void cmdIfCharacterDirection();
	void cmdSetMusicVolume();
	void cmdGoto();
	void cmdIfCurrentCharacter();
	void cmdClearItems();
	void cmdGiveAllItems();
	void cmdSetOutfitSpeed();
	void cmdRandomValue();
	void cmdIfCharacterInScene();
	void cmdFollowCharacter();
	void cmdStopFollowing();
	void cmdStopWalking();
	void cmdSetWalkingSound();
	void cmdSetFont();
	void cmdSetInterface();
	void cmdIfLanguage();
	void cmdSetCursor();
	void cmdIfCommand();
	void cmdCharacterActive();
	void cmdSaveObject();
	void cmdExecuteSavedObject();
	void cmdIfCurrentObject();
	void cmdSetObjectActive();
	void cmdClearSavedObject();
	void cmdSkipText();
	void cmdFade();
	void cmdSetVisibility(bool character);
	void cmdFadeInterface();
	void cmdSetLightMap();
	void cmdSetBrightness();
	void cmdSetItem();
	void cmdCharacterGoToPoint();
	void cmdDeleteSavegame();
	void cmdIfSavegame();
	void cmdSetAnimationIndex();
	void cmdWaitSound();
	void cmdSetScrollableArea(bool horizontal);
	void cmdSetTextOutput();
	void cmdPlaceCharacterAt();
	void cmdSetWaySystem();
	void cmdSetAnimationFrame(bool last);
	void cmdSetTextSpeed();
	void cmdRunScript();
	void cmdRunPartScript();
	void cmdSetCondition();
	void cmdIfCondition();
	void cmdHideCursor();
	void cmdHideInterfaces();
	void cmdEarthquake();
	void cmdCutscene();
	void saveGame(int slot, bool saveAfter);
	void cmdAutosave();
	void cmdGameSave();
	void cmdSetVolume();
	void cmdStartAction();
	void cmdPreloadAnimation();
	void cmdCharacterItem();
	void cmdScrollSavegames();
	void cmdMoveAnimation();
	void cmdSnoopAnimations();
	void cmdPreloadCharacter();
	void cmdShowObjectText();
	void cmdClearObjectText();
	void cmdWaitTalking();
	void cmdMoveObject(bool by);
};

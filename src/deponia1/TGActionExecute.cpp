#include "TGAction.h"

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "TGActionExecutor.h"
#include "TGCharacter.h"
#include "TSceneControl.h"
#include "TSoundFFMPEG.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vsplayer/animationGame.h"
#include "vsplayer/control/gameControl.h"
#include "vsplayer/control/masterControl.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

// How many parts an action may do in one call before it is taken to be in an endless loop.
static const int kMaxPartsPerCall = 10000;

// Confirmed (TTActionPart::IsIFActionPart(), Deponia_Linux.asm lines 1446716-1446750): the
// commands that start an if (and so are counted, with the end ifs, when a branch is skipped).
static bool isIfCommand(int command) {
	switch (command) {
	case 0x45:
	case 0x4F:
	case 0x54:
	case 0x5C:
	case 0x5F:
	case 0x67:
	case 0x7A:
	case 0x7B:
	case 0x8C:
	case 0xA0:
		return true;
	default:
		return false;
	}
}

// TGameControl implements everything used from it here, but g_pGameControl is only
// declared as TMasterControl* (AppGlobals.h) - the same cast the other classes use.
static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

static int dataId(const TVisObjRef &ref) {
	return PackVisId(ref.GetId());
}

// Confirmed (asm lines 199010-199030, 198996-199013 and the loop at 562A3A-562AAC): the cutscene
// is skipped: what the parts that were done before did (animations, texts, sounds, started
// actions) is undone, from the part the cutscene started with up to the one that is next.
static void undoStartedParts(const TVisObjRef &active, const TVList &parts, t_SkipCutsceneInfo *skipInfo) {
	int current = active.GetInt(kActionActionPartIndex);

	if (current < 0)
		return;

	size_t first = 0;

	for (size_t i = 0; i < parts.size(); i++) {
		TVisObjRef part(parts.at(i));

		if (part.GetInt(kActionPartCommand) == kCommandStartCutscene && part.GetInt(kActionPartInt) == 0) {
			first = i;
			break;
		}
	}

	for (size_t i = first; i < parts.size() && (int)i < current; i++) {
		TVisObjRef part(parts.at(i));

		switch (part.GetInt(kActionPartCommand)) {
		case kCommandShowAnimation:
			if (part.GetInt(kActionPartInt) == 0)
				TGAction::HideAnimation(part.GetLink(kActionPartLink));
			break;
		case kCommandStopCharacterAnimation: {
			// (the integer 1 of the animation is looked at, as in the original)
			TVisObjRef animation = part.GetLink(kActionPartLink);
			TVisObjRef owner = animation.GetParent();

			if (animation.GetInt(1) != 0)
				gameControl()->GetCharacter(owner)->StopCharacterAnim(TCharacterAnimEnum::kNone, true);

			break;
		}
		case kCommandShowText:
			if (part.GetInt(kActionPartInt) == 1)
				gameControl()->ClearText(part.GetLink(kActionPartLink));
			break;
		case kCommandShowTextAt:
			if (part.GetInt(kActionPartAltInt2) == 1)
				gameControl()->ClearText(part.GetLink(kActionPartLink));
			break;
		case kCommandPlaySound: {
			wxFileName path = part.GetPath(kActionPartPath);
			TSoundFFMPEG *sounds = gameControl()->GetSoundManager();

			if (sounds->IsPlaying(path))
				sounds->Stop(path);

			break;
		}
		case kCommandStartAction:
			if (part.GetInt(kActionPartInt) == 0) {
				TVisObjRef started = part.GetLink(kActionPartLink);

				for (TGAction *action : TGAction::GetRunningActions()) {
					if (*action == started) {
						action->Execute(true, skipInfo);
						break;
					}
				}
			}

			break;
		default:
			break;
		}
	}
}

// Confirmed (asm lines 198485-199010, 562634-56291C and the commands' common exits): goes through
// the parts from the one that is next; see the top of TGAction.h.
void TGAction::Execute(bool skip, t_SkipCutsceneInfo *skipInfo) {
	if (_paused)
		return;

	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	TVList parts;

	_data.GetList(kActionActionParts, parts);

	if (skip)
		undoStartedParts(_active, parts, skipInfo);

	if (_stopped || _finished)
		return;

	if (_executing)
		return;

	_executing = true;

	int index = _active.GetInt(kActionActionPartIndex);
	size_t position;

	if (index < 0) {
		position = 0;
		_active.SetValue(kActionActionPartIndex, 0, TSendEventEnum::kSendEvent);
	} else {
		position = (parts.size() >= (size_t)index) ? (size_t)index : parts.size();
	}

	int guard = kMaxPartsPerCall + 1;
	bool dialogEnded = false;

	while (position != parts.size()) {
		TVisObjRef part(parts.at(position));
		int command = part.GetInt(kActionPartCommand);
		int depth = _active.GetInt(kActionContinueWaitForEndIfElse);
		TGActionPartExecutor executor(*this, part, game, parts, skip, skipInfo, dialogEnded);

		if (depth > 0) {
			// inside the branch of an if that is not taken: only the ifs and their ends are counted
			if (isIfCommand(command)) {
				_active.SetValue(kActionContinueWaitForEndIfElse, depth + 1, TSendEventEnum::kSendEvent);
			} else if (depth == 1) {
				if (command == kCommandElse || command == kCommandEndIf)
					_active.SetValue(kActionContinueWaitForEndIfElse, 0, TSendEventEnum::kSendEvent);
			} else if (command == kCommandEndIf) {
				_active.SetValue(kActionContinueWaitForEndIfElse, depth - 1, TSendEventEnum::kSendEvent);
			}
		} else if ((unsigned int)(command - 5) <= 0x9B) {
			executor.run(command);
		}

		if (executor._quit)
			return;

		guard--;

		if (guard == 0) {
			if (wxLog::loglevel > 0) {
				wxLog::logexpanded(L"Action \"%s\" (active id: %d, data id: %d) has executed more than 10000 action parts without a pause. Action is probably in an endless loop. Execution of action is stopped.",
				                   _data.GetName().c_str().wc_str(), dataId(_active), dataId(_data));
			}

			_finished = true;
			break;
		}

		if (executor._wait)
			break;

		if (!executor._stay) {
			_active.SetValue(kActionActionPartIndex, _active.GetInt(kActionActionPartIndex) + 1, TSendEventEnum::kSendEvent);
			position++;
		}

		if (_paused)
			break;
	}

	if (position == parts.size())
		_finished = true;

	// a cutscene that this action started and did not end ends with it
	if (_finished && game.GetLink(kGameCutsceneAction) == _active) {
		game.SetValue(kGameHideCursor, false, TSendEventEnum::kSendEvent);
		game.SetValue(kGameHideInterfaces, false, TSendEventEnum::kSendEvent);
		game.ClearLink(kGameCutsceneAction, false);

		if (wxLog::loglevel > ((g_traceFlags & 2) ? 0 : 1)) {
			wxLog::logexpanded(L"A cutscene was started in action \"%s\" (data id: %ld) but was not terminated",
			                   _data.GetNameWithParents(3).wc_str(), (long)dataId(_active));
		}
	}

	_executing = false;
}

// Confirmed (asm lines 211382-211541): every running action is executed once (not while a video is
// playing); an action that is added meanwhile is executed too.
void TGAction::ContinueRunningActions(bool skip) {
	static t_SkipCutsceneInfo skipInfo;

	s_deletedActions = false;

	for (size_t i = 0; i < s_runningActions.size() && !gameControl()->IsVideoPlaying(); i++)
		s_runningActions[i]->Execute(skip, &skipInfo);
}

// Confirmed (asm lines 210995-211382): the cutscene is skipped: its action is executed in the skip
// mode (the commands then do their end results and tell where the cutscene ends), the characters
// and the view are put at their destinations, the music that changed on the way is made to take
// effect, and the character and the scene the cutscene ends with are shown.
void TGAction::SkipCutscene() {
	t_SkipCutsceneInfo info;

	info.character = gameControl()->GetCurrentCharacter()->GetRef();
	info.scene = gameControl()->GetScene()->GetRef();

	TVisObjRef game = gameControl()->GetGameSystem()->GetGame();
	TVisObjRef cutscene = game.GetLink(kGameCutsceneAction);

	for (TGAction *action : s_runningActions) {
		if (!(action->_active == cutscene))
			continue;

		action->Execute(true, &info);
		gameControl()->SetAllCharactersOnDestination();
		gameControl()->SetOnScrollDestination();

		for (TVisionaireObject *object : info.objects) {
			if (info.scene == *object) {
				// the music of the scene that is shown: its path is set again, for the scene to start it
				wxFileName music = info.scene.GetPath(kSceneBackgroundMusic);

				info.scene.SetValue(kSceneBackgroundMusic, music, TSendEventEnum::kNoEvent);
				info.scene.SetValue(kSceneBackgroundMusic, music, TSendEventEnum::kSendEvent);
				break;
			}
		}

		gameControl()->ResetState();
		gameControl()->ChangeCharacter(info.character, false, info.scene);
		action->_stopped = false;

		wxPoint noScroll;

		noScroll.x = -1;
		noScroll.y = -1;
		gameControl()->GetSceneControl()->SetNextStartScrollPos(noScroll);
		break;
	}

	game.ClearLink(kGameCutsceneAction, false);
}

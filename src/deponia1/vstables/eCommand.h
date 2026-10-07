// The commands of the action parts (kActionPartCommand of a TTActionPart; the type's own name,
// `eCommand`, is recovered from TTActionPart::IsIFActionPart(eCommand) etc., the values are the
// numbers the data and the code use). The names of the enumerators are not in the binary - the
// editor's names for them are loaded from a language file at run time (TTActionPart::Strings) -,
// so they are invented from what each command does in TGAction::Execute(); the messages the
// binary logs name some of them ('Change scene', 'Set value', FADE_INTERFACE ...). A command
// that is not listed here is one that Execute() does nothing for.
#pragma once

enum eCommand {
	kCommandElse = 5,            // IF ... ELSE ... END IF: the else branch (skips to the end if the if was true)
	kCommandEndIf = 6,
	kCommandChangeScene = 10,    // 'Change scene'
	kCommandChangeCharacter = 12, // 'Change character'
	kCommandStartDialog = 13,
	kCommandEndDialog = 14,
	kCommandEndAction = 16,      // the action is over
	kCommandShowAnimation = 19,  // starts an animation (or, with the flag, hides it)
	kCommandWaitAnimation = 22,
	kCommandShowText = 23,       // 'Display text'
	kCommandPlaySound = 24,
	kCommandWait = 30,
	kCommandCharacterGoToObject = 31,
	kCommandWaitCharacter = 32,
	kCommandSetCommand = 45,
	kCommandShowScene = 46,      // 'Show scene'
	kCommandCharacterGoTo = 47,
	kCommandScrollToObject = 48,
	kCommandScrollToPoint = 49,
	kCommandPlaceCharacter = 50,
	kCommandChangeOutfit = 51,
	kCommandSetCommentSet = 52,
	kCommandSetLanguage = 53,
	kCommandShowCharacter = 54,  // 'Show character'
	kCommandQuit = 55,
	kCommandShowHideInterface = 56, // SHOW_HIDE_INTERFACE
	kCommandTurnCharacter = 58,
	kCommandSceneMusic = 59,
	kCommandScrollToCharacter = 60,
	kCommandPlayVideo = 66,
	kCommandIfValue = 69,
	kCommandSetValue = 70,       // 'Set value'
	kCommandShowTextAt = 71,
	kCommandLoopSound = 72,
	kCommandStopSound = 73,
	kCommandStopCharacterAnimation = 61,
	kCommandCharacterGoTo2 = 96,  // the same as kCommandCharacterGoTo
	kCommandStartCutscene = 144,  // (its integer 0) the cutscene that can be skipped starts here
	kCommandStartAction = 148,    // starts another action and does its first parts at once
	kCommandIfCharacterDirection = 160
};

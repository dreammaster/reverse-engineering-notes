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
	kCommandStopCharacterAnimation = 61,
	kCommandPlayVideo = 66,
	kCommandIfValue = 69,
	kCommandSetValue = 70,       // 'Set value'
	kCommandShowTextAt = 71,
	kCommandLoopSound = 72,
	kCommandStopSound = 73,
	kCommandSetMusicVolume = 77,
	kCommandGoto = 78,
	kCommandIfCurrentCharacter = 79,
	kCommandClearItems = 80,
	kCommandGiveAllItems = 81,
	kCommandSetOutfitSpeed = 82,
	kCommandRandomValue = 83,
	kCommandIfCharacterInScene = 84,
	kCommandFollowCharacter = 85,
	kCommandStopFollowing = 86,
	kCommandStopWalking = 87,
	kCommandSetWalkingSound = 88,
	kCommandSetFont = 89,
	kCommandSetInterface = 90,
	kCommandIfLanguage = 92,
	kCommandSetCursor = 94,
	kCommandIfCommand = 95,
	kCommandCharacterGoTo2 = 96,  // the same as kCommandCharacterGoTo
	kCommandCharacterGoToObject2 = 97, // the same as kCommandCharacterGoToObject
	kCommandCharacterActive = 98,
	kCommandSaveObject = 99,
	kCommandExecuteSavedObject = 100,
	kCommandIfCurrentObject = 103,
	kCommandSetObjectActive = 105,
	kCommandClearSavedObject = 106,
	kCommandSkipText = 107,
	kCommandFade = 110,
	kCommandSetObjectVisibility = 111,
	kCommandFadeInterface = 112,   // FADE_INTERFACE
	kCommandSetLightMap = 114,
	kCommandSetBrightness = 115,
	kCommandSetCharacterVisibility = 116,
	kCommandSetItem = 117,
	kCommandCharacterGoToPoint = 119,
	kCommandDeleteSavegame = 121,
	kCommandIfSavegame = 123,
	kCommandSetAnimationIndex = 124,
	kCommandWaitSound = 125,
	kCommandSetScrollableAreaX = 126,
	kCommandSetScrollableAreaY = 127,
	kCommandSetTextOutput = 131,
	kCommandPlaceCharacterAt = 132,
	kCommandSetWaySystem = 133,
	kCommandSetFirstFrame = 134,
	kCommandSetLastFrame = 135,
	kCommandSetTextSpeed = 136,
	kCommandRunScript = 137,
	kCommandRunPartScript = 138,
	kCommandSetCondition = 139,
	kCommandIfCondition = 140,
	kCommandHideCursor = 141,
	kCommandHideInterfaces = 142,
	kCommandEarthquake = 143,
	kCommandCutscene = 144,       // (its integer 0 starts a cutscene that can be skipped, else it ends it)
	kCommandAutosave = 145,       // 'Load/Save autosave'
	kCommandGameSave = 146,       // 'Load/Save game'
	kCommandSetVolume = 147,
	kCommandStartAction = 148,    // starts another action and does its first parts at once
	kCommandPreloadAnimation = 149,
	kCommandCharacterItem = 150,
	kCommandScrollSavegames = 151,
	kCommandMoveAnimation = 152,
	kCommandSnoopAnimations = 153,
	kCommandPreloadCharacter = 154, // 'Preload/Unload character'
	kCommandShowObjectText = 155,   // 'Display object text'
	kCommandClearObjectText = 156,
	kCommandWaitTalking = 157,
	kCommandMoveObjectBy = 158,
	kCommandMoveObjectTo = 159,
	kCommandIfCharacterDirection = 160
};

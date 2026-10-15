// The record callbacks tools/gen_callbacks.py could not translate mechanically
// (they create and link sub-objects, or compute something), written by hand
// from the same sources (Deponia_Linux.asm lines noted per function).
//
// OnCreate() runs when an object is *created* (TTable::CreateObject), which
// only the editor and the savegame code do; the player loads its objects from
// the data files. The savegame classes (TS*) are generated; the editor-side
// ones that are not reconstructed yet are empty and say so.

#include "vstables/records.h"

#include <cstdlib>
#include <ctime>

#include "TTAction.h"
#include "TTButton.h"
#include "TTScene.h"
#include "TTText.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

// The names a new action gets by where it hangs (asm lines 1478536-1478560:
// all three are initialized from the literal "Action").
wxString TTAction::strAction(L"Action");
wxString TTAction::strRightClickAction(L"Action");
wxString TTAction::strStartAction(L"Action");

// Confirmed (asm lines 1477463-1477598)
void TTAction::OnCreate(TVisionaireObject *object) {
	TVisionaireObject *parent = object->GetParent();
	if (!parent)
		return;

	if (parent->GetTId().getTable() == 0)
		object->SetName(TCharHolder(strAction));
	if (object->GetParentField() == kGameRightClickAction)
		object->SetName(TCharHolder(strRightClickAction));
	if (object->GetParentField() == kGameStartAction)
		object->SetName(TCharHolder(strStartAction));
	if (parent->GetTId().getTable() == 6)
		object->SetValue(kActionExecutionType, 6, TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 1478... TTAreaAction::OnCreate): an area action starts
// with a new action (of execution type 0x1D) and an "any" character.
void TTAreaAction::OnCreate(TVisionaireObject *object) {
	TVisObjRef parent(object);
	TVisObjRef action = object->GetVisionaire()->CreateObject(7, parent, kAreaActionAction);

	action.SetValue(kActionExecutionType, 0x1D, TSendEventEnum::kNoEvent);
	object->SetLinkAnyObject(kAreaActionCharacter, false);
}

// Confirmed: a new character gets a name text, a first outfit (its current
// one) and default scale/dialog values.
void TTCharacter::OnCreate(TVisionaireObject *object) {
	TVisObjRef self(object);

	object->GetVisionaire()->CreateObject(14, self, kCharacterName);
	TVisObjRef outfit = object->GetVisionaire()->CreateObject(17, self, kCharacterOutfits);
	self.SetLink(kCharacterCurrentOutfit, outfit, true);

	object->SetValue(kCharacterScale, true, TSendEventEnum::kNoEvent);
	object->SetValue(kCharacterScaleFactor, 100, TSendEventEnum::kNoEvent);
	object->SetValue(kCharacterDialogVerticalSpace, 10, TSendEventEnum::kNoEvent);
	object->SetValue(kCharacterActionDestPosition, wxPoint{-1, -1}, TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 1461061-1461702; editor only, but the test games are made with it): the defaults of a new game
// object - the loading record, the language, the first interface classes, the sizes of the picture buffers and a random
// game id.
void TTGame::OnCreate(TVisionaireObject *object) {
	TVisObjRef game(object);
	TVisionaire *visionaire = object->GetVisionaire();

	visionaire->CreateObject(21, game, kGameLoading);
	object->SetValue(kGameFadeDelay, 600, TSendEventEnum::kNoEvent);
	object->SetValue(kGameFadeEffect, 3, TSendEventEnum::kNoEvent);
	object->SetValue(kGameWindowResolution, wxPoint{640, 480}, TSendEventEnum::kNoEvent);
	object->SetValue(kGameScrollSpeed, 300, TSendEventEnum::kNoEvent);
	object->SetValue(kGameHoldTime, 600, TSendEventEnum::kNoEvent);
	object->SetValue(kGameMinificationFilter, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameMagnificationFilter, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameLeftHoldBehaviour, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameMiddleClickBehaviour, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameRightClickBehaviour, 0, TSendEventEnum::kNoEvent);

	TVisObjRef language = visionaire->CreateObject(18, game, kGameLanguages);

	object->SetLink(kGameStandardLanguage, language.GetObjectPointer()->GetTId(), false);
	object->SetValue(kGameCommandBehaviour, 2, TSendEventEnum::kNoEvent);
	object->SetValue(kGameAlwaysAllowSkipText, false, TSendEventEnum::kNoEvent);
	object->SetValue(kGameDrawActionText, 1, TSendEventEnum::kNoEvent);
	object->SetValue(kGameSceneComposedFiles, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameCharacterComposedFiles, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameInterfaceComposedFiles, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameGameComposedFiles, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameMovieComposedFiles, 2, TSendEventEnum::kNoEvent);
	object->SetValue(kGameSpeakerTextAlignment, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameTextAlignment, 2, TSendEventEnum::kNoEvent);
	object->SetValue(kGameKeepCharacterSpritesDuringMenus, true, TSendEventEnum::kNoEvent);
	object->SetValue(kGameDisableInteractionDuringAnim, 0, TSendEventEnum::kNoEvent);
	object->SetValue(kGameAutoHideInterfacesInMenu, true, TSendEventEnum::kNoEvent);

	static const wchar_t *const kInterfaceClassNames[] = {
		L"MainInterface", L"Inventory", L"SecondaryInterface", L"OptionsInterface", L"Miscellaneous"
	};

	for (const wchar_t *name : kInterfaceClassNames) {
		TVisObjRef interfaceClass = visionaire->CreateObject(31, game, kGameInterfaceClasses);

		interfaceClass.SetValue(kInterfaceClassName, wxString(name), TSendEventEnum::kSendEvent);
	}

	object->SetValue(kGamePictureCacheSize, 40, TSendEventEnum::kNoEvent);
	object->SetValue(kGameCursorHorizontalScrollDistance, 5, TSendEventEnum::kNoEvent);
	object->SetValue(kGameCursorVerticalScrollDistance, 5, TSendEventEnum::kNoEvent);
	object->SetValue(kGamePreloadPicThreads, 3, TSendEventEnum::kNoEvent);
	object->SetValue(kGamePreloadedPicBufferSize, 0x4000, TSendEventEnum::kNoEvent);
	object->SetValue(kGamePicBufferSize, 0x1D4C0, TSendEventEnum::kNoEvent);
	object->SetValue(kSpeakerSoundPanFactor, 8, TSendEventEnum::kNoEvent);
	object->SetValue(kGameExecuteCalledActionImmediately, true, TSendEventEnum::kNoEvent);

	// the id of the game is a random number
	std::srand(static_cast<unsigned int>(std::time(nullptr)));

	int identifier = std::rand();

	object->SetValue(kGameId, CONVTOSTR(identifier), TSendEventEnum::kNoEvent);
}

// Confirmed (asm lines 1466337-1466431; editor only): a new interface moves by one, scrolls its items by one and is
// visible, and takes the first interface class of the game.
void TTInterface::OnCreate(TVisionaireObject *object) {
	object->SetValue(kInterfaceDisplacement, 1, TSendEventEnum::kNoEvent);
	object->SetValue(kInterfaceScrollStepSize, 1, TSendEventEnum::kNoEvent);
	object->SetValue(kInterfaceDisplacement, 1, TSendEventEnum::kNoEvent);
	object->SetValue(kInterfaceVisible, true, TSendEventEnum::kNoEvent);

	TVList classes;

	object->GetVisionaire()->GetGame().GetLinks(kGameInterfaceClasses, TypeOrder::kValue1, classes);
	if (!classes.empty())
		object->SetLink(kInterfaceInterfaceClass, classes.front()->GetTId(), false);
}

void TTOutfit::OnCreate(TVisionaireObject * /*object*/) {
}

void TTParticles::OnCreate(TVisionaireObject * /*object*/) {
}

// The editor-list names of actions parts and texts (a long switch over the
// command / the text language) are not reconstructed; the plain name stands in.
wxString TTActionPart::GetNameInList(const TVisionaireObject *object) {
	return wxString(object->GetName());
}

wxString TTText::GetNameInList(const TVisionaireObject *object) {
	return wxString(object->GetName());
}

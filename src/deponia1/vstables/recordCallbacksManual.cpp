// The record callbacks tools/gen_callbacks.py could not translate mechanically
// (they create and link sub-objects, or compute something), written by hand
// from the same sources (Deponia_Linux.asm lines noted per function).
//
// OnCreate() runs when an object is *created* (TTable::CreateObject), which
// only the editor and the savegame code do; the player loads its objects from
// the data files. The savegame classes (TS*) are generated; the editor-side
// ones that are not reconstructed yet are empty and say so.

#include "vstables/records.h"

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

// Not reconstructed (editor only; asm lines 1461061-1461702): the defaults of
// a new game object, including a random game id.
void TTGame::OnCreate(TVisionaireObject * /*object*/) {
}

// Not reconstructed (editor only).
void TTInterface::OnCreate(TVisionaireObject * /*object*/) {
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

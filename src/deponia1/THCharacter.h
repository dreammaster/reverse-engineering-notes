// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 217481-218850, the constructor at 218783): the TGCharacter the game
// actually creates (TGameControl::InitCharacters makes one for every character link, 0x3A0 bytes): a
// TGCharacter that is also a TEventHandlerInterface (a second vtable at +0x398) and listens to its data
// object and to the outfit it wears. Like the other TH* classes it is the thin layer that connects the
// data object to the running game: a change of a field of the character is taken over when it happens
// (the position, the destination, the scene, the outfit, ...).
#pragma once

#include "TGCharacter.h"
#include "datastruct/eventhandler.h"
#include "datastruct/visobjref.h"

class THCharacter : public TGCharacter, public TEventHandlerInterface {
public:
	THCharacter(const TVisObjRef &self, const TVisObjRef &parent);
	~THCharacter() override;

	/** A change of a field of the character, or of its outfit (kOutfitCharacterSpeed and the animations'
	 *  kOutfitTalkAnimations). `object` is the value that was replaced, for the fields that link to another
	 *  object (the outfit, the scene). */
	void OnEvent(TEventEnum event, int field, TVisionaireObject *object) override;
};

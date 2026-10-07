// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 144161-144340, all 6 manifest-listed methods): the text the
// game actually creates (TGameControl::StartText(), StartObjectText() and Load() make one): a
// TGText that is also a TEventHandlerInterface (a second vtable at +0xA0) and listens to its
// record. Like the other TH* classes it is the thin layer that connects the data to the running
// game: when the record's position changes (a script moves the text) the text takes the position
// from the data from then on.
#pragma once

#include "TGText.h"
#include "datastruct/eventhandler.h"
#include "datastruct/visobjref.h"
#include "vscommon/fontManager.h"

class TGCharacter;

class THText : public TGText, public TEventHandlerInterface {
public:
	/** See TGText: `active` is the record of the text, `object` the text object. */
	THText(const TVisObjRef &active, const TVisObjRef &object, TGCharacter *character, const TVisObjRef &text,
	       TextAlignmentEnum alignment, const TVisObjRef &font, const wxPoint &pos, bool background, bool speech);
	// A second, 2-argument overload (TGameControl::Load, Deponia_Linux.asm
	// lines 476841-476846) - used to recreate a saved text without any of the other overload's
	// extra parameters (the alignment, position and the like are in the saved record, and
	// TGText::Load() restores the rest).
	THText(const TVisObjRef &active, const TVisObjRef &object);
	~THText() override;

	/** A change of a field of the text's record. */
	void OnEvent(TEventEnum event, int field, TVisionaireObject *object) override;
};

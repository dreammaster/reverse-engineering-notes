// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 114929-116060): the button the game actually creates
// (0x2B0 bytes): a TMButton that is also a TEventHandlerInterface (a second vtable at
// +0x288) and listens to its data object and to the variables of its condition. Like the
// other TH* classes it is the thin layer that connects the data object to the running game:
// a change of the rotation, scale, shader or matrix of the button is taken over by its two
// pictures, and a change of the value of its condition has the interface show the button
// (or hide it). It knows the interface it belongs to.
#pragma once

#include "TMButton.h"
#include "datastruct/eventhandler.h"
#include "datastruct/vlist.h"

class TGInterface;

class THButton : public TMButton, public TEventHandlerInterface {
public:
	THButton(const TVisObjRef &ref, TGInterface *owner);
	~THButton() override;

	/** A change of a field of the button (or of one of the variables its condition is
	 *  made of: kConditionValue). */
	void OnEvent(TEventEnum event, int field, TVisionaireObject *object) override;

	/** Listens to the variables a condition is made of (see ConditionListener.h). */
	void RegisterConditions(TVisObjRef &condition, TVList &path);
	/** Stops listening to them. */
	void UnRegisterConditions();

private:
	TVList _conditions;     // +0x290, the variables that are listened to
	TGInterface *_owner;    // +0x2A8, the interface the button is in
};

// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 113494-114760): the TGObject the game actually
// creates (TGScene::SetScene() makes one THObject for each of the scene's own object
// links, 0x340 bytes): a TGObject that is also a TEventHandlerInterface (a second vtable
// at +0x320) and listens to its data object and to the variables of its condition. Like
// the other TH* classes it is the thin layer that connects the data object to the running
// game: a change of the object's position, offset, visibility, scroll factors, scale,
// rotation or matrix is taken over when it happens, and a change of the value of its
// condition shows or hides it.
#pragma once

#include "TGObject.h"
#include "datastruct/eventhandler.h"
#include "datastruct/vlist.h"

class THObject : public TGObject, public TEventHandlerInterface {
public:
	explicit THObject(const TVisObjRef &ref);
	~THObject() override;

	/** A change of a field of the object (or of one of the variables its condition is
	 *  made of: kConditionValue). */
	void OnEvent(TEventEnum event, int field, TVisionaireObject *object) override;

	/** Listens to the variables a condition is made of (a condition is a variable, or two
	 *  conditions joined; `path` is the conditions that are being looked at, to find a
	 *  condition that refers to itself). */
	void RegisterConditions(TVisObjRef &condition, TVList &path);
	/** Stops listening to them. */
	void UnRegisterConditions();

private:
	TVList _conditions;  // +0x328, the variables that are listened to
};

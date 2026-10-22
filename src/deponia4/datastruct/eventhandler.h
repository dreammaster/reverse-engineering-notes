// Not yet assert-confirmed to a specific file; stays alongside datagrp.h.
//
// Confirmed (Deponia_Linux.asm lines 2989467-2989471, 2989395-2989420 and the
// TDataGroup code that calls it): the interface a data record notifies when
// one of its fields (or links) changes. Its vtable has a single pure virtual,
// OnEvent(TEventEnum, int, TVisionaireObject *) - implemented by THObject (at
// +0x320 of the object, hence the thunk) among others. A handler registers
// for one event kind; the field number and, for a link change, the object that
// was linked are passed along. Every registration seen passes event 1.
#pragma once

class TVisionaireObject;

/** The kinds of change a handler can register for; 1, "a value or link
 *  changed", is the only one in use (name invented). */
enum class TEventEnum : int {
	kChanged = 1
};

class TEventHandlerInterface {
public:
	virtual void OnEvent(TEventEnum event, int field, TVisionaireObject *object) = 0;

protected:
	~TEventHandlerInterface() = default;
};

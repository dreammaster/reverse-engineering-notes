// Not an original class: the part of THObject, THButton, THItem and THScene that is the
// same in all of them. The original has a copy of RegisterConditions() and
// UnRegisterConditions() in each of these classes (Deponia_Linux.asm lines 113666-114205,
// 115101-115640, ...); they are the same code, so it is written once here and each class's
// own methods (which keep their recovered names) call it.
//
// An object or button is shown only when its condition is met (kObjectCondition,
// kButtonCondition...). A condition is a variable, or two conditions joined; what has to be
// listened to is each variable, so the owner hears when the condition's value changes
// (kConditionValue) and can show or hide itself.
#pragma once

#include "datastruct/eventhandler.h"
#include "datastruct/visobjref.h"
#include "datastruct/vlist.h"

/** Registers `handler` for the change of every variable of `condition` and adds them to
 *  `variables`. `path` is the compound conditions that are being looked at (empty at the
 *  start), to find a condition that refers to itself: that is logged, not followed. */
void RegisterConditionHandlers(TEventHandlerInterface *handler, TVisObjRef &condition, TVList &path,
                               TVList &variables);

/** Stops listening to all the variables in `variables` and forgets them. */
void UnRegisterConditionHandlers(TEventHandlerInterface *handler, TVList &variables);

// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed a real, opaque enum with individually-significant values
// spanning 0-36 (TManagedObject::GetActionsToTest()/
// ExecuteMatchingAction(), TTAction::IsImmediateExecutionType(),
// Deponia_Linux.asm) - ExecuteMatchingAction()'s own switch computes
// `value - 3` and jump-tables on it up to 25, confirming 3-28 inclusive
// each get individual handling there; 0-2 and 29-36 all otherwise fall
// back to its generic "does this candidate action's own type equal this
// value" match (29-33 aren't directly confirmed to occur, but are named
// here for completeness alongside the confirmed 34-36). None of these
// values have a resolved real-world meaning (they almost certainly
// correspond to Visionaire editor's own named action-type dropdown, which
// isn't available to this project), so they're named by raw value, the
// same convention already used for TypeOrder/eVisionaireTable.
#pragma once

enum class TypeActionExecution {
	kValue0 = 0, kValue1 = 1, kValue2 = 2, kValue3 = 3, kValue4 = 4, kValue5 = 5,
	kValue6 = 6, kValue7 = 7, kValue8 = 8, kValue9 = 9, kValue10 = 10, kValue11 = 11,
	kValue12 = 12, kValue13 = 13, kValue14 = 14, kValue15 = 15, kValue16 = 16, kValue17 = 17,
	kValue18 = 18, kValue19 = 19, kValue20 = 20, kValue21 = 21, kValue22 = 22, kValue23 = 23,
	kValue24 = 24, kValue25 = 25, kValue26 = 26, kValue27 = 27, kValue28 = 28, kValue29 = 29,
	kValue30 = 30, kValue31 = 31, kValue32 = 32, kValue33 = 33, kValue34 = 34, kValue35 = 35,
	kValue36 = 36
};

// Confirmed a small, real out-param struct (TManagedObject::
// GetActionsToTest()/ExecuteMatchingAction()/HandlePostExecution(),
// Deponia_Linux.asm) - real size ~0x20 bytes (seen allocated as such in
// ExecuteEvent, though that allocation dance itself isn't replicated - see
// TManagedObject.cpp). `matchedType` and the flags below are all confirmed
// read/written at these exact positions across the three methods that
// share this struct; none have a resolved semantic meaning beyond their
// observed behavior (documented at each flag), so they're named by
// position like TGDetectInfo's flagA/flagB.
class TGActionInfo {
public:
	// Confirmed written whenever a match is found (GetActionsToTest()'s
	// default-case fallback matches, or ExecuteMatchingAction() accepts a
	// candidate action) - the raw TypeActionExecution-shaped value (or, in
	// GetActionsToTest(), the candidate action's own "type" field) that
	// matched.
	int matchedType = 0;
	// Confirmed set true only alongside a match that's also either
	// "reached" (flag6) or TTAction::IsImmediateExecutionType(matchedType)
	// - gates AddRunningAction() in ExecuteMatchingAction() and the
	// "do nothing" early-out at the top of HandlePostExecution().
	bool flag4 = false;
	// Confirmed set true in the complementary case to flag4 (matched, not
	// reached, not an immediate-execution type, and the action's own
	// command isn't an "any object" wildcard) - HandlePostExecution()
	// checks this directly to decide whether to retry via
	// TGCharacter::ShowComment() instead.
	bool flag5 = false;
	// Confirmed set true by GetActionsToTest() when the event's own
	// character is within reach of this object (or the current scene's
	// own field 0x124 is set) - read back by several of its own switch
	// cases and by HandlePostExecution().
	bool flag6 = false;
	// Confirmed set true alongside flag6 in several (not all)
	// GetActionsToTest() switch cases - a more specific "reached and this
	// particular candidate type" qualifier; read by HandlePostExecution()
	// to decide whether its ShowComment()-retry path even applies.
	bool flag7 = false;
	// Confirmed set true in exactly one GetActionsToTest() switch case
	// (mouse-event case 1, when this object's own game-data reference is
	// non-empty) - read by HandlePostExecution() to decide whether to
	// clear the game's own "reached object" link (0x2AE) and whether to
	// tell TGObjectManager to drop whatever's held.
	bool flag8 = false;
	// Confirmed read by HandlePostExecution() (gates its own top-level
	// early-out alongside flag4) but never written by GetActionsToTest()
	// or ExecuteMatchingAction() - presumably set by an overriding
	// subclass's own HandlePostExecution() before it forwards to this
	// base, or by a caller not covered by this pass. Always false here.
	bool flag9 = false;
	// Confirmed set true when a match's own command resolves to an "any
	// object" wildcard (TVisObjRef::IsAnyObject()) - read by
	// HandlePostExecution() alongside flag9 at its very top.
	bool flagA = false;
	// Confirmed set true in GetActionsToTest()'s non-reached fallback
	// cases once a default-case candidate type is pushed - marks that at
	// least one "not reached" candidate exists, independent of flag6.
	bool flagB = false;
};

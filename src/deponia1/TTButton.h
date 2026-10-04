// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed a zero-overhead TVisObjRef subclass - every TTButton ctor seen
// so far is a tail call straight into the matching TVisObjRef ctor, with no
// extra fields of its own (TGObjectManager::HandleEvent/
// IsCurrentObjectWalkable, Deponia_Linux.asm lines 1525148-1525168). Only
// the one method this project currently needs is modeled here; the rest of
// TTButton's real surface (manifest: 12 methods) isn't reversed yet.
// TTObject - a same-shaped base this presumably derives from in the
// original, per the same zero-overhead-ctor evidence seen at other TTButton/
// TTObject call sites - isn't modeled at all yet, since nothing here needs
// it.
#pragma once

#include "datastruct/typegrp.h"

#include "datastruct/visobjref.h"
#include "vstables/fieldIds.h"

class TTButton : public TVisObjRef {
public:
	// Recovered from the binary's schema (vstables/records.cpp).
	static TTypeGroup &GetTypeGroup();
	static void InitType(int versionLow, int versionHigh);
	static void OnCreate(TVisionaireObject *object);
	static void OnInit(TVisionaireObject *object);

	TTButton() = default;
	explicit TTButton(const TVisObjRef &ref) : TVisObjRef(ref) {
	}

	// Confirmed in full (Deponia_Linux.asm lines 1525850-1525903): this
	// button's parent object's own "standard command" link (field 0x12A)
	// points back at this button itself.
	bool IsStandardCommand() const {
		return GetParent().GetLink(kInterfaceStandardCommand) == *this;
	}
	// Confirmed in full (Deponia_Linux.asm lines 1525911-1525926): its own
	// "type" field (0x129) is one of two specific values - real meaning of
	// 3/6 not resolved.
	bool IsCommand() const {
		int type = GetInt(kButtonType);
		return type == 6 || type == 3;
	}
};

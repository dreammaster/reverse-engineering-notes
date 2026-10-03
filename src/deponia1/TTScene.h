// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed a TVisObjRef-derived "typed handle onto a scene data record"
// (recovered RTTI; TGScene embeds one at +0x48, TMSavegame::MakeSaveGameName()
// builds a temporary from a plain TVisObjRef). Only the constructor from a
// plain TVisObjRef and GetNameLanguage() are confirmed call shapes; the
// latter's lookup (the scene's name in the current text language) is not
// reversed.
#pragma once

#include "WxStub.h"
#include "datastruct/visobjref.h"

class TTScene : public TVisObjRef {
public:
	TTScene() = default;
	explicit TTScene(const TVisObjRef &ref) : TVisObjRef(ref) {
	}

	wxString GetNameLanguage() const {
		return wxString();
	}
};

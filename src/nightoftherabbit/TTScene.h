// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed a TVisObjRef-derived "typed handle onto a scene data record"
// (recovered RTTI; TGScene embeds one at +0x48, TMSavegame::MakeSaveGameName()
// builds a temporary from a plain TVisObjRef). Its methods are in TTScene.cpp.
#pragma once

#include "datastruct/typegrp.h"

#include <vector>

#include "TCharHolder.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"

class TTScene : public TVisObjRef {
public:
	// Recovered from the binary's schema (vstables/records.cpp).
	static TTypeGroup &GetTypeGroup();
	static void InitType(int versionLow, int versionHigh);
	static void OnCreate(TVisionaireObject *object);
	static void OnInit(TVisionaireObject *object);

	TTScene() = default;
	explicit TTScene(const TVisObjRef &ref) : TVisObjRef(ref) {
	}

	/** The first two points of the scene's points as a rectangle - the x and y of the first, then the x and y of the
	 *  second in the place of the width and the height (the points are -1, -1 where there are none). */
	wxRect GetLoadingRect();
	/** The name of the scene in the current text language (empty for an empty reference). */
	wxString GetNameLanguage() const;
	/** The files of the sounds that the actions of the scene start (the commands to play a sound, the scene music, loop a sound
	 *  and set the walking sound); the vector is emptied first. */
	void GetSoundsOnScene(std::vector<TCharHolder> &sounds) const;
};

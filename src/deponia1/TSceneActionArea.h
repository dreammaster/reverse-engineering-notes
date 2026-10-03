// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed call shape only (TGScene::InitActionAreas(), Deponia_Linux.asm
// lines 169215-169506): a 0x38-byte object built from a TVisObjRef of one of
// a scene's "action area" links (field 0x2A9); not reversed beyond that
// constructor/destructor pair.
#pragma once

#include "datastruct/visobjref.h"

class TSceneActionArea {
public:
	explicit TSceneActionArea(const TVisObjRef &ref) : _ref(ref) {
	}
	~TSceneActionArea() = default;

private:
	TVisObjRef _ref;
};

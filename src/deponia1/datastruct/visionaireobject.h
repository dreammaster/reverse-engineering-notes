// Not yet assert-confirmed to a specific file; stays at the top level of
// datastruct/ alongside visobjref.h/visionaire.h, its evident siblings.
//
// The engine's underlying game-data record; TVisObjRef is a lightweight
// handle/reference onto one (confirmed: TVList holds TVisionaireObject*
// elements, and TVisObjRef has a converting constructor from one - see
// TGameControl::InitInterfaces, Deponia_Linux.asm lines 458124-458250).
// Nothing about this class's own fields/methods has been reversed.
#pragma once

#include <cstdint>

#include "TCharHolder.h"

class TVisionaireObject {
public:
	TVisionaireObject() = default;

	// Confirmed call shape only (TGameControl::InitGameActions, asm lines
	// 467044-467065): called directly on a raw TVisionaireObject* pulled from
	// a TVList, unlike the rest of this survey's field access which goes
	// through a TVisObjRef handle - not reversed beyond that.
	int GetInt(int fieldId) const;
	// Confirmed call shapes only (TGameControl::LoadAndInitGame,
	// Deponia_Linux.asm lines 468017-468037) - same "no field id, a property
	// of the object itself" shape as TVisObjRef::GetName(); GetLink() takes a
	// field id like TVisObjRef::GetLink() but returns a raw
	// TVisionaireObject* rather than a TVisObjRef by value.
	TCharHolder GetName() const;
	TVisionaireObject *GetLink(int fieldId) const;
	// Confirmed call shape only (TGameControl::InitScripts, Deponia_Linux.asm
	// line 458562) - not reversed beyond that.
	wxString GetStr(int fieldId) const;
	// Confirmed call shape only (asm line 468037) - same 3-4 byte packed id
	// shape as TVisObjRef::GetId(), see that method's own comment.
	const std::uint8_t *GetId() const;

private:
	std::uint8_t _id[4] {};
};

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

// Packs a TVisObjRef::GetId()/TVisionaireObject::GetId() 3-byte id into a
// 32-bit value the same way every confirmed hash-lookup site does it
// (TGameControl::StartDialog/EndDialog/GetCharacter, TFontManager::
// GetFont/SetCurrentFont/Initialize, and others): byte0 | (byte1<<8) |
// (sign-extended byte2<<16) - the sign extension of the third byte is
// confirmed (an `and 0xFF000000` masking a `sar 0x1F`-derived sign mask in
// the disassembly), its purpose is not.
inline int PackVisId(const std::uint8_t *id) {
	return id[0] | (id[1] << 8) | (static_cast<int>(static_cast<std::int8_t>(id[2])) << 16);
}

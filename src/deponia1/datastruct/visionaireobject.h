// Not yet assert-confirmed to a specific file; stays at the top level of
// datastruct/ alongside visobjref.h/visionaire.h, its evident siblings.
//
// The engine's underlying game-data record; TVisObjRef is a lightweight
// handle/reference onto one (confirmed: TVList holds TVisionaireObject*
// elements, and TVisObjRef has a converting constructor from one - see
// TGameControl::InitInterfaces, Deponia_Linux.asm lines 458124-458250).
// Nothing about this class's own fields/methods has been reversed.
#pragma once

class TVisionaireObject {
public:
	TVisionaireObject() = default;
};

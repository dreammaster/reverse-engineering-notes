// Not yet assert-confirmed to a specific file; stays at the top level of
// datastruct/ alongside visobjref.h/visionaire.h, its evident siblings.
//
// TVList is passed by reference into TVisionaire::GetList(int, TVList&,
// bool) (TGameControl::InitFonts, asm lines 458293-458331: the caller
// zero-initializes 24 stack bytes in place of calling a visible
// constructor, matching the 3-pointer shape of a vector-like container)
// and into TVisObjRef::GetLinks(int, TypeOrder, TVList&)
// (TGameControl::InitInterfaces/InitGameActions, asm lines 458124-458250,
// 466739-467222+: iterated via explicit begin()/end() calls whose elements
// are TVisionaireObject* - each one converted to a TVisObjRef via its
// converting constructor before use - not TVisObjRef directly, contradicting
// the type this was first modeled with for InitFonts, which never actually
// inspects an element).
#pragma once

#include <vector>

#include "datastruct/visobjref.h"

class TVisionaireObject;

class TVList {
public:
	void clear() {
		items.clear();
	}
	bool empty() const {
		return items.empty();
	}
	// Confirmed call shape only (TGameControl::LoadAndInitGame,
	// Deponia_Linux.asm line 467937).
	std::size_t size() const {
		return items.size();
	}
	// Confirmed call shape only (asm line 468190).
	TVisionaireObject *front() const {
		return items.front();
	}
	std::vector<TVisionaireObject *>::iterator begin() {
		return items.begin();
	}
	std::vector<TVisionaireObject *>::iterator end() {
		return items.end();
	}
	// Confirmed call shape only (TGameControl::HandleMouseMove,
	// Deponia_Linux.asm line 472191) - a plain copy-assignment-style snapshot
	// of another list's elements.
	void copy(const TVList &other) {
		items = other.items;
	}
	// Confirmed call shape only (TGameControl::HandleMouseMove, Deponia_Linux.
	// asm line 472341) - passed a TGInterface's own TVisObjRef field there.
	// TVisObjRef doesn't carry a real backing TVisionaireObject* pointer in
	// this reconstruction (see visobjref.h's own header comment: it's a
	// field-value stub, not a real handle), so there is nothing genuine to
	// append to items - left as a no-op rather than fabricating a pointer.
	void push_back(const TVisObjRef &/*ref*/) {
	}

	std::vector<TVisionaireObject *> items;
};

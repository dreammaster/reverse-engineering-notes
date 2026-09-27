// Not yet assert-confirmed to a specific file; stays at the top level of
// datastruct/ alongside visobjref.h/visionaire.h, its evident siblings.
//
// TVList is passed by reference into TVisionaire::GetList(int, TVList&,
// bool) (TGameControl::InitFonts, asm lines 458293-458331: the caller
// zero-initializes 24 stack bytes in place of calling a visible
// constructor, matching the 3-pointer shape of a vector-like container)
// and into TVisObjRef::GetLinks(int, eTypeOrder, TVList&)
// (TGameControl::InitInterfaces/InitGameActions, asm lines 458124-458250,
// 466739-467222+: iterated via explicit begin()/end() calls whose elements
// are TVisionaireObject* - each one converted to a TVisObjRef via its
// converting constructor before use - not TVisObjRef directly, contradicting
// the type this was first modeled with for InitFonts, which never actually
// inspects an element).
#pragma once

#include <vector>

class TVisionaireObject;

class TVList {
public:
    void clear() { items.clear(); }
    bool empty() const { return items.empty(); }
    std::vector<TVisionaireObject*>::iterator begin() { return items.begin(); }
    std::vector<TVisionaireObject*>::iterator end() { return items.end(); }

    std::vector<TVisionaireObject*> items;
};

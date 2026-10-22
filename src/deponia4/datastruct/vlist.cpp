#include "datastruct/vlist.h"

#include "Diagnostics.h"
#include "datastruct/visionaireobject.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/datastruct/visionaireobject.cpp";

TVList::TVList(const TVList &other) {
	copy(other);
}

// Confirmed (asm lines 586522-586596)
TVList::~TVList() {
	clear();
}

TVList &TVList::operator=(const TVList &other) {
	if (this != &other)
		copy(other);
	return *this;
}

// Confirmed (asm lines 586597-586647)
void TVList::clear() {
	for (TVisionaireObject *object : items)
		object->Release();
	items.clear();
}

// Confirmed (asm lines 586648-586719)
TVList::iterator TVList::erase(iterator position) {
	(*position)->Release();
	return items.erase(position);
}

// Confirmed (asm lines 586881-586939)
void TVList::pop_back() {
	items.back()->Release();
	items.pop_back();
}

// Confirmed (asm lines 594219-594289)
void TVList::push_back(TVisionaireObject *object) {
	x_assert(object != nullptr, "obj != NULL", kSourceFile, 0x60);
	object->GetReference();
	items.push_back(object);
}

void TVList::push_back(const TVisObjRef &ref) {
	TVisionaireObject *object = ref.GetObjectPointer();
	if (object)
		push_back(object);
}

// Confirmed (asm lines 593980-594113)
TVList::iterator TVList::insert(iterator position, TVisionaireObject *object) {
	x_assert(object != nullptr, "obj != NULL", kSourceFile, 0x60);
	object->GetReference();
	return items.insert(position, object);
}

TVList::iterator TVList::insert(iterator position, const TVisObjRef &ref) {
	TVisionaireObject *object = ref.GetObjectPointer();
	if (!object)
		return position;
	return insert(position, object);
}

// Confirmed (asm lines 594114-594218 and 594785-594888)
void TVList::copy(const TVList &other) {
	clear();
	for (TVisionaireObject *object : other.items)
		push_back(object);
}

#include "vstables/records.h"

#include "datastruct/visionaireobject.h"

// Confirmed (asm lines 1468584-1468606): the sort function of the scene's
// objects - by their order (despite the name).
bool TTObject::cmpY(const TVisionaireObject *a, const TVisionaireObject *b) {
	return a->GetOrder() < b->GetOrder();
}

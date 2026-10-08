#include "vstables/records.h"

#include "TTText.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

// Confirmed (asm lines 1468584-1468606): the sort function of the scene's
// objects - by their order (despite the name).
bool TTObject::cmpY(const TVisionaireObject *a, const TVisionaireObject *b) {
	return a->GetOrder() < b->GetOrder();
}

// Confirmed (asm lines 1468407-1468574)
wxString TTObject::GetLanguageName() const {
	if (IsEmpty())
		return wxString();

	TTText name(GetLink(kObjectName));
	wxString text = name.GetTextString();

	if (text.IsEmpty())
		text = wxString(GetName());

	return text;
}

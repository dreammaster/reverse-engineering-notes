#include "THText.h"

#include "vstables/fieldIds.h"

// Confirmed (asm lines 144330-144368)
THText::THText(const TVisObjRef &active, const TVisObjRef &object, TGCharacter *character, const TVisObjRef &text,
               TextAlignmentEnum alignment, const TVisObjRef &font, const wxPoint &pos, bool background, bool speech)
	: TGText(active, object, character, text, alignment, font, pos, background, speech) {
	_active.RegisterEventHandler(this, TEventEnum::kChanged);
}

// Confirmed (asm lines 144287-144312)
THText::THText(const TVisObjRef &active, const TVisObjRef &object) : TGText(active, object) {
	_active.RegisterEventHandler(this, TEventEnum::kChanged);
}

// Confirmed (asm lines 144203-144229)
THText::~THText() {
	_active.UnRegisterEventHandler(this);
}

// Confirmed (asm lines 144162-144174)
void THText::OnEvent(TEventEnum /*event*/, int field, TVisionaireObject * /*object*/) {
	if (field == kTextPosition)
		_positionSet = true;
}

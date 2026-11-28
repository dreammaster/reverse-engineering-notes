#include "TTButton.h"

#include "TTText.h"
#include "datastruct/visionaireobject.h"
#include "vstables/records.h"

bool TTButton::IsActive() const {
	TTCondition condition(GetLink(kButtonCondition));

	return condition.IsTrue() != GetBool(kButtonConditionNegate);
}

wxString TTButton::GetLanguageName() const {
	if (IsEmpty())
		return wxString();

	TTText text(GetLink(kButtonName));

	return text.GetTextString();
}

wxString TTButton::GetLanguageConjunctionName() const {
	if (IsEmpty())
		return wxString();

	TTText text(GetLink(kButtonConjunction));

	return text.GetTextString();
}

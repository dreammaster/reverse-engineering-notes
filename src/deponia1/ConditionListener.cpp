#include "ConditionListener.h"

#include <cwchar>
#include <string>

#include "WxStub.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

// Adds "'name' (id: n)" and the suffix to the text of the way down to a condition.
static void appendCondition(std::wstring &text, const TVisObjRef &condition, const wchar_t *suffix) {
	std::wstring name = condition.GetName().c_str().ToStdWstring();
	wchar_t number[16];

	swprintf(number, 16, L"%d", PackVisId(condition.GetId()));
	text += L"'" + name + L"' (id: " + number + L")" + suffix;
}

// Confirmed (THObject::RegisterConditions(), asm lines 113666-114205, and the same code
// in THButton, 115101-115640): a condition that is a variable is listened to; one that is
// made of two others lists them (one level at a time: a condition that is already on the way
// down is a cycle, which is logged).
void RegisterConditionHandlers(TEventHandlerInterface *handler, TVisObjRef &condition, TVList &path,
                               TVList &variables) {
	if (condition.GetBool(kConditionIsVariable)) {
		condition.RegisterEventHandler(handler, TEventEnum::kChanged);
		variables.push_back(condition);
		return;
	}

	for (TVisionaireObject *item : path) {
		if (condition == *item) {
			if (wxLog::loglevel > 0) {
				// the way down to it
				std::wstring chain;

				for (TVisionaireObject *step : path)
					appendCondition(chain, TVisObjRef(step), L" - ");

				appendCondition(chain, condition, L"");

				TVisObjRef first(path.front());

				wxLog::logexpanded(L"Condition '%s' (id: %d) has a cyclic reference: %s", first.GetName().c_str().wc_str(),
				                   PackVisId(first.GetId()), wxString(chain).wc_str());
			}
			return;
		}
	}

	path.push_back(condition);

	TVisObjRef first = condition.GetLink(kConditionCondition1);
	TVisObjRef second = condition.GetLink(kConditionCondition2);

	if (!first.IsEmpty())
		RegisterConditionHandlers(handler, first, path, variables);
	if (!second.IsEmpty())
		RegisterConditionHandlers(handler, second, path, variables);

	path.pop_back();
}

// Confirmed (THObject::UnRegisterConditions(), asm lines 114721-114752)
void UnRegisterConditionHandlers(TEventHandlerInterface *handler, TVList &variables) {
	for (TVisionaireObject *item : variables)
		TVisObjRef(item).UnRegisterEventHandler(handler);

	variables.clear();
}

#include "vstables/records.h"

#include "datastruct/vlist.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

// Confirmed (asm lines 1449977-1450603): a condition is either a variable (its
// value is the field kConditionValue) or a combination of two other conditions
// (each possibly negated) with AND/OR, possibly negated as a whole. `evaluating`
// holds the conditions being evaluated, to find cyclic references: such a
// condition is logged and false.
bool TTCondition::IsTrue(TVList &evaluating) const {
	if (IsEmpty())
		return true;

	if (GetBool(kConditionIsVariable))
		return GetBool(kConditionValue);

	for (TVisionaireObject *condition : evaluating) {
		if (!(*this == *condition))
			continue;

		if (wxLog::loglevel > 0) {
			std::wstring chain;

			for (TVisionaireObject *entry : evaluating) {
				wchar_t text[512];

				swprintf(text, 512, L"'%ls' (id: %d) ", entry->GetName().c_str().c_str(), entry->GetId24());
				chain += text;
			}
			wchar_t last[512];
			swprintf(last, 512, L"'%ls' (id: %d)", GetName().c_str().c_str(), GetObjectPointer()->GetId24());
			chain += last;

			wxLog::logexpanded(L"Tried to evaluate condition '%ls' (id: %d) which has a cyclic reference: %ls",
			                   evaluating.front()->GetName().c_str().c_str(), evaluating.front()->GetId24(),
			                   chain.c_str());
		}
		return false;
	}

	TTCondition first(GetLink(kConditionCondition1));
	TTCondition second(GetLink(kConditionCondition2));

	evaluating.push_back(*this);
	bool a = first.IsTrue(evaluating);
	bool b = second.IsTrue(evaluating);
	evaluating.pop_back();

	if (GetBool(kConditionCondition1Negate))
		a = !a;
	if (GetBool(kConditionCondition2Negate))
		b = !b;

	bool result;
	if (GetInt(kConditionOperator) == 0)
		result = a && b;
	else
		result = (GetInt(kConditionOperator) == 1) ? (a || b) : a;

	// a missing operand leaves the other one
	if (first.IsEmpty())
		result = b;
	if (second.IsEmpty())
		result = a;

	if (GetBool(kConditionReturnNegate))
		result = !result;
	return result;
}

// Confirmed (asm lines 1450604-1450647)
bool TTCondition::IsTrue() const {
	TVList evaluating;

	return IsTrue(evaluating);
}

// Confirmed (asm lines 1450648-1450702): sets the value of a variable
// condition; false if this isn't one or it already has that value.
bool TTCondition::SetTo(bool value) {
	if (IsEmpty())
		return false;
	if (!GetBool(kConditionIsVariable))
		return false;
	if (GetBool(kConditionValue) == value)
		return false;

	SetValue(kConditionValue, value, TSendEventEnum::kSendEvent);
	return true;
}

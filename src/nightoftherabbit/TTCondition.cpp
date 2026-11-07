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

	// Night of the Rabbit (read in its listing): a condition combines any number of conditions, ConditionConditions (links), each
	// inverted when its flag in ConditionConditionsNegate (a list of booleans) says so; ConditionOperator 0 is the AND of all of
	// them, 1 the OR of all of them, any other operator gives false. Condition1 and Condition2 are only read by the upgrade of
	// projects of the older format (TVisionaireGame::UpdateVersion).
	TVList conditions;
	std::vector<bool> negate;

	GetLinks(kConditionConditions, TypeOrder::kValue0, conditions);
	GetBools(kConditionConditionsNegate, negate);

	evaluating.push_back(*this);

	std::vector<bool> results;
	size_t index = 0;

	for (TVisionaireObject *object : conditions) {
		if (object && !object->IsEmpty()) {
			TTCondition condition{TVisObjRef(object)};
			bool value = condition.IsTrue(evaluating);

			if (index < negate.size() && negate[index])
				value = !value;
			results.push_back(value);
		}
		index++;
	}
	evaluating.pop_back();

	// (the value of ConditionIsVariable, false here, when there is no result at all)
	bool result = false;

	if (results.size() == 1) {
		result = results[0];
	} else if (results.size() > 1) {
		int op = GetInt(kConditionOperator);

		if (op == 0) {
			result = results[0];
			for (size_t i = 1; i < results.size(); i++)
				result = result && results[i];
		} else if (op == 1) {
			result = results[0];
			for (size_t i = 1; i < results.size(); i++)
				result = result || results[i];
		}
	}

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

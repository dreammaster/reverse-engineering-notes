#include "TTAction.h"

#include "vstables/fieldIds.h"

TVisObjRef TTAction::GetCommand() const {
	return GetLink(kActionCommand);
}

int TTAction::GetTypeAction() const {
	return GetInt(kActionExecutionType);
}

TVisObjRef TTAction::GetFixture() const {
	return GetLink(kActionFixture);
}

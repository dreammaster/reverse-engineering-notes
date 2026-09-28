#include "datastruct/visionaire.h"

TVisObjRef TVisionaire::GetGame() const {
	return TVisObjRef();
}

void TVisionaire::GetList(int /*fieldId*/, TVList &/*outList*/, bool /*flag*/) const {
}

TVisObjRef TVisionaire::CreateActiveObject(int /*typeId*/, const TVisObjRef &/*source*/) {
	return TVisObjRef();
}

TVisObjRef TVisionaire::GetEmptyObject() const {
	return TVisObjRef();
}

#include "vscommon/scripting/id.h"

TId::TId(int /*a*/, int /*b*/) {
}

bool TId::operator==(const TId &/*other*/) const {
	return false;
}

void UnrefLuaFieldsCache(const TId &/*id*/, int /*value*/) {
}

TId AnyId(-1, -1);

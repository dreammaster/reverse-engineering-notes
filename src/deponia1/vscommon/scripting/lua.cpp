#include "vscommon/scripting/lua.h"

void LuaDoString(const std::string &/*code*/, const std::string &/*chunkName*/) {
}

void LuaDoString(const std::string &/*code*/) {
}

void LuaDoRef(int /*ref*/) {
}

void LuaDebugName(const char */*name*/) {
}

void LuaExecuteEventHandler(const std::string &/*handler*/, const TVisObjRef &/*object*/) {
}

std::string IdStrStd(const std::uint8_t */*id*/) {
	return std::string();
}

TVisionaire *GetLuaGame() {
	return nullptr;
}

bool FindObjectByNameOrId(const wxString &/*nameOrId*/, TVisObjRef &/*outObject*/, bool /*flag*/) {
	return false;
}

void LuaSetCurrentAction(const TVisObjRef &/*action*/) {
}

#include "vscommon/scripting/lua.h"

void LuaDoRef(int /*ref*/) {
}

void LuaDebugName(const char */*name*/) {
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

void LuaSetNumber(const std::string &/*name*/, double /*value*/) {
}

// Not reconstructed yet (visionaireobjectLua.cpp and the commands): nothing is put into Lua.
#include "vscommon/scripting/luaConversion.h"
#include "vscommon/scripting/visLuaObjects.h"

LuaVisionaireObject *CheckVisionaireObject(lua_State *, int, bool) {
	return nullptr;
}

int luaopen_VisionaireObject(lua_State *) {
	return 0;
}

int luaopen_Sprite(lua_State *) {
	return 0;
}

int luaopen_Particles(lua_State *) {
	return 0;
}

void InitCommonCommands() {
}

void ConvertToLua(const TVisObjRef &) {
	lua_pushnil(L);
}

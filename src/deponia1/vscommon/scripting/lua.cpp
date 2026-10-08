// Confirmed (Deponia_Linux.asm lines 1399990-1401184 and 1401184-1405988, src/vscommon/scripting/lua.cpp
// by its place before the constants): the odds and ends of the Lua state - closing it, running a file or a
// reference of the registry, the conversion of a data object and of a list of them from and to Lua - and, in
// luaGlobals.cpp, the constants the scripts get.
#include "vscommon/scripting/lua.h"

#include "Diagnostics.h"
#include "common/lua/lauxlib.h"
#include "datastruct/visionaire.h"
#include "vscommon/scripting/luaConversion.h"
#include "vscommon/scripting/visLuaObjects.h"
#include "vstables/visionaireGame.h"

static const char *s_currentCaller = nullptr;

static wxString utf(const char *text) {
	wxString result;

	toUTF(&result, text ? text : "");
	return result;
}

// Confirmed (asm lines 119631-119635): the name of the Lua function that runs, for the profiler.
void LuaDebugName(const char *name) {
	s_currentCaller = name;
}

// Confirmed (asm lines 1400299-1400326)
void CloseLua() {
	if (L) {
		lua_close(L);
		L = nullptr;
	}

	L = nullptr;
	ClearLuaObjectCaches();
}

// Confirmed (asm lines 1400326-1400342)
TVisionaire *GetLuaGame() {
	return luaGame;
}

// Confirmed (asm lines 1400342-1400596): the results of a file that ran well are left on the stack.
void LuaDoFile(const wxFileName &file) {
	std::string path(file.GetFullPath().mb_str());
	bool failed = true;

	if (luaL_loadfile(L, path.c_str()) == 0)
		failed = lua_pcall(L, 0, LUA_MULTRET, 0) != 0;

	if (!failed)
		return;

	const char *message = lua_tolstring(L, -1, nullptr);

	if (wxLog::loglevel >= 0)
		wxLog::logexpanded(L"Failed to load and run file '%s' in Lua: %s", file.GetFullPath().wc_str(),
		                   utf(message).wc_str());

	lua_settop(L, -2);
}

// Confirmed (asm lines 1400596-1400772)
void LuaDoFile(const char *path) {
	if (luaL_loadfile(L, path) == 0 && lua_pcall(L, 0, LUA_MULTRET, 0) == 0)
		return;

	const char *message = lua_tolstring(L, -1, nullptr);

	if (wxLog::loglevel >= 0)
		wxLog::logexpanded(L"Failed to load and run file '%s' in Lua: %s", utf(path).wc_str(), utf(message).wc_str());

	lua_settop(L, -2);
}

// Confirmed (asm lines 1400972-1401184): runs the function that the registry holds under `ref` (the delayed
// calls of the game), under the error handler in the player.
void LuaDoRef(int ref) {
	if (TVisionaire::IsVisPlayerMode)
		lua_getfield(L, LUA_GLOBALSINDEX, "debugfunc");

	int top = lua_gettop(L);

	debugger.BeginArea(ProfileArea::kValue2, std::string(), -1);
	lua_rawgeti(L, LUA_REGISTRYINDEX, ref);

	if (lua_pcall(L, 0, LUA_MULTRET, TVisionaire::IsVisPlayerMode ? lua_gettop(L) - 1 : 0) != 0) {
		const char *message = lua_tolstring(L, -1, nullptr);

		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Failed to run reference in Lua: %s", utf(message).wc_str());

		lua_settop(L, -2);
	}

	if (TVisionaire::IsVisPlayerMode)
		lua_remove(L, top);

	profileClean();
	debugger.EndArea(ProfileArea::kValue2, -1);
}

// Confirmed call shape (TGAction::Execute, asm lines 201760-201830): the global `currentAction` is the
// action that runs a script (when there is none, the empty object).
void LuaSetCurrentAction(const TVisObjRef &action) {
	ConvertToLua(action);
	lua_setfield(L, LUA_GLOBALSINDEX, "currentAction");
}

// Confirmed call shape (TGameControl::Update, asm lines 619880-619890): a number in a global variable.
void LuaSetNumber(const std::string &name, double value) {
	lua_pushnumber(L, value);
	lua_setfield(L, LUA_GLOBALSINDEX, name.c_str());
}

// Confirmed (asm lines 1400013-1400046)
bool ConvertFromLua(TVisObjRef &object, int index) {
	return GetObjectFromLua(object, index);
}

// Confirmed (asm lines 1400028-1400046)
void ConvertToLua(const TVisObjRef &object) {
	CreateVisionaireObject(L, object);
}

// Confirmed (asm lines 1400046-1400215): the objects are in the table from 1 up to the first nil; an object is
// a data object, or a string that names one (or gives its id).
bool ConvertFromLua(TVList &objects, int index) {
	for (int i = 1;; i++) {
		lua_pushinteger(L, i);
		lua_gettable(L, index);

		int type = lua_type(L, -1);

		if (type == LUA_TNIL) {
			lua_settop(L, -2);
			return true;
		}

		TVisObjRef object;

		if (type == LUA_TUSERDATA) {
			if (!GetObjectFromLua(object, -1)) {
				lua_settop(L, -2);
				return false;
			}
		} else if (type == LUA_TSTRING) {
			if (!FindObjectByNameOrId(utf(lua_tolstring(L, -1, nullptr)), object, true)) {
				lua_settop(L, -2);
				return false;
			}
		} else {
			lua_settop(L, -2);
			return false;
		}

		objects.push_back(object);
		lua_settop(L, -2);
	}
}

// Confirmed (asm lines 1400215-1400276): a table of the objects, numbered from 1, that has the metatable of the
// tables of the game (which is what finds an object of it by its name).
void ConvertToLua(const TVList &objects) {
	lua_createtable(L, static_cast<int>(objects.size()), 0);
	lua_getfield(L, LUA_REGISTRYINDEX, "Visionaire.TVisionaireTable");
	lua_setmetatable(L, -2);

	int number = 1;

	for (TVisionaireObject *object : objects) {
		lua_pushinteger(L, number++);
		CreateVisionaireObject(L, object);
		lua_settable(L, -3);
	}
}

// Not reconstructed yet (the commands, and the particles): nothing is put into Lua.
void CmdVisObjTo(lua_State *) {
}

int luaopen_Particles(lua_State *) {
	return 0;
}

#include "vscommon/scripting/visLua.h"

#include <string>
#include <unordered_set>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "common/lua/lauxlib.h"
#include "common/lua/lualib.h"
#include "datastruct/visionaire.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/lua.h"
#include "vscommon/scripting/luaConversion.h"
#include "vscommon/scripting/visLuaObjects.h"
#include "vstables/visionaireGame.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vscommon/scripting/visLua.cpp";

lua_State *L = nullptr;
TVisionaireGame *luaGame = nullptr;

// The names of the global variables that InitLua() made (the hash table VisionaireGlobalVars).
static std::unordered_set<std::wstring> s_internalGlobalVars;

static wxString utf(const char *text) {
	wxString result;

	toUTF(&result, text ? text : "");
	return result;
}

// The point that means "the line between two parts" in the scripts.
static const int kDividingPoint = -10000;

// Confirmed (asm lines 1405988-1406026)
void ExportEmptyObject() {
	ConvertToLua(luaGame->GetEmptyObject());
	lua_setfield(L, LUA_GLOBALSINDEX, "emptyObject");
}

// Confirmed (asm lines 1406026-1406064): (the name of the global is the string at 0xD40420 of the
// binary, which has no label of its own: it is "game", what the scripts call it)
void ExportGameObject() {
	ConvertToLua(luaGame->GetGame());
	lua_setfield(L, LUA_GLOBALSINDEX, "game");
}

// Confirmed (asm lines 1406064-1406092)
void ExportDividingPoint() {
	wxPoint point;

	point.x = kDividingPoint;
	point.y = kDividingPoint;
	ConvertToLua(point);
	lua_setfield(L, LUA_GLOBALSINDEX, "DIVIDING_POINT");
}

// Confirmed (asm lines 1406092-1406207)
void ExportDirs(const wxString &appDir, const wxString &resourcesDir, const wxString & /*unused*/) {
	if (!appDir.IsEmpty()) {
		lua_pushstring(L, appDir.mb_str());
		lua_setfield(L, LUA_GLOBALSINDEX, "localAppDir");
	}

	if (!resourcesDir.IsEmpty()) {
		lua_pushstring(L, resourcesDir.mb_str());
		lua_setfield(L, LUA_GLOBALSINDEX, "localResourcesDir");
	}
}

// Confirmed (asm lines 1406207-1406409): the names of the globals that exist when the scripts have not
// run yet.
void StoreNamesOfInternalGlobalVars() {
	lua_pushnil(L);

	while (lua_next(L, LUA_GLOBALSINDEX)) {
		wxString name = utf(lua_tolstring(L, -2, nullptr));

		s_internalGlobalVars.insert(name.ToStdWstring());
		lua_settop(L, -2);
	}
}

// Confirmed (asm lines 1406643-1406740)
bool IsInternalGlobalVar(const wxString &name) {
	return s_internalGlobalVars.count(name.ToStdWstring()) != 0;
}

// Confirmed (asm lines 1406409-1406643)
void InitLua(TVisionaireGame *game, const wxString &appDir, const wxString &resourcesDir, const wxString &unused) {
	L = luaL_newstate();
	luaL_openlibs(L);

	lua_pushcclosure(L, LuaPrint, 0);
	lua_setfield(L, LUA_GLOBALSINDEX, "print");

	// (the libraries of the script editor - socket, mime, mobdebug - are not reconstructed)
	luaGame = game;

	luaopen_VisionaireObject(L);
	luaopen_Sprite(L);
	InitCommonCommands();

	for (int field = 0x65; field != 0x348; field++)
		SetField(field);

	SetTables();
	SetEnums();
	ExportEmptyObject();
	ExportGameObject();
	ExportDividingPoint();
	ExportDirs(appDir, resourcesDir, unused);

	lua_pushcclosure(L, lua_debugerror, 0);
	lua_setfield(L, LUA_GLOBALSINDEX, "debugerror");

	// the error handler of the scripts: it reports the error through debugerror()
	if (luaL_loadstring(L, "function debugfunc(err) debugerror(err) end") == 0)
		lua_pcall(L, 0, 0, 0);

	luaopen_Particles(L);
	StoreNamesOfInternalGlobalVars();
}

// Confirmed (asm lines 1406740-1406949): print() as Lua's own (the arguments through tostring(),
// tab between them), but the line goes to the log (when it is on at level 2) and not to the console.
int LuaPrint(lua_State *state) {
	int count = lua_gettop(state);
	std::string line;

	lua_getfield(state, LUA_GLOBALSINDEX, "tostring");

	for (int i = 1; i <= count; i++) {
		lua_pushvalue(state, -1);
		lua_pushvalue(state, i);
		lua_call(state, 1, 1);

		const char *text = lua_tolstring(state, -1, nullptr);

		if (!text)
			return luaL_error(state, "'tostring' must return a string to 'print'");

		if (i > 1)
			line += "\t";

		line += text;
		lua_settop(state, -2);
	}

	if (wxLog::loglevel > 1)
		wxLog::logexpanded(L"%s", utf(line.c_str()).wc_str());

	return 0;
}

static void logScriptError(const char *message, const std::string &code) {
	if (wxLog::loglevel < 0)
		return;

	wxLog::logexpanded(L"Failed to run string in Lua: %s", utf(message).wc_str());
	wxLog::logexpanded(L"String content: %s", utf(code.c_str()).wc_str());
}

// Confirmed (asm lines 1406949-1407229): in the player a script runs under the error handler
// `debugfunc`, which is what makes the error (and where it is) known.
void LuaDoString(const std::string &code) {
	if (TVisionaire::IsVisPlayerMode)
		lua_getfield(L, LUA_GLOBALSINDEX, "debugfunc");

	int top = lua_gettop(L);

	debugger.BeginArea(ProfileArea::kValue2, std::string(), -1);

	int status = luaL_loadstring(L, code.c_str());

	if (status == 0)
		status = lua_pcall(L, 0, LUA_MULTRET, TVisionaire::IsVisPlayerMode ? lua_gettop(L) - 1 : 0);

	if (status != 0) {
		logScriptError(lua_tolstring(L, -1, nullptr), code);
		lua_settop(L, -2);
	}

	if (TVisionaire::IsVisPlayerMode)
		lua_remove(L, top);

	profileClean();
	debugger.EndArea(ProfileArea::kValue2, -1);
}

// Confirmed (asm lines 1408406-1409018)
void LuaDoString(const std::string &code, const std::string &chunkName) {
	g_loadingState = L"Lua Script";

	if (TVisionaire::IsVisPlayerMode && !chunkName.empty()) {
		debugger.BeginArea(ProfileArea::kValue2, chunkName, -1);
		lua_getfield(L, LUA_GLOBALSINDEX, "debugfunc");

		if (luaL_loadbuffer(L, code.data(), code.size(), chunkName.c_str()) != 0) {
			logScriptError(lua_tolstring(L, -1, nullptr), code);
			lua_settop(L, -2);
			profileClean();
			lua_settop(L, -2);
			debugger.EndArea(ProfileArea::kValue2, -1);
			g_loadingState = L"Loading";
			return;
		}

		int status = lua_pcall(L, 0, 0, lua_gettop(L) - 1);

		lua_remove(L, -2);

		if (status != 0) {
			logScriptError(lua_tolstring(L, -1, nullptr), code);
			lua_settop(L, -2);
		}

		debugger.EndArea(ProfileArea::kValue2, -1);
	} else if (luaL_loadstring(L, code.c_str()) != 0 || lua_pcall(L, 0, LUA_MULTRET, 0) != 0) {
		logScriptError(lua_tolstring(L, -1, nullptr), code);
		lua_settop(L, -2);
	}

	g_loadingState = L"Loading";
	profileClean();
}

static void logResultError(const char *format, const std::string &name, int number) {
	if (wxLog::loglevel < 0)
		return;

	wxLog::logexpanded(utf(format).wc_str(), utf(name.c_str()).wc_str(), number);
}

// The message of a result that is not of the type the caller wants: with several results it says
// which one (counted from 1), else not.
static void logWrongResult(const char *withNumber, const char *alone, const std::string &name, size_t results,
                           int number) {
	if (results >= 2)
		logResultError(withNumber, name, number);
	else
		logResultError(alone, name, number);
}

// Confirmed (asm lines 1407229-1408406): calls the global function `name` under the error handler,
// with the arguments as Lua values, and reads its return values into `results`, one result
// after the other from the top of the stack (so the first result gets the last value the function
// returned - the original does that). Every result is checked against its type.
bool LuaExecuteFunction(const std::string &name, std::vector<TArgument *> &arguments,
                        std::vector<TArgument *> &results) {
	debugger.BeginArea(ProfileArea::kValue2, name, -1);
	lua_getfield(L, LUA_GLOBALSINDEX, "debugfunc");

	int handler = lua_gettop(L);

	lua_getfield(L, LUA_GLOBALSINDEX, name.c_str());

	for (TArgument *argument : arguments) {
		switch (argument->GetType()) {
		case TArgType::kBool:
			lua_pushboolean(L, argument->GetBool());
			break;
		case TArgType::kInt:
			lua_pushinteger(L, argument->GetInt());
			break;
		case TArgType::kFloat:
			lua_pushnumber(L, argument->GetFloat());
			break;
		case TArgType::kPoint:
			ConvertToLua(argument->GetPoint());
			break;
		case TArgType::kRect:
			ConvertToLua(argument->GetRect());
			break;
		case TArgType::kString:
			lua_pushstring(L, argument->GetString().mb_str());
			break;
		case TArgType::kPath:
			lua_pushstring(L, argument->GetPath().mb_str());
			break;
		case TArgType::kObject:
			ConvertToLua(argument->GetObject());
			break;
		case TArgType::kStringList:
			ConvertToLua(argument->GetStringList());
			break;
		case TArgType::kIntList:
			ConvertToLua(argument->GetIntList());
			break;
		case TArgType::kFloatList:
			ConvertToLua(argument->GetFloatList());
			break;
		default:
			x_assert(false, "false", kSourceFile, 0x180);
			return false;
		}
	}

	if (lua_pcall(L, (int)arguments.size(), (int)results.size(), handler) != 0) {
		if (wxLog::loglevel >= 0) {
			wxLog::logexpanded(L"Failed to execute hook function '%s': %s", utf(name.c_str()).wc_str(),
			                   utf(lua_tolstring(L, -1, nullptr)).wc_str());
		}

		lua_settop(L, -2);
		lua_remove(L, handler);
		profileClean();
		debugger.EndArea(ProfileArea::kValue2, -1);
		return false;
	}

	profileClean();

	bool ok = true;
	int number = 1;

	for (TArgument *result : results) {
		switch (result->GetType()) {
		case TArgType::kBool:
			if (lua_type(L, -1) == LUA_TBOOLEAN) {
				result->Set(lua_toboolean(L, -1) != 0);
			} else {
				logWrongResult("Function '%s' must return a boolean value as %d. return value",
				               "Function '%s' must return a boolean value", name, results.size(), number);
				ok = false;
			}

			break;
		case TArgType::kInt:
			if (lua_isnumber(L, -1)) {
				result->Set((int)lua_tointeger(L, -1));
			} else {
				logWrongResult("Function '%s' must return a number value as %d. return value",
				               "Function '%s' must return a number value", name, results.size(), number);
				ok = false;
			}

			break;
		case TArgType::kFloat:
			if (lua_isnumber(L, -1)) {
				result->Set((double)lua_tonumber(L, -1));
			} else {
				logWrongResult("Function '%s' must return a number value as %d. return value",
				               "Function '%s' must return a number value", name, results.size(), number);
				ok = false;
			}

			break;
		case TArgType::kPoint: {
			wxPoint point;

			if (lua_type(L, -1) == LUA_TTABLE && ConvertFromLua(point, lua_gettop(L))) {
				result->Set(point);
			} else {
				logWrongResult("Function '%s' must return a table (with x and y) as %d. return value",
				               "Function '%s' must return a table (with x and y)", name, results.size(), number);
				ok = false;
			}

			break;
		}
		case TArgType::kRect: {
			wxRect rect;

			if (lua_type(L, -1) == LUA_TTABLE && ConvertFromLua(rect, lua_gettop(L))) {
				result->Set(rect);
			} else {
				logWrongResult("Function '%s' must return a table (with x, y, width and height) as %d. return value",
				               "Function '%s' must return a table (with x, y, width and height)", name,
				               results.size(), number);
				ok = false;
			}

			break;
		}
		case TArgType::kString:
		case TArgType::kPath:
			if (lua_isstring(L, -1)) {
				wxString text = utf(lua_tolstring(L, -1, nullptr));

				if (result->GetType() == TArgType::kString)
					result->Set(text);
				else
					result->SetPath(text);
			} else {
				logWrongResult("Function '%s' must return a string value as %d. return value",
				               "Function '%s' must return a string value", name, results.size(), number);
				ok = false;
			}

			break;
		case TArgType::kObject: {
			// (the original looks at the first stack slot here, not the top: a result that is an
			// object is not found unless the stack is just that)
			LuaVisionaireObject *object = CheckVisionaireObject(L, 1, true);

			if (object) {
				result->Set(object->object);
			} else {
				logWrongResult("Function '%s' must return a visionaire object as %d. return value",
				               "Function '%s' must return a visionaire object", name, results.size(), number);
			}

			break;
		}
		default:
			x_assert(false, "false", kSourceFile, 0x215);
			ok = false;
			return false;
		}

		lua_settop(L, -2);
		number++;
	}

	x_assert(ok, "ret", kSourceFile, 0x219);
	LuaDebugName("");
	lua_remove(L, handler);
	debugger.EndArea(ProfileArea::kValue2, -1);
	return ok;
}

// Confirmed (asm lines 1409018-1409566)
bool LuaExecuteEventHandler(const std::string &handler, const TVisObjRef &object) {
	TArgument argument;
	std::vector<TArgument *> arguments;
	std::vector<TArgument *> results;

	argument.Set(object);
	arguments.push_back(&argument);
	return LuaExecuteFunction(handler, arguments, results);
}

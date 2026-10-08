// Confirmed (Deponia_Linux.asm lines 431833-434031, "src/vsplayer/scripting/scriptingutils.cpp"): the global
// variables of the Lua scripts in the savegames. A save makes a `ScriptVariable` record (table 0x22) for every
// global variable of the scripts and, below it, for the elements of its table; a load puts the records of the
// global variables back into the Lua state. Not saved: the globals of the engine itself (IsInternalGlobalVar), the
// functions (unless the table has the field `_savefunctions_`; never the global ones), the tables that have the field
// `_temporary_`, userdata that is not a data object, and keys that are not a number or a string. A table below the
// 10th level is not saved either.
#include <cstring>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "WxStub.h"
#include "base64.h"
#include "common/lua/lauxlib.h"
#include "datastruct/visionaireobject.h"
#include "datastruct/visobjref.h"
#include "datastruct/vlist.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/visLuaObjects.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vsplayer/scripting/scriptingutils.cpp";

/** The table of the ScriptVariable records. */
static const int kScriptVariableTable = 0x22;

/** Confirmed (asm `writer(lua_State *, void const *, unsigned long, void *)`): collects the bytes lua_dump gives. */
static int writer(lua_State *, const void *data, size_t size, void *buffer) {
	luaL_addlstring(static_cast<luaL_Buffer *>(buffer), static_cast<const char *>(data), size);
	return 0;
}

// Confirmed (asm lines 431833-432793): saves the table at `index` of the stack as the children of `parent` (the
// global variables, with an empty parent, when it is the table of the globals).
static void traverseTable(int index, TVisObjRef &parent, TVisionaireGame &game) {
	const TVisObjRef none;
	const bool isGlobalTable = (index == LUA_GLOBALSINDEX);

	lua_pushnil(L);

	while (lua_next(L, index) != 0) {
		int keyType = lua_type(L, -2);

		if (keyType != LUA_TNUMBER && keyType != LUA_TSTRING) {
			lua_settop(L, -2);
			continue;
		}

		wxString key;

		if (keyType == LUA_TSTRING)
			toUTF(&key, lua_tolstring(L, -2, nullptr));
		else
			key = CONVTOSTR(static_cast<long>(lua_tonumber(L, -2)));

		if (isGlobalTable && IsInternalGlobalVar(key)) {
			lua_settop(L, -2);
			continue;
		}

		int valueType = lua_type(L, -1);
		bool isNumberOrString = (valueType == LUA_TNUMBER || valueType == LUA_TSTRING);
		bool isBoolean = (valueType == LUA_TBOOLEAN);
		bool isFunction = (valueType == LUA_TFUNCTION);
		bool skip = false;

		if (!isNumberOrString && !isBoolean && valueType != LUA_TTABLE && !isFunction && valueType != LUA_TUSERDATA) {
			lua_settop(L, -2);
			continue;
		}

		if (isFunction) {
			if (isGlobalTable) {
				skip = true;
			} else {
				lua_pushstring(L, "_savefunctions_");
				lua_rawget(L, -4);
				skip = (lua_type(L, -1) == LUA_TNIL);
				lua_settop(L, -2);
			}
		} else if (valueType == LUA_TTABLE) {
			lua_pushstring(L, "_temporary_");
			lua_rawget(L, -2);
			skip = (lua_type(L, -1) != LUA_TNIL);
			lua_settop(L, -2);
		} else if (valueType == LUA_TUSERDATA) {
			// Only the data objects are saved. (The original takes one value too many off the stack for a userdata
			// that has no metatable at all; here such a value is just skipped.)
			skip = true;

			if (lua_getmetatable(L, -1)) {
				lua_getfield(L, LUA_REGISTRYINDEX, "Visionaire.TVisionaireObject");
				skip = !lua_rawequal(L, -1, -2);
				lua_settop(L, -2);
				lua_settop(L, -2);
			}
		}

		if (skip) {
			lua_settop(L, -2);
			continue;
		}

		TVisObjRef variable;

		if (parent.IsEmpty())
			variable = game.CreateActiveObject(kScriptVariableTable, none);
		else
			variable = game.CreateObject(kScriptVariableTable, parent, kScriptVariableItems);

		variable.SetName(TCharHolder(key));
		variable.SetValue(kScriptVariableKeyType, keyType, TSendEventEnum::kSendEvent);
		variable.SetValue(kScriptVariableValueType, valueType, TSendEventEnum::kSendEvent);
		variable.SetValue(kScriptVariableIsGlobalVar, isGlobalTable, TSendEventEnum::kSendEvent);

		if (isNumberOrString) {
			wxString value;

			toUTF(&value, lua_tolstring(L, -1, nullptr));
			variable.SetValue(kScriptVariableValue, value, TSendEventEnum::kSendEvent);
		} else if (isBoolean) {
			variable.SetValue(kScriptVariableValue, wxString(lua_toboolean(L, -1) ? L"true" : L"false"),
			                  TSendEventEnum::kSendEvent);
		} else if (valueType == LUA_TUSERDATA) {
			// (the metatable of the data objects was found by the check above)
			if (lua_getmetatable(L, -1)) {
				lua_getfield(L, LUA_REGISTRYINDEX, "Visionaire.TVisionaireObject");

				if (lua_rawequal(L, -1, -2)) {
					LuaVisionaireObject *object = static_cast<LuaVisionaireObject *>(lua_touserdata(L, -3));
					wxString value;

					toUTF(&value, IdStrStd(object->object->GetId()).c_str());
					variable.SetValue(kScriptVariableValue, value, TSendEventEnum::kSendEvent);
				}

				lua_settop(L, -3);
			}
		} else if (isFunction) {
			// The bytecode of the function, in base64. (The original passes the pointer of the text to SetValue, which
			// takes it for the boolean overload: it never stores the text. The text is what it means to store.)
			luaL_Buffer buffer;

			luaL_buffinit(L, &buffer);

			if (lua_dump(L, writer, &buffer) != 0 && wxLog::loglevel >= 0)
				wxLog::logexpanded(L"unable to dump given function");

			luaL_pushresult(&buffer);

			size_t length = 0;
			const char *code = lua_tolstring(L, -1, &length);
			std::string text = base64_encode(reinterpret_cast<const unsigned char *>(code), static_cast<unsigned int>(length));

			lua_settop(L, -2);

			wxString value;

			toUTF(&value, text.c_str());
			variable.SetValue(kScriptVariableValue, value, TSendEventEnum::kSendEvent);
		} else {
			// A table: its elements are the children of the record, up to 10 levels down.
			int top = lua_gettop(L);

			if (top <= 0x13) {
				traverseTable(top, variable, game);
			} else {
				// The name of the whole table, the one at the top.
				wxString name(variable.GetName());
				TVisObjRef ancestor = variable.GetParent();

				while (!ancestor.IsEmpty()) {
					name = wxString(ancestor.GetName());
					ancestor = ancestor.GetParent();
				}

				if (wxLog::loglevel > 0) {
					wxLog::logexpanded(L"The whole lua table '%s' was not saved because there are more than 10 nested "
					                   L"levels. Either fix possible cyclic reference or use '_temporary_' field to avoid "
					                   L"table being stored in savegames.", name.wc_str());
				}
			}
		}

		lua_settop(L, -2);
	}
}

// Confirmed (asm lines 432803-432848)
void SaveGlobalScriptVariables(TVisionaireGame &game) {
	lua_settop(L, 0);

	int stackSizeBefore = lua_gettop(L);
	TVisObjRef none;

	traverseTable(LUA_GLOBALSINDEX, none, game);

	x_assert(stackSizeBefore == lua_gettop(L), "stackSizeBefore == stackSizeAfter", kSourceFile, 0xB3);
}

/**
 * Confirmed (asm lines 432858-433387 and 433397-434031, which has the same loop for the global variables): puts the
 * records of the list into the table at `index`, a value for each key. It pushes the value and then the key in the
 * original, and calls lua_settable, which takes the key as the value just below the top: it would set
 * `table[value] = key`. That cannot be what the game relies on (saved variables are restored by it), so the key is
 * pushed first here. TODO: check this against the behaviour of the game if a saved variable ever comes back wrong.
 */
static void loadList(const TVList &list, int index, TVisionaireGame &game) {
	(void)game;

	for (TVisionaireObject *object : list) {
		int keyType = object->GetInt(kScriptVariableKeyType);
		int valueType = object->GetInt(kScriptVariableValueType);
		wxString value = object->GetStr(kScriptVariableValue);

		// The key. (What the original does with any other kind is not defined, the record is skipped.)
		if (keyType == LUA_TNUMBER) {
			double number = 0.0;

			if (!object->GetName().ToDouble(&number)) {
				x_assert(false, "false", kSourceFile, 0xFA);
				continue;
			}

			lua_pushnumber(L, number);
		} else if (keyType == LUA_TSTRING) {
			lua_pushstring(L, object->GetName().mb_str());
		} else {
			continue;
		}

		switch (valueType) {
		case LUA_TBOOLEAN:
			lua_pushboolean(L, value.ToStdWstring() == L"true");
			break;
		case LUA_TNUMBER: {
			double number = 0.0;

			if (!value.ToDouble(&number) && wxLog::loglevel >= 0) {
				wxLog::logexpanded(L"lua variable %s might not be parsed correctly %f", wxString(object->GetName().mb_str()).wc_str(),
				                   number);
			}

			lua_pushnumber(L, number);
			break;
		}
		case LUA_TSTRING:
			lua_pushstring(L, value.mb_str());
			break;
		case LUA_TTABLE: {
			lua_createtable(L, 0, 0);

			int table = lua_gettop(L);
			TVList items;

			object->GetLinks(kScriptVariableItems, TypeOrder::kValue0, items);
			loadList(items, table, game);
			break;
		}
		case LUA_TFUNCTION: {
			std::string code = base64_decode(std::string(value.mb_str()));

			luaL_loadbuffer(L, code.data(), code.size(), "");
			break;
		}
		case LUA_TUSERDATA: {
			TVisObjRef reference;

			FindObjectByNameOrId(value, reference, true);
			CreateVisionaireObject(L, reference);
			break;
		}
		default:
			// (nil and light userdata are never saved)
			lua_settop(L, -2);
			continue;
		}

		lua_settable(L, index);
	}
}

// Confirmed (asm lines 433397-434031)
void LoadGlobalScriptVariables(TVisionaireGame &game) {
	int stackSizeBefore = lua_gettop(L);
	TVList all;
	TVList globals;

	game.GetList(kScriptVariableTable, all, false);

	for (TVisionaireObject *object : all) {
		if (object->GetBool(kScriptVariableIsGlobalVar))
			globals.push_back(TVisObjRef(*object));
	}

	if (!globals.empty())
		loadList(globals, LUA_GLOBALSINDEX, game);

	x_assert(stackSizeBefore == lua_gettop(L), "stackSizeBefore == stackSizeAfter", kSourceFile, 0x11D);
}

// Confirmed (Deponia_Linux.asm lines 1419217-1428982, src/vscommon/scripting/visionaireobjectLua.cpp by the
// asserts): how the scripts see the data of the game.
//
// A data object (TVisionaireObject) is a userdata (LuaVisionaireObject, holding a counted reference to it)
// with the metatable "Visionaire.TVisionaireObject". There is one userdata for each object, which the
// registry keeps (the object holds the number of its entry, TVisionaireObject::GetLuaObject()), so that the
// same object is always the same Lua value. `obj.Name` reads a field and `obj.Name = "x"` writes one (the
// fields are named by their XML name, see getFieldFromString()); the methods (`obj:getInt(VName)` ...) are the
// functions of visionaireobject_m. Each table of the game (`Scenes`, `Characters` ...) is a global, a table
// {tableId = n} with the metatable "Visionaire.TVisionaireTable" whose __index finds the object of that
// name (or number); the tables of linked objects that getLinks() makes have the same metatable (they have no
// tableId), and for those the names are looked up in an index made together with the table.
#include "vscommon/scripting/visLuaObjects.h"

#include <cstring>
#include <unordered_map>
#include <utility>
#include <vector>

#include "Diagnostics.h"
#include "TXMLNames.h"
#include "common/lua/lauxlib.h"
#include "datastruct/table.h"
#include "datastruct/typegrp.h"
#include "datastruct/visionaire.h"
#include "datastruct/visionaireobject.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/lua.h"
#include "vscommon/scripting/luaConversion.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vscommon/scripting/visionaireobjectLua.cpp";
static const char *const kObjectMetatable = "Visionaire.TVisionaireObject";
static const char *const kTableMetatable = "Visionaire.TVisionaireTable";
static const char *const kObjectExpected = "'VisionaireObject' expected";

// ---------------------------------------------------------------------------------------------------------
// The caches (the original's hash maps LinksCache, RefStringIndexCache and lua_functions)

/** The key of an id in the caches. */
static unsigned long idKey(const TId &id) {
	return (static_cast<unsigned long>(id.getId()) & 0xFFFFFF) | (static_cast<unsigned long>(id.getTable()) << 24);
}

static TId idOf(const TVisionaireObject *object) {
	const std::uint8_t *id = object->GetId();

	return TId(id[0] + (id[1] << 8) + (static_cast<signed char>(id[2]) << 16), id[3]);
}

/** For each object (by id), the registry entries of the tables that getLinks() made for its link fields
 *  (field, entry), so that the same table is given while the field does not change. */
static std::unordered_map<unsigned long, std::vector<std::pair<int, int> > > s_linksCache;
/** For each of those tables (by its address) the position of the first object of each name, which finds
 *  an object by its name. */
static std::unordered_map<const void *, std::unordered_map<std::string, int> > s_refStringIndexCache;
/** The functions of the methods, by name. */
static std::unordered_map<std::string, lua_CFunction> s_luaFunctions;

// Confirmed (asm lines 1422668-1422765): the registry entry of the table for that field, or 0.
static int CheckLuaFieldsCache(const TId &id, int field) {
	auto found = s_linksCache.find(idKey(id));

	if (found == s_linksCache.end())
		return 0;

	for (const std::pair<int, int> &entry : found->second) {
		if (entry.first == field)
			return entry.second;
	}

	return 0;
}

// Confirmed (asm lines 1424710-1424906)
static void AddLuaFieldsCache(const TId &id, int field, int reference) {
	s_linksCache[idKey(id)].push_back(std::make_pair(field, reference));
}

/** Gives up the table of a cached entry: the entry of the registry and the index of its names. (The
 *  original lets the entries of the registry be when it clears all of the cache of an object, and the
 *  index of the names when it clears all of the cache.) */
static void dropCachedTable(int reference) {
	lua_rawgeti(L, LUA_REGISTRYINDEX, reference);
	s_refStringIndexCache.erase(lua_topointer(L, -1));
	lua_settop(L, -2);
	luaL_unref(L, LUA_REGISTRYINDEX, reference);
}

// Confirmed (asm lines 1422765-1423337): forgets the tables that were made for a field of the object (or for all of
// its fields, when `field` is -1); with the id (-1,-1) of nothing, for all of the objects.
void UnrefLuaFieldsCache(const TId &id, int field) {
	if (!L) {
		s_linksCache.clear();
		s_refStringIndexCache.clear();
		return;
	}

	if (id.getId() == -1 && id.getTable() == 0xFF) {
		for (const auto &cached : s_linksCache) {
			for (const std::pair<int, int> &entry : cached.second)
				dropCachedTable(entry.second);
		}

		s_linksCache.clear();
		return;
	}

	auto found = s_linksCache.find(idKey(id));

	if (found == s_linksCache.end())
		return;

	std::vector<std::pair<int, int> > &entries = found->second;

	if (field == -1) {
		for (const std::pair<int, int> &entry : entries)
			dropCachedTable(entry.second);

		s_linksCache.erase(found);
		return;
	}

	for (size_t i = 0; i < entries.size(); i++) {
		if (entries[i].first == field) {
			dropCachedTable(entries[i].second);
			entries.erase(entries.begin() + i);
			return;
		}
	}
}

// Confirmed (asm lines 1424434-1424710): the userdata of an object that goes away is let go, and the tables made
// for its fields are forgotten.
void LuaObjectUnref(const TId &id, int handle) {
	if (!L)
		return;

	luaL_unref(L, LUA_REGISTRYINDEX, handle);
	UnrefLuaFieldsCache(id, -1);
}

/** Forgets all that the caches know (the Lua state is gone). */
void ClearLuaObjectCaches() {
	s_linksCache.clear();
	s_refStringIndexCache.clear();
	s_luaFunctions.clear();
}

// ---------------------------------------------------------------------------------------------------------
// Making and checking the userdata

static wxString utf(const char *text) {
	wxString result;

	toUTF(&result, text ? text : "");
	return result;
}

// Confirmed (asm lines 1422150-1422210): the data object of the userdata at `index`; it is an error when
// there is not one. The userdata is taken from the stack when `remove` says so.
LuaVisionaireObject *CheckVisionaireObject(lua_State *state, int index, bool remove) {
	LuaVisionaireObject *object = static_cast<LuaVisionaireObject *>(luaL_checkudata(state, index, kObjectMetatable));

	if (!object)
		luaL_argerror(state, index, kObjectExpected);

	if (remove)
		lua_remove(state, index);

	return object;
}

// Confirmed (asm lines 1422210-1422245)
bool GetObjectFromLua(TVisObjRef &object, int index) {
	LuaVisionaireObject *userdata = static_cast<LuaVisionaireObject *>(luaL_checkudata(L, index, kObjectMetatable));

	if (!userdata)
		return false;

	object.Set(userdata->object);
	return true;
}

// Confirmed (asm lines 1422245-1422290)
int luaL_optboolean(lua_State *state, int index, int defaultValue) {
	if (lua_type(state, index) == LUA_TBOOLEAN)
		return lua_toboolean(state, index);

	return defaultValue;
}

// Confirmed (asm lines 1421701-1421833): pushes the userdata of the object (an object that is not there is the
// empty object of the game); a new one is made, and kept in the registry, only the first time.
void CreateVisionaireObject(lua_State *state, TVisionaireObject *object) {
	if (!object) {
		TVisObjRef empty = GetLuaGame()->GetEmptyObject();

		object = empty.GetObjectPointer();

		if (!object) {
			LuaVisionaireObject *userdata = static_cast<LuaVisionaireObject *>(lua_newuserdata(state, sizeof(LuaVisionaireObject)));

			userdata->object = empty.GetReference();
			lua_getfield(state, LUA_REGISTRYINDEX, kObjectMetatable);
			lua_setmetatable(state, -2);
			return;
		}
	}

	if (object->GetLuaObject() != 0) {
		lua_rawgeti(state, LUA_REGISTRYINDEX, object->GetLuaObject());
		return;
	}

	LuaVisionaireObject *userdata = static_cast<LuaVisionaireObject *>(lua_newuserdata(state, sizeof(LuaVisionaireObject)));

	userdata->object = object->GetReference();
	lua_getfield(state, LUA_REGISTRYINDEX, kObjectMetatable);
	lua_setmetatable(state, -2);

	int reference = luaL_ref(state, LUA_REGISTRYINDEX);

	object->SetLuaObject(reference);
	lua_rawgeti(state, LUA_REGISTRYINDEX, reference);
}

// Confirmed (asm lines 1421833-1421855)
void CreateVisionaireObject(lua_State *state, const TVisObjRef &object) {
	CreateVisionaireObject(state, object.GetObjectPointer());
}

// Confirmed (asm lines 1419217-1419247): obj.new() makes a userdata without an object.
static int new_T(lua_State *state) {
	lua_remove(state, 1);

	LuaVisionaireObject *userdata = static_cast<LuaVisionaireObject *>(lua_newuserdata(state, sizeof(LuaVisionaireObject)));

	userdata->object = nullptr;
	lua_getfield(state, LUA_REGISTRYINDEX, kObjectMetatable);
	lua_setmetatable(state, -2);
	return 1;
}

// Confirmed (asm lines 1419247-1419275): a table of the game made by a script is a plain table.
static int new_TT(lua_State *state) {
	lua_remove(state, 1);
	lua_createtable(state, 0, 0);
	lua_getfield(state, LUA_REGISTRYINDEX, kTableMetatable);
	lua_setmetatable(state, -2);
	return 1;
}

// ---------------------------------------------------------------------------------------------------------
// The methods that read a field

/** The start of most of the methods: the object is checked and taken off the stack, and the field (a
 *  number) is the first argument. Null when it is not a number (the method then gives nothing). */
static TVisionaireObject *objectAndField(lua_State *state, int &field) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, true);

	if (lua_type(state, 1) != LUA_TNUMBER)
		return nullptr;

	field = static_cast<int>(lua_tointeger(state, 1));
	return self->object;
}

// Confirmed (asm lines 1419290-1419357)
static int GetBool(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	lua_pushboolean(state, object->GetBool(field));
	return 1;
}

// Confirmed (asm lines 1419357-1419424)
static int GetStr(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	lua_pushstring(state, object->GetStrHolder(field).mb_str());
	return 1;
}

// Confirmed (asm lines 1419424-1419491)
static int GetSprite(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	CreateTSprite(state, object->GetSprite(field));
	return 1;
}

// Confirmed (asm lines 1419491-1419555)
static int GetRect(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	ConvertToLua(*object->GetRect(field));
	return 1;
}

// Confirmed (asm lines 1419555-1419624)
static int GetTexts(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	std::vector<TTextLanguage> *texts = nullptr;

	object->GetTexts(field, &texts);
	ConvertToLua(*texts);
	return 1;
}

// Confirmed (asm lines 1419624-1419825): getTextStr(field [, language]) is the text of a text object (the field
// links to it) in the language, which is the standard language of the game when it is not given; "" when
// there is no such text.
static int GetTextStr(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	int language;

	if (lua_type(state, 2) == LUA_TNUMBER) {
		language = static_cast<int>(lua_tointeger(state, 2));
	} else {
		TVisObjRef standard = GetLuaGame()->GetGame().GetLink(kGameStandardLanguage);
		const std::uint8_t *id = standard.GetId();

		language = id[0] + (id[1] << 8) + (static_cast<signed char>(id[2]) << 16);
	}

	std::vector<TTextLanguage> *texts = nullptr;
	TVisionaireObject *linked = object->GetLink(field);

	if (linked)
		linked->GetTexts(kTextTextLanguages, &texts);

	if (texts) {
		for (TTextLanguage &text : *texts) {
			if (text.languageId == language) {
				lua_pushstring(state, text.text.mb_str());
				return 1;
			}
		}
	}

	lua_pushstring(state, "");
	return 1;
}

// Confirmed (asm lines 1419825-1419889)
static int GetPoint(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	ConvertToLua(*object->GetPoint(field));
	return 1;
}

// Confirmed (asm lines 1419889-1419969)
static int GetName(lua_State *state) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, false);

	lua_pushstring(state, self->object->GetName().mb_str());
	return 1;
}

// Confirmed (asm lines 1419969-1420030)
static int ClearLink(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (object)
		object->ClearLink(field, true);

	return 0;
}

// Confirmed (asm lines 1420030-1420097)
static int GetInt(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	lua_pushinteger(state, object->GetInt(field));
	return 1;
}

// Confirmed (asm lines 1420097-1420165)
static int GetFloat(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	lua_pushnumber(state, object->GetFloat(field));
	return 1;
}

// Confirmed (asm lines 1420165-1420222)
static int IsEmpty(lua_State *state) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, true);

	lua_pushboolean(state, self->object ? self->object->IsEmpty() : true);
	return 1;
}

// Confirmed (asm lines 1420222-1420278)
static int IsAnyObject(lua_State *state) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, true);

	lua_pushboolean(state, self->object ? self->object->IsAnyObject() : false);
	return 1;
}

// Confirmed (asm lines 1420278-1420363): getId() is {tableId = , id = }.
static int GetId(lua_State *state) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, false);
	const std::uint8_t *id = self->object->GetId();

	lua_createtable(state, 0, 0);
	lua_pushstring(state, "tableId");
	lua_pushinteger(state, static_cast<signed char>(id[3]));
	lua_settable(state, -3);
	lua_pushstring(state, "id");
	lua_pushinteger(state, id[0] + (id[1] << 8) + (static_cast<signed char>(id[2]) << 16));
	lua_settable(state, -3);
	return 1;
}

// Confirmed (asm lines 1420363-1420415): __gc
static int DeleteVisionaireObject(lua_State *state) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, false);

	if (self->object) {
		self->object->Release();
		self->object = nullptr;
	}

	return 0;
}

// Confirmed (asm lines 1420415-1420506): __eq: the objects of the same id.
static int EqVisionaireObject(lua_State *state) {
	LuaVisionaireObject *first = CheckVisionaireObject(state, 1, false);
	LuaVisionaireObject *second = CheckVisionaireObject(state, 2, false);

	if (first->object && second->object)
		lua_pushboolean(state, std::memcmp(first->object->GetId(), second->object->GetId(), 4) == 0);
	else
		lua_pushboolean(state, false);

	return 1;
}

// Confirmed (asm lines 1420506-1420608)
static int GetInts(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	std::vector<int> values;

	object->GetInts(field, values);
	ConvertToLua(values);
	return 1;
}

// Confirmed (asm lines 1420608-1420710)
static int GetFloats(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	std::vector<float> values;

	object->GetFloats(field, values);
	ConvertToLua(values);
	return 1;
}

// Confirmed (asm lines 1420710-1420844)
static int GetPaths(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	std::vector<TCharHolder> values;

	object->GetPaths(field, values);
	ConvertToLua(values);
	return 1;
}

// Confirmed (asm lines 1420844-1420965)
static int SetName(lua_State *state) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, true);

	if (lua_type(state, 1) != LUA_TSTRING)
		return 0;

	self->object->SetName(TCharHolder(utf(lua_tolstring(state, 1, nullptr))));
	return 0;
}

// Confirmed (asm lines 1420965-1421263): tostring(object) is `Name (table,id)`.
static int PrintVisionaireObject(lua_State *state) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, true);
	wxString text;

	if (self->object && !self->object->IsEmpty()) {
		const std::uint8_t *id = self->object->GetId();

		text = wxString(self->object->GetName().c_str().ToStdWstring() + L" (" +
		                std::to_wstring(static_cast<signed char>(id[3])) + L"," +
		                std::to_wstring(id[0] + (id[1] << 8) + (static_cast<signed char>(id[2]) << 16)) + L")");
	} else {
		text = wxString(L"---Empty---");
	}

	lua_pushstring(state, text.mb_str());
	return 1;
}

// Confirmed (asm lines 1421855-1421922)
static int GetLink(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	CreateVisionaireObject(state, object->GetLink(field));
	return 1;
}

// Confirmed (asm lines 1421922-1421989)
static int GetParent(lua_State *state) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, true);
	TVisObjRef parent(self->object->GetParent());

	CreateVisionaireObject(state, parent.GetObjectPointer());
	return 1;
}

// Confirmed (asm lines 1421989-1422150): getObject("path" [, warn]) is the object that the path (the
// fields to follow, see objAccess.h) leads to from this one; the warnings are on unless `false` is given.
static int GetObject(lua_State *state) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, true);

	if (lua_type(state, 1) != LUA_TSTRING)
		return 0;

	bool warn = true;

	if (lua_type(state, 2) == LUA_TBOOLEAN)
		warn = lua_toboolean(state, 2) != 0;

	TVisObjRef object(self->object);

	FindObjectByNameRelative(utf(lua_tolstring(state, 1, nullptr)), object, warn);
	CreateVisionaireObject(state, object.GetObjectPointer());
	return 1;
}

// Confirmed (asm lines 1422290-1422468): the path of a field
static int GetObjectPath(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	lua_pushstring(state, object->GetPath(field).GetFullPath().mb_str());
	return 1;
}

// Confirmed (asm lines 1423337-1423435)
static int GetPoints(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	std::vector<wxPoint> values;

	object->GetPoints(field, values);
	ConvertToLua(values);
	return 1;
}

// Confirmed (asm lines 1423435-1423533)
static int GetRects(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	std::vector<wxRect> values;

	object->GetRects(field, values);
	ConvertToLua(values);
	return 1;
}

// Confirmed (asm lines 1423533-1423650)
static int GetSprites(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	std::vector<TSprite> values;

	object->GetSprites(field, values);
	ConvertToLua(values);
	return 1;
}

// Confirmed (asm lines 1424906-1425524): getLinks(field) is a table of the objects that the field links to.
// The table is made once and given again until the field changes (UnrefLuaFieldsCache()); with it an
// index of the names is made for the objects to be found by them.
static int GetLinks(lua_State *state) {
	int field;
	TVisionaireObject *object = objectAndField(state, field);

	if (!object)
		return 0;

	TId id = idOf(object);
	int reference = CheckLuaFieldsCache(id, field);

	if (reference != 0) {
		lua_rawgeti(state, LUA_REGISTRYINDEX, reference);
		return 1;
	}

	TVList list;

	object->GetLinks(field, TypeOrder::kValue0, list);
	ConvertToLua(list);
	reference = luaL_ref(state, LUA_REGISTRYINDEX);
	AddLuaFieldsCache(id, field, reference);
	lua_rawgeti(state, LUA_REGISTRYINDEX, reference);

	// the position of the first object of each name (the Lua tables count from 1)
	std::unordered_map<std::string, int> &names = s_refStringIndexCache[lua_topointer(state, -1)];

	names.clear();

	for (int i = static_cast<int>(list.size()) - 1; i >= 0; i--)
		names[list.at(i)->GetName().mb_str()] = i + 1;

	return 1;
}

// ---------------------------------------------------------------------------------------------------------
// The tables of the game

// Confirmed (asm lines 1423650-1424152): the __index of the tables of the game and of the lists of objects.
static int GetTableIndex(lua_State *state) {
	int keyType = lua_type(state, 2);

	if (keyType != LUA_TNUMBER && keyType != LUA_TSTRING)
		return 0;

	const char *key = lua_tolstring(state, 2, nullptr);

	lua_pushstring(state, "tableId");
	lua_rawget(state, 1);

	if (lua_type(state, -1) != LUA_TNIL) {
		// a table of the game: the object at the number, or of the name
		int tableId = static_cast<int>(lua_tointeger(state, -1));

		lua_settop(state, -2);

		TTable *table = nullptr;

		if (!luaGame->GetTable(tableId, &table))
			return 0;

		if (keyType == LUA_TNUMBER) {
			int index = static_cast<int>(lua_tointeger(state, 2));

			if (index > 0 && static_cast<unsigned long>(index) <= table->GetCount()) {
				TVisionaireObject *object = nullptr;

				if (!table->GetObjectAtPosition(&object, index - 1))
					return 0;

				CreateVisionaireObject(state, object);
				return 1;
			}

			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Index out of bounds %d", index);

			return 0;
		}

		if (std::strcmp(key, "__len") == 0) {
			TVList list;

			table->GetList(list, true);
			lua_pushinteger(state, static_cast<lua_Integer>(list.size()));
			return 1;
		}

		TVisObjRef found;

		table->GetByName(utf(key), found);
		CreateVisionaireObject(state, found.GetObjectPointer());
		return 1;
	}

	// a list of objects: a number that is not in it is nothing, a name is looked up
	if (keyType == LUA_TNUMBER) {
		lua_rawget(state, 1);
		return 1;
	}

	lua_settop(state, -2);
	lua_remove(state, 2);

	auto index = s_refStringIndexCache.find(lua_topointer(state, 1));

	if (index != s_refStringIndexCache.end()) {
		auto position = index->second.find(key);

		if (position != index->second.end()) {
			lua_pushinteger(state, position->second);
			lua_rawget(state, 1);
			return 1;
		}
	}

	lua_pushnil(state);

	while (lua_next(state, 1)) {
		LuaVisionaireObject *entry = static_cast<LuaVisionaireObject *>(luaL_checkudata(state, -1, kObjectMetatable));

		if (!entry)
			luaL_argerror(state, -1, kObjectExpected);

		if (entry->object->GetName() == key)
			return 1;

		lua_settop(state, -2);
	}

	if (wxLog::loglevel >= 0)
		wxLog::logexpanded(L"Visobj %s could not be found in linklist", utf(key).wc_str());

	return 0;
}

// Confirmed (asm lines 1421263-1421405): the globals `Characters`, `Scenes` ... (the plural names of the tables)
// are the tables of the game.
int luaopen_ExportTables(lua_State *state) {
	x_assert(lua_gettop(state) == 0, "lua_gettop(L) == 0", kSourceFile, 0x80);

	lua_createtable(state, 0, 0);

	int methods = lua_gettop(state);

	luaL_newmetatable(state, kTableMetatable);

	int metatable = lua_gettop(state);

	lua_pushlstring(state, "__metatable", 11);
	lua_pushvalue(state, methods);
	lua_settable(state, metatable);

	lua_pushstring(state, "__index");
	lua_pushcclosure(state, GetTableIndex, 0);
	lua_settable(state, -3);

	lua_createtable(state, 0, 0);

	int methodsMeta = lua_gettop(state);

	lua_pushlstring(state, "__call", 6);
	lua_pushcclosure(state, new_TT, 0);
	lua_settable(state, methods);

	lua_pushlstring(state, "new", 3);
	lua_pushvalue(state, -2);
	lua_settable(state, methodsMeta);
	lua_setmetatable(state, methods);
	lua_settop(state, -3);

	for (int table = 0; table != 0x27; table++) {
		lua_createtable(state, 0, 0);
		lua_pushstring(state, "tableId");
		lua_pushinteger(state, table);
		lua_settable(state, -3);
		lua_getfield(state, LUA_REGISTRYINDEX, kTableMetatable);
		lua_setmetatable(state, -2);
		lua_setfield(state, LUA_GLOBALSINDEX, GetLuaGame()->GetTableNamePlural(table, true).mb_str());
	}

	x_assert(lua_gettop(state) == 0, "lua_gettop(L) == 0", kSourceFile, 0xAA);
	return 0;
}

// ---------------------------------------------------------------------------------------------------------
// The fields by their names

// Confirmed (asm lines 1422468-1422668): the id of the field that `name` says. The name of a field may have the
// `V` of the constants before it, and may have the name of the table before it (`SceneName`); the
// fields of the active records (ActiveTexts ...) are those of the record they stand for.
static int getFieldFromString(LuaVisionaireObject *self, const char *name) {
	const std::uint8_t *id = self->object->GetId();
	int table = static_cast<signed char>(id[3]);
	TTypeGroup *group = GetTypeGroup(table);

	if (name[0] == 'V' && name[1] > '@' && name[1] <= 'Z')
		name++;

	std::string singular(self->object->GetVisionaire()->GetTableNameSingular(table, true).mb_str());
	std::string withTable = singular + name;
	int field = TXMLNames::GetNrByUtf8Name(withTable.c_str());

	if (field == -1 || group->GetType(field, false) == static_cast<eTypeData>(-1)) {
		field = TXMLNames::GetNrByUtf8Name(name);

		if (field != -1 && group->GetType(field, false) != static_cast<eTypeData>(-1))
			return field;
	} else {
		return field;
	}

	// no field of that name: for an active record the one of the record it stands for
	int base = -1;

	switch (static_cast<std::uint8_t>(id[3])) {
	case 0x19:
		base = 7;
		break;
	case 0x1A:
		base = 9;
		break;
	case 0x18:
		base = 0xE;
		break;
	default:
		break;
	}

	// (for the other tables the original adds the name to the name it has made before, which is the name of
	// the table and the name already: it tries the name twice)
	if (base != -1)
		withTable = std::string(self->object->GetVisionaire()->GetTableNameSingular(base, true).mb_str());

	if (!withTable.empty()) {
		withTable += name;
		field = TXMLNames::GetNrByUtf8Name(withTable.c_str());
	}

	x_assert(field != -1, "fieldID != -1", kSourceFile, 0x3FB);
	return field;
}

/** The method that gets a field of the type, called with the object and the field on the stack. */
static int callGetter(lua_State *state, eTypeData type) {
	switch (type) {
	case eTypeData::kBool:
		return GetBool(state);
	case eTypeData::kInt:
		return GetInt(state);
	case eTypeData::kString:
		return GetStr(state);
	case eTypeData::kPath:
		return GetObjectPath(state);
	case eTypeData::kFloat:
		return GetFloat(state);
	case eTypeData::kRectList:
		return GetRects(state);
	case eTypeData::kSpriteList:
		return GetSprites(state);
	case eTypeData::kPointList:
		return GetPoints(state);
	case eTypeData::kIntList:
		return GetInts(state);
	case eTypeData::kPathList:
		return GetPaths(state);
	case eTypeData::kFloatList:
		return GetFloats(state);
	case eTypeData::kPoint:
		return GetPoint(state);
	case eTypeData::kRect:
		return GetRect(state);
	case eTypeData::kSprite:
		return GetSprite(state);
	case eTypeData::kLink:
		return GetLink(state);
	case eTypeData::kLinkList:
		return GetLinks(state);
	case eTypeData::kTextList:
		return GetTexts(state);
	default:
		// (the string list and the unused type have no getter: the original goes on with what is on the stack)
		return 1;
	}
}

// Confirmed (asm lines 1426327-1426906): the __index of the objects. A name of a method gives the method; the
// special names `id`, `tableId`, `name` and `parent` give those; any other is a field.
static int GetIndex(lua_State *state) {
	if (lua_type(state, 2) == LUA_TSTRING) {
		auto method = s_luaFunctions.find(lua_tolstring(state, 2, nullptr));

		if (method != s_luaFunctions.end()) {
			lua_pushcclosure(state, method->second, 0);
			return 1;
		}
	}

	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, true);

	if (lua_type(state, 1) != LUA_TSTRING)
		return 0;

	if (self->object->IsEmpty()) {
		lua_pushstring(state, "object is empty");
		return lua_error(state);
	}

	const char *name = lua_tolstring(state, 1, nullptr);
	const std::uint8_t *id = self->object->GetId();

	if (std::strcmp(name, "id") == 0) {
		lua_pushinteger(state, id[0] + (id[1] << 8) + (static_cast<signed char>(id[2]) << 16));
		return 1;
	}

	if (std::strcmp(name, "tableId") == 0) {
		lua_pushinteger(state, static_cast<signed char>(id[3]));
		return 1;
	}

	if (std::strcmp(name, "name") == 0) {
		lua_pushstring(state, self->object->GetName().mb_str());
		return 1;
	}

	if (std::strcmp(name, "parent") == 0) {
		TVisObjRef parent(self->object->GetParent());

		CreateVisionaireObject(state, parent.GetObjectPointer());
		return 1;
	}

	TTypeGroup *group = GetTypeGroup(static_cast<signed char>(id[3]));
	int field = getFieldFromString(self, name);
	TTypeData *typeData = group->GetTypeDataPtr(field);

	if (!typeData) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Unknown data-field \"%s\" for object %s %s.", utf(name).wc_str(),
			                   IdStr(*self->object).wc_str(), self->object->GetName().c_str().wc_str());

		lua_debugerror(state);
		return 0;
	}

	lua_settop(state, -2);
	CreateVisionaireObject(state, self->object);
	lua_pushinteger(state, field);
	return callGetter(state, typeData->GetType());
}

// ---------------------------------------------------------------------------------------------------------
// The methods that write

// Confirmed (asm lines 1426906-1427187): setTextStr(field, "text" [, language]) sets the text of the text object that
// the field links to, in the language (the standard one when it is not given), adding the language when the
// text has none.
static int SetTextStr(lua_State *state) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, true);

	if (lua_type(state, 1) != LUA_TNUMBER || lua_type(state, 2) != LUA_TSTRING)
		return 0;

	int field = static_cast<int>(lua_tointeger(state, 1));
	const char *text = lua_tolstring(state, 2, nullptr);
	int language;

	if (lua_type(state, 3) == LUA_TNUMBER) {
		language = static_cast<int>(lua_tointeger(state, 3));
	} else {
		TVisObjRef standard = GetLuaGame()->GetGame().GetLink(kGameStandardLanguage);
		const std::uint8_t *id = standard.GetId();

		language = id[0] + (id[1] << 8) + (static_cast<signed char>(id[2]) << 16);
	}

	std::vector<TTextLanguage> *texts = nullptr;
	TVisionaireObject *linked = self->object->GetLink(field);

	if (linked)
		linked->GetTexts(kTextTextLanguages, &texts);

	if (!texts)
		return 0;

	for (TTextLanguage &entry : *texts) {
		if (entry.languageId == language) {
			entry.text = TCharHolder(text);
			return 0;
		}
	}

	TTextLanguage added;

	added.languageId = language;
	added.text = TCharHolder(text);
	texts->push_back(added);
	return 0;
}

/** The error when a value could not be set. */
static int setValueFailed(lua_State *state, TVisionaireObject *object) {
	std::string name(object->GetName().mb_str());
	std::string table(luaGame->GetVisTableName(static_cast<signed char>(object->GetId()[3]), true).mb_str());
	const std::uint8_t *id = object->GetId();

	return luaL_error(state, "call to setValue on object %s (%s,%d) failed\n", name.c_str(), table.c_str(),
	                  id[0] + (id[1] << 8) + (static_cast<signed char>(id[2]) << 16));
}

// Confirmed (asm lines 1427187-1428042): setValue(field, value) sets a field to a value; what is given
// has to be of the type of the field (or a text for a path and a link, which are then looked up).
static int SetValue(lua_State *state) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, true);
	TVisionaireObject *object = self->object;

	if (lua_type(state, 1) != LUA_TNUMBER) {
		const std::uint8_t *id = object->GetId();

		return luaL_error(state, "call to setValue on object %s (%s,%d) failed, first argument is not a number.\n",
		                  object->GetName().mb_str(), std::string(luaGame->GetVisTableName(static_cast<signed char>(id[3]), true).mb_str()).c_str(),
		                  id[0] + (id[1] << 8) + (static_cast<signed char>(id[2]) << 16));
	}

	int field = static_cast<int>(lua_tointeger(state, 1));
	bool done = false;

	switch (lua_type(state, 2)) {
	case LUA_TBOOLEAN:
		done = object->SetValue(field, lua_toboolean(state, 2) != 0, TSendEventEnum::kSendEvent);
		break;
	case LUA_TNUMBER:
		switch (object->GetTypeField(field)) {
		case static_cast<int>(eTypeData::kInt):
			done = object->SetValue(field, static_cast<int>(lua_tointeger(state, 2)), TSendEventEnum::kSendEvent);
			break;
		case static_cast<int>(eTypeData::kBool):
			done = object->SetValue(field, lua_toboolean(state, 2) != 0, TSendEventEnum::kSendEvent);
			break;
		case static_cast<int>(eTypeData::kFloat):
			done = object->SetValue(field, static_cast<float>(lua_tonumber(state, 2)), TSendEventEnum::kSendEvent);
			break;
		default:
			break;
		}
		break;
	case LUA_TSTRING:
		switch (object->GetTypeField(field)) {
		case static_cast<int>(eTypeData::kString):
			done = object->SetValue(field, utf(lua_tolstring(state, 2, nullptr)), TSendEventEnum::kSendEvent);
			break;
		case static_cast<int>(eTypeData::kPath): {
			wxFileName path(utf(lua_tolstring(state, 2, nullptr)).ToStdWstring());

			path.NormalizePath();
			done = object->SetValue(field, path, TSendEventEnum::kSendEvent);
			break;
		}
		case static_cast<int>(eTypeData::kLink): {
			TVisObjRef found;

			if (FindObjectByNameOrId(utf(lua_tolstring(state, 2, nullptr)), found, true)) {
				const std::uint8_t *linkId = found.GetId();

				done = object->SetLink(field, TId(linkId[0] + (linkId[1] << 8) + (static_cast<signed char>(linkId[2]) << 16), linkId[3]), true);
			}
			break;
		}
		default:
			break;
		}
		break;
	case LUA_TTABLE:
		switch (object->GetTypeField(field)) {
		case static_cast<int>(eTypeData::kPoint): {
			wxPoint value;

			ConvertFromLua(value, 2);
			done = object->SetValue(field, value, TSendEventEnum::kSendEvent);
			break;
		}
		case static_cast<int>(eTypeData::kRect): {
			wxRect value;

			ConvertFromLua(value, 2);
			done = object->SetValue(field, value, TSendEventEnum::kSendEvent);
			break;
		}
		case static_cast<int>(eTypeData::kSprite): {
			TSprite value;

			ConvertFromLua(value, 2);
			done = object->SetValue(field, value, TSendEventEnum::kSendEvent);
			break;
		}
		case static_cast<int>(eTypeData::kPointList): {
			std::vector<wxPoint> value;

			ConvertFromLua(value, 2);
			done = object->SetValue(field, value, TSendEventEnum::kSendEvent);
			break;
		}
		case static_cast<int>(eTypeData::kRectList): {
			std::vector<wxRect> value;

			ConvertFromLua(value, 2);
			done = object->SetValue(field, value, TSendEventEnum::kSendEvent);
			break;
		}
		case static_cast<int>(eTypeData::kSpriteList): {
			std::vector<TSprite> value;

			ConvertFromLua(value, 2);
			done = object->SetValue(field, value, TSendEventEnum::kSendEvent);
			break;
		}
		case static_cast<int>(eTypeData::kLinkList): {
			TVList value;

			if (ConvertFromLua(value, 2))
				done = object->SetValue(field, value, true);
			break;
		}
		case static_cast<int>(eTypeData::kTextList): {
			std::vector<TTextLanguage> value;

			ConvertFromLua(value, 2);
			done = object->SetValue(field, value, TSendEventEnum::kSendEvent);
			break;
		}
		default:
			break;
		}
		break;
	case LUA_TUSERDATA:
		if (object->GetTypeField(field) == static_cast<int>(eTypeData::kLink)) {
			LuaVisionaireObject *linked = static_cast<LuaVisionaireObject *>(luaL_checkudata(state, 2, kObjectMetatable));

			if (!linked)
				luaL_argerror(state, 2, kObjectExpected);

			lua_remove(state, 2);
			done = object->SetLink(field, idOf(linked->object), true);
		}
		break;
	default:
		// (nothing, a light userdata and a function are not set)
		return 0;
	}

	if (!done)
		return setValueFailed(state, object);

	return 0;
}

// Confirmed (asm lines 1428042-1428271): __newindex. The object, the field and the value are put on the
// stack the way setValue() takes them.
static int SetIndex(lua_State *state) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, true);

	if (lua_type(state, 1) != LUA_TSTRING)
		return 0;

	const char *name = lua_tolstring(state, 1, nullptr);
	TTypeGroup *group = GetTypeGroup(static_cast<signed char>(self->object->GetId()[3]));
	int field = getFieldFromString(self, name);

	if (!group->GetTypeDataPtr(field)) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Unknown data-field \"%s\" for object %s %s.", utf(name).wc_str(),
			                   IdStr(*self->object).wc_str(), self->object->GetName().c_str().wc_str());

		lua_debugerror(state);
		return 0;
	}

	lua_remove(state, 1);
	lua_pushinteger(state, field);
	lua_insert(state, 1);
	CreateVisionaireObject(state, self->object);
	lua_insert(state, 1);
	return SetValue(state);
}

// ---------------------------------------------------------------------------------------------------------
// Debugging

// Confirmed (asm lines 1424152-1424434): logs what is on the stack, one line for each.
void stackDump(lua_State *state) {
	int top = lua_gettop(state);

	for (int i = 1; i <= top; i++) {
		int type = lua_type(state, i);

		if (wxLog::loglevel <= 1)
			continue;

		switch (type) {
		case LUA_TSTRING:
			wxLog::logexpanded(L"%d '%s'", i, utf(lua_tolstring(state, i, nullptr)).wc_str());
			break;
		case LUA_TBOOLEAN:
			wxLog::logexpanded(L"%d %s", i, utf(lua_toboolean(state, i) ? "true" : "false").wc_str());
			break;
		case LUA_TNUMBER:
			wxLog::logexpanded(L"%d %g", i, lua_tonumber(state, i));
			break;
		case LUA_TTABLE:
			wxLog::logexpanded(L"%d %s", i, utf("table").wc_str());
			break;
		default:
			wxLog::logexpanded(L"%d %s", i, utf(lua_typename(state, type)).wc_str());
			break;
		}
	}
}

// Confirmed (asm lines 1428271-1428982): debug([thread,] message [, level]) is the stack traceback of Lua's
// debug library (db_errorfb) that is logged as well as given. (The original looks up the text of the chunk in a map
// to add the line that failed, but nothing ever puts a chunk there.)
static int DoStack(lua_State *state) {
	const int kFirstLevels = 12;
	const int kLastLevels = 10;
	lua_State *thread = state;
	int argument = 0;

	if (lua_type(state, 1) == LUA_TTHREAD) {
		thread = lua_tothread(state, 1);
		argument = 1;
	}

	int level;
	bool firstPart = true;
	lua_Debug info;

	if (lua_isnumber(state, argument + 2)) {
		level = static_cast<int>(lua_tointeger(state, argument + 2));
		lua_settop(state, -2);
	} else {
		level = (state == thread) ? 1 : 0;
	}

	if (lua_gettop(state) == argument)
		lua_pushlstring(state, "", 0);
	else if (!lua_isstring(state, argument + 1))
		return 1;
	else
		lua_pushlstring(state, "\n", 1);

	lua_pushlstring(state, "stack traceback:", 16);

	while (lua_getstack(thread, level++, &info)) {
		if (level > kFirstLevels && firstPart) {
			if (!lua_getstack(thread, level + kLastLevels, &info)) {
				level--;
			} else {
				lua_pushlstring(state, "\n\t...", 5);

				while (lua_getstack(thread, level + kLastLevels, &info))
					level++;
			}

			firstPart = false;
			continue;
		}

		lua_pushlstring(state, "\n\t", 2);
		lua_getinfo(thread, "Snl", &info);
		lua_pushfstring(state, "%s:", info.short_src);

		if (info.currentline > 0)
			lua_pushfstring(state, "%d:", info.currentline);

		if (*info.namewhat != '\0')
			lua_pushfstring(state, " in function '%s'", info.name);
		else if (*info.what == 'm')
			lua_pushfstring(state, " in main chunk");
		else if (*info.what == 'C' || *info.what == 't')
			lua_pushlstring(state, " ?", 2);
		else
			lua_pushfstring(state, " in function <%s:%d>", info.short_src, info.linedefined);

		lua_concat(state, lua_gettop(state) - argument);
	}

	lua_concat(state, lua_gettop(state) - argument);

	const char *traceback = lua_tolstring(state, -1, nullptr);

	if (wxLog::loglevel > 1)
		wxLog::logexpanded(L"%s", utf(traceback).wc_str());

	return 1;
}

// Confirmed (asm lines 1425524-1426327): info() is a table that describes the object: its type (`_Type`, the
// name of its table), `_Name`, `_Id` and `_Parent`, and then the value of each field of its type, under
// the name of the field.
static int VisObjInfo(lua_State *state) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, false);

	lua_remove(state, 1);

	TVisionaireObject *object = self->object;
	int table = static_cast<signed char>(object->GetId()[3]);
	TTypeGroup *group = GetTypeGroup(table);
	wxString typeName = object->GetVisionaire()->GetTableNameSingular(table, true);

	lua_createtable(state, 0, 0);
	lua_pushstring(state, "_Type");
	lua_pushstring(state, typeName.mb_str());
	lua_settable(state, -3);

	lua_pushstring(state, "_Name");
	CreateVisionaireObject(state, object);
	lua_insert(state, 1);
	GetName(state);
	lua_remove(state, 1);
	lua_settable(state, -3);

	lua_pushstring(state, "_Id");
	CreateVisionaireObject(state, object);
	lua_insert(state, 1);
	GetId(state);
	lua_settable(state, -3);

	lua_pushstring(state, "_Parent");
	CreateVisionaireObject(state, object);
	lua_insert(state, 1);
	GetParent(state);
	lua_settable(state, -3);

	for (TTypeData *typeData : group->GetTypes()) {
		lua_pushstring(state, TXMLNames::GetStringUtf8(typeData->GetDescription()));
		CreateVisionaireObject(state, object);
		lua_insert(state, 1);
		lua_pushinteger(state, typeData->GetDescription());
		lua_insert(state, 2);
		callGetter(state, typeData->GetType());
		lua_remove(state, 1);
		lua_settable(state, -3);
	}

	return 1;
}

// Confirmed (asm lines 1419275-1419290): `to` is the command VisObjTo.
static int VisObjTo(lua_State *state) {
	CmdVisObjTo(state);
	return 0;
}

namespace {

struct LuaMethod {
	const char *name;
	lua_CFunction function;
};

// Confirmed (asm: visionaireobject_m)
const LuaMethod kObjectMethods[] = {
	{"isEmpty", IsEmpty},
	{"isAnyObject", IsAnyObject},
	{"getId", GetId},
	{"getParent", GetParent},
	{"getObject", GetObject},
	{"getName", GetName},
	{"setName", SetName},
	{"setValue", SetValue},
	{"clearLink", ClearLink},
	{"setTextStr", SetTextStr},
	{"getInt", GetInt},
	{"getFloat", GetFloat},
	{"getBool", GetBool},
	{"getStr", GetStr},
	{"getPath", GetObjectPath},
	{"getSprite", GetSprite},
	{"getRect", GetRect},
	{"getPoint", GetPoint},
	{"getLink", GetLink},
	{"getLinks", GetLinks},
	{"debug", DoStack},
	{"getPoints", GetPoints},
	{"getRects", GetRects},
	{"getSprites", GetSprites},
	{"getPaths", GetPaths},
	{"getInts", GetInts},
	{"getFloats", GetFloats},
	{"getTexts", GetTexts},
	{"getTextStr", GetTextStr},
	{"to", VisObjTo},
	{"info", VisObjInfo},
};

} // End of anonymous namespace

// Confirmed (asm lines 1421405-1421701): the registration is that of the "Lunar" class template again (see
// luaSprite.cpp), but the methods are not put in the table of the methods: __index finds them in
// s_luaFunctions.
int luaopen_VisionaireObject(lua_State *state) {
	lua_createtable(state, 0, 0);

	int methods = lua_gettop(state);

	luaL_newmetatable(state, kObjectMetatable);

	int metatable = lua_gettop(state);

	lua_pushlstring(state, "__metatable", 11);
	lua_pushvalue(state, methods);
	lua_settable(state, metatable);

	lua_pushlstring(state, "__tostring", 10);
	lua_pushcclosure(state, PrintVisionaireObject, 0);
	lua_settable(state, metatable);

	lua_pushstring(state, "__index");
	lua_pushcclosure(state, GetIndex, 0);
	lua_settable(state, -3);

	lua_pushstring(state, "__newindex");
	lua_pushcclosure(state, SetIndex, 0);
	lua_settable(state, -3);

	lua_pushlstring(state, "__gc", 4);
	lua_pushcclosure(state, DeleteVisionaireObject, 0);
	lua_settable(state, -3);

	lua_pushlstring(state, "__eq", 4);
	lua_pushcclosure(state, EqVisionaireObject, 0);
	lua_settable(state, -3);

	lua_createtable(state, 0, 0);

	int methodsMeta = lua_gettop(state);

	lua_pushlstring(state, "__call", 6);
	lua_pushcclosure(state, new_T, 0);
	lua_settable(state, methods);

	lua_pushlstring(state, "new", 3);
	lua_pushvalue(state, -2);
	lua_settable(state, methodsMeta);
	lua_setmetatable(state, methods);

	for (const LuaMethod &method : kObjectMethods)
		s_luaFunctions[method.name] = method.function;

	lua_settop(state, -3);
	return luaopen_ExportTables(state);
}

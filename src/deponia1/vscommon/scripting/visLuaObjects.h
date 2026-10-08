// Confirmed (Deponia_Linux.asm lines 1417000-1428300, vscommon/scripting/visionaireobjectLua.cpp): how the
// data objects are seen by the scripts: a userdata (LuaVisionaireObject) with a metatable whose
// functions read and write the fields (GetInt, SetValue, GetLink ...). Also the setup that makes
// the constants the scripts use (the id of every field, the number of every table, the enums), the
// sprite and particle objects, and the debug error function.
//
// Not reconstructed yet: the functions of these objects themselves (the stubs in lua.cpp).
#pragma once

#include "common/lua/lua.h"
#include "datastruct/visobjref.h"

class TVisionaireObject;

/** What the userdata of a data object holds (the object is referenced by the game's tables). */
struct LuaVisionaireObject {
	TVisionaireObject *object;
};

LuaVisionaireObject *CheckVisionaireObject(lua_State *state, int index, bool quiet);

int luaopen_VisionaireObject(lua_State *state);
int luaopen_Sprite(lua_State *state);
int luaopen_Particles(lua_State *state);
int lua_debugerror(lua_State *state);

/** Makes a global for the field with this id (named after the XML name of the field). */
void SetField(int field);
/** Makes the global of every field. */
void SetFields();
/** Makes a global for each table of the game's data. */
void SetTables();
/** Makes the globals for the enums of the scripts (luaGlobals.cpp). */
void SetEnums();
/** Puts the commands the scripts call into the Lua state. */
void InitCommonCommands();

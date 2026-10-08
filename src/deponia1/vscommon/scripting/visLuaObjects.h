// Confirmed (Deponia_Linux.asm lines 1417000-1428300, vscommon/scripting/visionaireobjectLua.cpp): how the
// data objects are seen by the scripts: a userdata (LuaVisionaireObject) with a metatable whose
// functions read and write the fields (GetInt, SetValue, GetLink ...). Also the setup that makes
// the constants the scripts use (the id of every field, the number of every table, the enums), the
// sprite and particle objects, and the debug error function.
//
// (visionaireobjectLua.cpp, luaSprite.cpp, luaGlobals.cpp)
#pragma once

#include "vscommon/scripting/visLua.h"
#include "datastruct/visobjref.h"

class TSprite;
class TVisionaireObject;
class TVisObjRef;

/** What the userdata of a data object holds (the object is referenced by the game's tables). */
struct LuaVisionaireObject {
	TVisionaireObject *object;
};

/** The data object of the userdata at `index` (it is an error when it is not one); the userdata is taken
 *  off the stack when `remove` says so. */
LuaVisionaireObject *CheckVisionaireObject(lua_State *state, int index, bool remove);
/** Pushes the userdata of a data object (the empty object when there is none): the same one each time. */
void CreateVisionaireObject(lua_State *state, TVisionaireObject *object);
void CreateVisionaireObject(lua_State *state, const TVisObjRef &object);
/** Sets `object` to the data object of the userdata at `index` (of the Lua state of the game). */
bool GetObjectFromLua(TVisObjRef &object, int index);
/** The boolean at `index`, or `defaultValue` when it is not one. */
int luaL_optboolean(lua_State *state, int index, int defaultValue);
/** Logs the values on the stack. */
void stackDump(lua_State *state);
/** Forgets the tables that were made for the link fields of the objects (the Lua state goes). */
void ClearLuaObjectCaches();
/** The tables of the game as globals (`Scenes` ...). */
int luaopen_ExportTables(lua_State *state);
/** The command behind the object method `to` (not reconstructed yet). */
void CmdVisObjTo(lua_State *state);

int luaopen_VisionaireObject(lua_State *state);
/** Makes the sprite class of the scripts (luaSprite.cpp) and the function createSprite(). */
int luaopen_Sprite(lua_State *state);
/** The sprite of the userdata at `index` (it is an error when it is not a sprite). */
TSprite **CheckSprite(lua_State *state, int index);
/** createSprite("path"): pushes a new sprite. */
int CreateSprite(lua_State *state);
/** Pushes a new sprite for the scripts that is a copy of `sprite`. */
void CreateTSprite(lua_State *state, const TSprite &sprite);
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

// Confirmed (Deponia_Linux.asm lines 1405988-1409566, vscommon/scripting/visLua.cpp): the game's Lua. One
// Lua state (`L`, ScummVM's Lua 5.1) holds the scripts of the game; InitLua() makes it and puts into it
// what the scripts see: the libraries, the data objects (luaopen_VisionaireObject), the commands the
// scripts call (InitCommonCommands), a constant for every field and table of the data (SetField(),
// SetTables(), SetEnums()), the `game` and `emptyObject` objects and `DIVIDING_POINT`. Everything
// that runs a script goes through LuaDoString() (a piece of text) or LuaExecuteFunction() (a function
// by its name, with arguments and results as TArguments, the way the hooks of the engine call the
// scripts); both log what goes wrong and go on.
//
// Not reconstructed: the libraries for the network and the debugger that InitLua() of the original
// puts in `package.preload` (socket, mime, mobdebug: they serve the script editor of the game's
// developers), and the profiler (profileClean() does nothing).
#pragma once

#include <string>
#include <vector>

#include "WxStub.h"
// (ScummVM keeps the engines from using the C library's functions; this code uses the C++ library)
#ifndef FORBIDDEN_SYMBOL_ALLOW_ALL
	#define FORBIDDEN_SYMBOL_ALLOW_ALL
#endif
#include "common/lua/lua.h"
#include "datastruct/visobjref.h"
#include "vscommon/scripting/argument.h"

class TVisionaireGame;

/** The Lua state of the game. */
extern lua_State *L;
/** The game whose data the scripts work on. */
extern TVisionaireGame *luaGame;

/** Makes the Lua state for `game`; `appDir` and `resourcesDir` (when they are not empty) are the
 *  scripts' `localAppDir` and `localResourcesDir`. */
void InitLua(TVisionaireGame *game, const wxString &appDir, const wxString &resourcesDir, const wxString &unused);
/** The exports of the game's objects to Lua (also done again when the game is replaced). */
void ExportEmptyObject();
void ExportGameObject();
void ExportDividingPoint();
void ExportDirs(const wxString &appDir, const wxString &resourcesDir, const wxString &unused);
void StoreNamesOfInternalGlobalVars();
/** The script function `print`. */
int LuaPrint(lua_State *state);
/** Whether `name` is a global variable that InitLua() made (the others are the scripts' own: only
 *  those are saved with the game). */
bool IsInternalGlobalVar(const wxString &name);

/** Closes the Lua state (lua.cpp). */
void CloseLua();
/** Runs a Lua file; what goes wrong is logged. */
void LuaDoFile(const wxFileName &file);
void LuaDoFile(const char *path);
/** Runs the function that the registry holds under `ref` (see LuaDoString()). */
void LuaDoRef(int ref);

/** Runs a piece of Lua. `chunkName` is how errors name it. What the code returns stays on the stack (except for a
 *  chunk with a name in the player's mode). */
void LuaDoString(const std::string &code);
void LuaDoString(const std::string &code, const std::string &chunkName);
/** Calls the function `name` of the scripts with `arguments`; its return values go to `results`
 *  (they have their types already). False if the call fails or a result is not of its type. */
bool LuaExecuteFunction(const std::string &name, std::vector<TArgument *> &arguments,
                        std::vector<TArgument *> &results);
/** Calls the function `handler` of the scripts with a data object. */
bool LuaExecuteEventHandler(const std::string &handler, const TVisObjRef &object);

/** Adds the time since the last call to the profile of the Lua function that ran. */
void profileClean();

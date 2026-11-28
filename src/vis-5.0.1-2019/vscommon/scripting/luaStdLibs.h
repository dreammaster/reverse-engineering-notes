// The libraries of the standard distribution of Lua that the scripts of the game use and the core of Lua of ScummVM (common/lua)
// leaves out: `io`, `os`, `package` with `require`, and LuaFileSystem (`lfs`, the original has it built in; the scripts get it
// with require("lfs")). The original runs LuaJIT 2.1.0-beta3, which has them all; the scripts of Deponia 1 use io.open/close,
// os.time/date and require("lfs") (THIRD_PARTY.md). Written for this reconstruction on the C and C++ library: inside ScummVM the
// files are Common::File and these are replaced by what ScummVM's own Lua has.
#pragma once

struct lua_State;

/** Makes the libraries `io`, `os`, `package` (and the global `require`), and puts `lfs` among the preloaded modules. */
void OpenLuaStdLibs(lua_State *state);

// The part of the script debugger that the scripts' error handler uses (Deponia_Linux.asm lines
// 129698-129823 and 120362-120390). The debugger itself - the remote console that stops a script at an error
// (debugTCPLoop), the hook that profiles the Lua functions (lua_debughookf, profileClean) - serves the
// developers of the game and is not reconstructed.
#include "AppGlobals.h"
#include "WxStub.h"
#include "vscommon/scripting/visLua.h"
#include "vscommon/scripting/visLuaObjects.h"

// Confirmed (asm lines 129698-129823): the error handler of the scripts (`debugerror(err)`): the
// message and then the traceback go to the log (at level 2). It gives nothing back, so the error
// that the call reports has no message.
int lua_debugerror(lua_State *state) {
	if (wxLog::loglevel > 1) {
		const char *message = lua_tolstring(state, -1, nullptr);
		wxString text;

		toUTF(&text, message ? message : "");
		wxLog::logexpanded(L"%s", text.wc_str());
	}

	lua_getfield(state, LUA_GLOBALSINDEX, "debug");
	lua_pushstring(state, "traceback");
	lua_gettable(state, -2);
	lua_remove(state, -2);
	lua_pcall(state, 0, 1, 0);

	if (wxLog::loglevel > 1) {
		const char *traceback = lua_tolstring(state, -1, nullptr);
		wxString text;

		toUTF(&text, traceback ? traceback : "");
		wxLog::logexpanded(L"%s", text.wc_str());
	}

	return 0;
}

void profileClean() {
}

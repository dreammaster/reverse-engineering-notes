// Confirmed (Deponia_Linux.asm lines 286098-289015, 144410-144480): the global `system` of the scripts. Reading a name
// of it gives a function (`system.pauseAllSounds()`) or a value (`system.savegamesCount`, `frameTime` ...), writing
// some names changes the engine (`system.pauseMainLoops = true`). The name of the source file is not recovered.
#include <cstdio>
#include <cstring>
#include <list>
#include <string>
#include <unordered_map>

#include "vscommon/scripting/visLua.h"
#include "AppGlobals.h"
#include "SdlStub.h"
#include "TGAction.h"
#include "TGScene.h"
#include "TSoundFFMPEG.h"
#include "sha1.h"
#include "common/lua/lauxlib.h"
#include "graphicslib/graphics.h"
#include "graphicslib/subsys.h"
#include "vscommon/canimation.h"
#include "vsplayer/animationGame.h"
#include "vsplayer/control/gameControl.h"
#include "vsplayer/control/gameController.h"
#include "vsplayer/scripting/luaSystem.h"
#include "vstables/fieldIds.h"
#include "vstables/visionaireGame.h"

static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

/** The functions of `system` by name (asm `system_luafunctions`: a hash map made when the object is). */
static std::unordered_map<std::string, lua_CFunction> system_luafunctions;

// Confirmed (asm lines 438459-438465)
int graphics_gc(lua_State *) {
	return 0;
}

// Confirmed (asm lines 144410-144468): the memory of the textures of the video card plus what the engine itself
// takes (the number 0x1172680, about 18 MB). TODO: the sum of the sizes of the textures of the GL backend
// (TGraphicsInterface::GetTextures()), which is not reconstructed.
unsigned long getGPUMem() {
	return 0x1172680;
}

// Confirmed (asm lines 144468-144476)
unsigned long getProcessMem() {
	return 0;
}

// Confirmed (asm lines 286098-286235): `system.pauseEngineUpdate`, `pauseMainLoops`, `pauseOnFocusLost` and
// `moviePauseAllowed` take a boolean, `selectedSavegame` a number.
static int system_newindex(lua_State *state) {
	const char *name = lua_tolstring(state, 2, nullptr);
	int type = lua_type(state, 3);

	if (!name)
		return 0;

	if (type == LUA_TNUMBER) {
		if (std::strcmp(name, "selectedSavegame") != 0)
			return 0;

		gameControl()->GetScene()->SetSelectedSavegame(static_cast<int>(lua_tointeger(state, 3)));
		return 1;
	}

	if (type != LUA_TBOOLEAN)
		return 0;

	if (std::strcmp(name, "moviePauseAllowed") == 0) {
		g_bMoviePauseAllowed = lua_toboolean(state, 3) != 0;
	} else if (std::strcmp(name, "pauseOnFocusLost") == 0) {
		CanLoseFocus = lua_toboolean(state, 3) != 0;
	} else if (std::strcmp(name, "pauseMainLoops") == 0) {
		MainLoopsPaused = lua_toboolean(state, 3) != 0;
	} else if (std::strcmp(name, "pauseEngineUpdate") == 0) {
		// (the object under the mouse is let go)
		TVisObjRef game = gameControl()->GetVisionaire()->GetGame();

		game.ClearLink(kGameCurrentObject, true);
		EngineUpdatePaused = lua_toboolean(state, 3) != 0;
	}

	return 0;
}

// The sounds of all kinds but the music: sound, speech, the second kind of sound and the global ones.
static const TSoundTypeEnum kSoundKinds[] = {TSoundTypeEnum::kSound, TSoundTypeEnum::kSpeech, TSoundTypeEnum::kSound2,
                                             TSoundTypeEnum::kGlobal
                                            };

// Confirmed (asm lines 286245-286281)
static int system_resumeAllSounds(lua_State *) {
	for (TSoundTypeEnum kind : kSoundKinds)
		gameControl()->GetSoundManager()->Continue(kind);

	return 0;
}

// Confirmed (asm lines 286288-286303)
static int system_resumeBackgroundMusic(lua_State *) {
	gameControl()->GetSoundManager()->Continue(TSoundTypeEnum::kMusic);
	return 0;
}

// Confirmed (asm lines 286313-286349)
static int system_pauseAllSounds(lua_State *) {
	for (TSoundTypeEnum kind : kSoundKinds)
		gameControl()->GetSoundManager()->Pause(kind);

	return 0;
}

// Confirmed (asm lines 286356-286371)
static int system_pauseBackgroundMusic(lua_State *) {
	gameControl()->GetSoundManager()->Pause(TSoundTypeEnum::kMusic);
	return 0;
}

// Confirmed (asm lines 286381-286406)
static int system_resumeAllAnimations(lua_State *) {
	for (TGAnimation *animation : TGAnimation::GetRunningAnimations())
		animation->SetPaused(false);

	return 0;
}

// Confirmed (asm lines 286416-286441)
static int system_pauseCurrentAnimations(lua_State *) {
	for (TGAnimation *animation : TGAnimation::GetRunningAnimations())
		animation->SetPaused(true);

	return 0;
}

// Confirmed (asm lines 286451-286476)
static int system_resumeAllActions(lua_State *) {
	for (TGAction *action : TGAction::GetRunningActions())
		action->SetPaused(false);

	return 0;
}

// Confirmed (asm lines 286486-286511)
static int system_pauseCurrentActions(lua_State *) {
	for (TGAction *action : TGAction::GetRunningActions())
		action->SetPaused(true);

	return 0;
}

// The open controller with the number (the instance id of its joystick), or null.
static SDL_GameController *findController(int id) {
	for (const auto &controller : TGameController::GetGameControllers()) {
		if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller.first)) == id)
			return controller.first;
	}

	return nullptr;
}

// Confirmed (asm lines 286522-286561): one more than the highest number of the open controllers.
static int system_controllercount(lua_State *state) {
	int count = 0;

	if (!TGameController::GetGameControllers().empty()) {
		int highest = -1;

		for (const auto &controller : TGameController::GetGameControllers()) {
			int id = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller.first));

			if (id > highest)
				highest = id;
		}

		count = highest + 1;
	}

	lua_pushinteger(state, count);
	return 1;
}

// Confirmed (asm lines 286573-286631): the name of the joystick and the name of the controller.
static int system_controllername(lua_State *state) {
	int id = static_cast<int>(luaL_checkinteger(state, 1));
	SDL_GameController *controller = findController(id);

	if (!controller)
		return 0;

	const char *joystickName = SDL_JoystickName(SDL_GameControllerGetJoystick(controller));
	const char *controllerName = SDL_GameControllerName(controller);

	lua_pushstring(state, joystickName ? joystickName : "");
	lua_pushstring(state, controllerName ? controllerName : "");
	return 2;
}

// Confirmed (asm lines 286649-286660)
static int system_controllerisgame(lua_State *state) {
	lua_pushboolean(state, SDL_IsGameController(static_cast<int>(luaL_checkinteger(state, 1))));
	return 1;
}

// Confirmed (asm lines 286676-286734)
static int system_controllerguid(lua_State *state) {
	int id = static_cast<int>(luaL_checkinteger(state, 1));
	SDL_GameController *controller = findController(id);

	if (!controller)
		return 0;

	char text[33];

	SDL_JoystickGetGUIDString(SDL_JoystickGetGUID(SDL_GameControllerGetJoystick(controller)), text, sizeof(text));
	lua_pushstring(state, text);
	return 1;
}

// Confirmed (asm lines 286753-286766)
static int system_joystickcount(lua_State *state) {
	lua_pushinteger(state, SDL_NumJoysticks());
	return 1;
}

// Confirmed (asm lines 286777-286840): a table with the name of the video card and the number of CPU cores.
static int system_systemInfo(lua_State *state) {
	lua_createtable(state, 0, 0);

	lua_pushstring(state, "gpu");
	lua_pushstring(state, g_subSys ? g_subSys->GetGPUName().c_str() : "");
	lua_settable(state, -3);

	lua_pushstring(state, "cpuCores");
	lua_pushinteger(state, SDL_GetCPUCount());
	lua_settable(state, -3);
	return 1;
}

// Confirmed (asm lines 287051-287250): the modes of the first display with 32 bits for a pixel, as "WIDTHxHEIGHT"
// texts in a list. A mode with the same width or the same height as the one before it (the list of SDL is sorted)
// is left out.
static int system_getDisplayModes(lua_State *state) {
	int count = 0;
	int previousWidth = -1;
	int previousHeight = -1;

	lua_createtable(state, 0, 0);

	for (int index = 0; index < SDL_GetNumDisplayModes(0); index++) {
		SDL_DisplayMode mode;

		if (SDL_GetDisplayMode(0, index, &mode) != 0)
			continue;

		switch (mode.format) {
		case SDL_PIXELFORMAT_RGB888:
		case SDL_PIXELFORMAT_RGBX8888:
		case SDL_PIXELFORMAT_BGR888:
		case SDL_PIXELFORMAT_BGRX8888:
		case SDL_PIXELFORMAT_ARGB8888:
		case SDL_PIXELFORMAT_RGBA8888:
		case SDL_PIXELFORMAT_ABGR8888:
		case SDL_PIXELFORMAT_BGRA8888:
			break;
		default:
			continue;
		}

		if (mode.w == previousWidth || mode.h == previousHeight)
			continue;

		char text[32];

		std::snprintf(text, sizeof(text), "%dx%d", mode.w, mode.h);
		lua_pushinteger(state, ++count);
		lua_pushstring(state, text);
		lua_settable(state, -3);
		previousWidth = mode.w;
		previousHeight = mode.h;
	}

	return 1;
}

// Confirmed (asm lines 288178-288612): a function of `system` by its name, or one of the values.
static int system_index(lua_State *state) {
	int type = lua_type(state, 2);
	const char *name = lua_tolstring(state, 2, nullptr);

	if (type != LUA_TSTRING)
		return 0;

	auto function = system_luafunctions.find(name);

	if (function != system_luafunctions.end()) {
		lua_pushcclosure(state, function->second, 0);
		return 1;
	}

	if (std::strcmp(name, "vramUsed") == 0) {
		lua_pushinteger(state, static_cast<lua_Integer>(getGPUMem() >> 20));
	} else if (std::strcmp(name, "memoryUsed") == 0) {
		lua_pushinteger(state, static_cast<lua_Integer>(getProcessMem() >> 20));
	} else if (std::strcmp(name, "lastFrameTime") == 0) {
		lua_pushinteger(state, lastFrameTime);
	} else if (std::strcmp(name, "frameTime") == 0) {
		lua_pushinteger(state, static_cast<lua_Integer>(fps));
	} else if (std::strcmp(name, "cacheContents") == 0) {
		// The sprite cache of the graphics as lines of text (asm lines 288385-288440), a line each.
		std::list<wxString> contents;
		std::wstring text;

		graphics->GetSpriteCache()->PrintCacheContents(contents);

		for (const wxString &line : contents) {
			text += line.ToStdWstring();
			text += L"\n";
		}

		lua_pushstring(state, wxString(text).mb_str());
	} else if (std::strcmp(name, "savegamesCount") == 0) {
		lua_pushinteger(state, gameControl()->GetScene()->GetSavegameCount());
	} else if (std::strcmp(name, "selectedSavegame") == 0) {
		lua_pushinteger(state, gameControl()->GetScene()->GetSelectedSavegameIndex());
	} else if (std::strcmp(name, "savegamesScrollPos") == 0) {
		lua_pushinteger(state, gameControl()->GetScene()->GetFirstVisibleSavegame());
	} else {
		return 0;
	}

	return 1;
}

// Confirmed (asm lines 3016913-3017000, `system_meths`)
static const luaL_Reg system_meths[] = {
	{"__index", system_index},
	{"__newindex", system_newindex},
	{"__gc", graphics_gc},
	{"getDisplayModes", system_getDisplayModes},
	{"controllerGuid", system_controllerguid},
	{"controllerName", system_controllername},
	{"controllerCount", system_controllercount},
	{"joystickCount", system_joystickcount},
	{"joystickIsController", system_controllerisgame},
	{"pauseCurrentActions", system_pauseCurrentActions},
	{"resumeAllActions", system_resumeAllActions},
	{"pauseCurrentAnimations", system_pauseCurrentAnimations},
	{"resumeAllAnimations", system_resumeAllAnimations},
	{"pauseBackgroundMusic", system_pauseBackgroundMusic},
	{"pauseAllSounds", system_pauseAllSounds},
	{"resumeBackgroundMusic", system_resumeBackgroundMusic},
	{"resumeAllSounds", system_resumeAllSounds},
	{"systemInfo", system_systemInfo},
	{nullptr, nullptr}
};

// Confirmed (asm lines 288742-289015)
void lua_open_system(lua_State *state) {
	luaL_newmetatable(state, "Visionaire.TSystem");
	lua_pushlstring(state, "__index", 7);
	lua_pushvalue(state, -2);
	lua_rawset(state, -3);
	luaL_openlib(state, nullptr, system_meths, 0);

	for (const luaL_Reg *entry = system_meths; entry->name; entry++)
		system_luafunctions[entry->name] = entry->func;

	lua_createtable(state, 0, 0);
	lua_getfield(state, LUA_REGISTRYINDEX, "Visionaire.TSystem");
	lua_setmetatable(state, -2);
	lua_setfield(state, LUA_GLOBALSINDEX, "system");
}

// Confirmed (asm lines 440291-440333): sha1("text") is the hash as 40 hex digits.
int lua_sha1(lua_State *state) {
	const char *text = luaL_checklstring(state, 1, nullptr);
	unsigned char hash[20];
	char hex[41];

	sha1::calc(text, static_cast<int>(std::strlen(text)), hash);
	sha1::toHexString(hash, hex);
	lua_pushstring(state, hex);
	return 1;
}

// Confirmed (asm lines 443641-443733): setDelay(milliseconds, function or name): runs the function (the Lua function
// itself, kept by a reference) or the global function of that name after the time.
int lua_setDelay(lua_State *state) {
	int delay = static_cast<int>(luaL_checkinteger(state, 1));
	int type = lua_type(state, 2);

	if (type == LUA_TSTRING) {
		gameControl()->SetDelay(delay, std::string(lua_tolstring(state, 2, nullptr)));
	} else if (type == LUA_TFUNCTION) {
		gameControl()->SetDelay(delay, luaL_ref(state, LUA_REGISTRYINDEX));
	} else {
		lua_pushstring(state, "param 2 should be string or function");
		lua_error(state);
	}

	return 0;
}

// Confirmed (asm lines 447452-447586): `InitDrawLua` makes the Lua side of the drawing - the Box2D physics (tolua_b2_open: not
// reconstructed, a third-party library), the classes of the sprites, framebuffers, buffers and movies, the global `sha1`, the
// object `graphics`, the object `system` and the global `setDelay`.
void InitDrawLua(lua_State *state) {
	lua_settop(state, 0);

	luaopen_Graphics(state);

	lua_pushcclosure(state, lua_sha1, 0);
	lua_setfield(state, LUA_GLOBALSINDEX, "sha1");

	luaopen_GraphicsObject(state);
	lua_open_system(state);

	lua_pushcclosure(state, lua_setDelay, 0);
	lua_setfield(state, LUA_GLOBALSINDEX, "setDelay");
}

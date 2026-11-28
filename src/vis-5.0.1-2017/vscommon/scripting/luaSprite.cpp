// Confirmed (Deponia_Linux.asm lines 1417094-1418795): how the scripts see a sprite (TSprite). It is
// a userdata that holds a pointer to a TSprite of its own (made by createSprite("path") or from a
// TSprite by CreateTSprite()), with the metatable "Visionaire.TSprite" and the functions setPath,
// getPath, getSize, setPosition and getPosition.
#include "vscommon/scripting/visLua.h"
#include "vscommon/scripting/visLuaObjects.h"

#include "TCharHolder.h"
#include "TSprite.h"
#include "common/lua/lauxlib.h"
#include "graphicslib/graphics.h"
#include "graphicslib/picture.h"

static const char *const kSpriteMetatable = "Visionaire.TSprite";
static const char *const kSpriteExpected = "'Sprite' expected";

/** A number of the table at the top of the stack, by its key (0 when it is not a number). */
static int tableNumber(lua_State *state, const char *key) {
	lua_pushstring(state, key);
	lua_gettable(state, -2);

	int value = static_cast<int>(lua_tonumber(state, -1));

	lua_settop(state, -2);
	return value;
}

static void pushPoint(lua_State *state, int x, int y) {
	lua_createtable(state, 0, 2);
	lua_pushstring(state, "x");
	lua_pushinteger(state, x);
	lua_settable(state, -3);
	lua_pushstring(state, "y");
	lua_pushinteger(state, y);
	lua_settable(state, -3);
}

// Confirmed (asm lines 1418748-1418795)
TSprite **CheckSprite(lua_State *state, int index) {
	TSprite **sprite = static_cast<TSprite **>(luaL_checkudata(state, index, kSpriteMetatable));

	if (!sprite)
		luaL_argerror(state, index, kSpriteExpected);

	return sprite;
}

// Confirmed (asm lines 1417094-1417187): setPosition({x = , y = }); a position only (the scale of the
// sprite stays, as the scale -1 is below 0).
static int SetPosition(lua_State *state) {
	TSprite **sprite = CheckSprite(state, 1);

	if (lua_type(state, 2) != LUA_TTABLE)
		return 0;

	wxPoint position;

	position.x = tableNumber(state, "x");
	position.y = tableNumber(state, "y");
	(*sprite)->SetPosition(position, -1.0f);
	return 0;
}

// Confirmed (asm lines 1417187-1417253): getPosition() is {x, y}.
static int GetPosition(lua_State *state) {
	TSprite **sprite = CheckSprite(state, 1);
	wxPoint position = (*sprite)->GetPosition();

	pushPoint(state, position.x, position.y);
	return 1;
}

// Confirmed (asm lines 1417253-1417306)
static int DeleteSprite(lua_State *state) {
	TSprite **sprite = CheckSprite(state, 1);

	delete *sprite;
	*sprite = nullptr;
	return 0;
}

// Confirmed (asm lines 1417446-1417750): getSize() is {x = width, y = height} of the picture. The
// picture is looked up in the graphics cache by the name that TPictureIO::GetSpriteName() gives
// (an undefined transparency becomes alpha first); when it is not in the cache the picture is loaded
// for the answer, and a picture that does not load gives -1, -1.
static int GetSize(lua_State *state) {
	TSprite **sprite = CheckSprite(state, 1);
	TSprite *self = *sprite;

	if (self->GetTransparency() == eTransparencyMode::kAny)
		self->SetTransparency(eTransparencyMode::kAlpha, 0);

	int color = 0;

	if (self->GetTransparency() == eTransparencyMode::kColorKey)
		color = static_cast<int>(self->GetTransparentColor());

	std::wstring key = self->GetPath().GetFullPath().ToStdWstring() + L"_0_" +
	                   std::to_wstring(static_cast<int>(self->GetTransparency())) + L"_" + std::to_wstring(color) + L"_0";
	TSpriteHandle *handle = graphics->GetSpriteFromCache(wxString(key));
	TPictureIO *picture = nullptr;

	if (!handle) {
		picture = new TPictureIO(*self, false);
		picture->RefreshSprite(false);
		handle = picture->GetSpriteHandle();
	}

	int width = -1;
	int height = -1;

	if (handle) {
		width = handle->width;
		height = handle->height;
	}

	pushPoint(state, width, height);
	delete picture;
	return 1;
}

// Confirmed (asm lines 1417750-1417893)
static int GetPath(lua_State *state) {
	TSprite **sprite = CheckSprite(state, 1);

	lua_pushstring(state, (*sprite)->GetPath().GetFullPath().mb_str());
	return 1;
}

// Confirmed (asm lines 1417893-1418155): setPath("path"); the path is made absolute as it is.
static int SetPath(lua_State *state) {
	TSprite **sprite = CheckSprite(state, 1);

	if (lua_type(state, 2) != LUA_TSTRING)
		return 0;

	wxString text;

	toUTF(&text, lua_tolstring(state, 2, nullptr));

	wxFileName path(text.ToStdWstring());

	path.NormalizePath();
	(*sprite)->SetPath(TCharHolder(path.GetFullPath()));
	return 0;
}

// Confirmed (asm lines 1418155-1418486): tostring(sprite)
static int PrintSprite(lua_State *state) {
	TSprite **sprite = CheckSprite(state, 1);
	std::wstring text;

	if ((*sprite)->IsEmpty()) {
		text = L"Sprite: Empty";
	} else {
		wxPoint position = (*sprite)->GetPosition();

		text = L"Sprite: path " + (*sprite)->GetPath().GetFullPath().ToStdWstring() + L", width " +
		       std::to_wstring((*sprite)->GetWidth()) + L", height " + std::to_wstring((*sprite)->GetHeight()) +
		       L", pos (" + std::to_wstring(position.x) + L"," + std::to_wstring(position.y) + L")";
	}

	lua_pushstring(state, wxString(text).mb_str());
	return 1;
}

// Confirmed (asm lines 1418486-1418688): createSprite("path")
int CreateSprite(lua_State *state) {
	const char *name = luaL_checklstring(state, 1, nullptr);
	TSprite **sprite = static_cast<TSprite **>(lua_newuserdata(state, sizeof(TSprite *)));
	wxString text;

	toUTF(&text, name);

	wxFileName path(text.ToStdWstring());

	path.NormalizePath();
	*sprite = new TSprite(path);
	lua_getfield(state, LUA_REGISTRYINDEX, kSpriteMetatable);
	lua_setmetatable(state, -2);
	return 1;
}

// Confirmed (asm lines 1418688-1418748): a sprite for the scripts from a copy of `sprite`.
void CreateTSprite(lua_State *state, const TSprite &sprite) {
	TSprite **userdata = static_cast<TSprite **>(lua_newuserdata(state, sizeof(TSprite *)));

	*userdata = new TSprite(sprite);
	lua_getfield(state, LUA_REGISTRYINDEX, kSpriteMetatable);
	lua_setmetatable(state, -2);
}

namespace {

struct LuaMethod {
	const char *name;
	lua_CFunction function;
};

// Confirmed (asm: sprite_m)
const LuaMethod kSpriteMethods[] = {
	{"__tostring", PrintSprite},
	{"__gc", DeleteSprite},
	{"setPath", SetPath},
	{"getPath", GetPath},
	{"getSize", GetSize},
	{"setPosition", SetPosition},
	{"getPosition", GetPosition},
};

} // End of anonymous namespace

// Confirmed (asm lines 1417306-1417446): the registration is the one of the usual "Lunar" class template,
// with its oddities kept: the table of the methods is the "__metatable" and the "__index" of the
// metatable; its own metatable has `new` as itself (the `__call` is put in the methods).
int luaopen_Sprite(lua_State *state) {
	lua_createtable(state, 0, 0);

	int methods = lua_gettop(state);

	luaL_newmetatable(state, kSpriteMetatable);

	int metatable = lua_gettop(state);

	lua_pushlstring(state, "__metatable", 11);
	lua_pushvalue(state, methods);
	lua_settable(state, metatable);

	lua_pushlstring(state, "__tostring", 10);
	lua_pushcclosure(state, PrintSprite, 0);
	lua_settable(state, metatable);

	lua_pushstring(state, "__index");
	lua_pushvalue(state, methods);
	lua_settable(state, -3);

	lua_pushlstring(state, "__gc", 4);
	lua_pushcclosure(state, DeleteSprite, 0);
	lua_settable(state, -3);

	lua_createtable(state, 0, 0);

	int methodsMeta = lua_gettop(state);

	lua_pushlstring(state, "__call", 6);
	lua_pushcclosure(state, CreateSprite, 0);
	lua_settable(state, methods);

	lua_pushlstring(state, "new", 3);
	lua_pushvalue(state, -2);
	lua_settable(state, methodsMeta);
	lua_setmetatable(state, methods);

	for (const LuaMethod &method : kSpriteMethods) {
		lua_pushstring(state, method.name);
		lua_pushcclosure(state, method.function, 0);
		lua_settable(state, methods);
	}

	lua_settop(state, -3);
	lua_pushcclosure(state, CreateSprite, 0);
	lua_setfield(state, LUA_GLOBALSINDEX, "createSprite");
	return 1;
}

#include "vscommon/scripting/luaConversion.h"

#include "common/lua/lua.h"
#include "vscommon/scripting/visLua.h"

// Reads the field `name` of the table at `index` as a number into `out`; false if it is not there.
// (The key is on the stack when the table is asked, see luaConversion.h; the value is let go.)
static bool readNumber(const char *name, int index, int &out) {
	lua_pushstring(L, name);
	lua_gettable(L, index);

	if (lua_type(L, -1) == LUA_TNIL) {
		lua_settop(L, -2);
		return false;
	}

	out = (int)lua_tonumber(L, -1);
	lua_settop(L, -2);
	return true;
}

static void setInteger(const char *name, lua_Integer value) {
	lua_pushstring(L, name);
	lua_pushinteger(L, value);
	lua_settable(L, -3);
}

// Confirmed (asm lines 556367-556432)
bool ConvertFromLua(wxPoint &point, int index) {
	return readNumber("x", index, point.x) && readNumber("y", index, point.y);
}

// Confirmed (asm lines 556442-556471)
void ConvertToLua(const wxPoint &point) {
	lua_createtable(L, 0, 2);
	setInteger("x", point.x);
	setInteger("y", point.y);
}

// Confirmed (asm lines 556481-556584)
bool ConvertFromLua(wxRect &rect, int index) {
	return readNumber("x", index, rect.x) && readNumber("y", index, rect.y) &&
	       readNumber("width", index, rect.width) && readNumber("height", index, rect.height);
}

// Confirmed (asm lines 556594-556641)
void ConvertToLua(const wxRect &rect) {
	lua_createtable(L, 0, 4);
	setInteger("x", rect.x);
	setInteger("y", rect.y);
	setInteger("width", rect.width);
	setInteger("height", rect.height);
}

// Confirmed (asm lines 556651-556839): the path has to be there; the position (a table), the
// transparency mode, the transparent color and the pause are left as a new sprite has them when
// they are not (-1: the position keeps its scale, the mode is not set, the pause is -1).
bool ConvertFromLua(TSprite &sprite, int index) {
	lua_pushstring(L, "path");
	lua_gettable(L, index);

	if (lua_type(L, -1) == LUA_TNIL) {
		lua_settop(L, -2);
		return false;
	}

	sprite.SetPath(TCharHolder(lua_tolstring(L, -1, nullptr)));
	lua_settop(L, -2);

	wxPoint position;

	lua_pushstring(L, "position");
	lua_gettable(L, index);

	if (lua_type(L, -1) == LUA_TTABLE)
		ConvertFromLua(position, -2);

	lua_settop(L, -2);
	sprite.SetPosition(position, -1.0f);

	int transparency = -1;

	lua_pushstring(L, "transparency");
	lua_gettable(L, index);

	if (lua_type(L, -1) == LUA_TNUMBER)
		transparency = (int)lua_tointeger(L, -1);

	lua_settop(L, -2);

	unsigned int color = (unsigned int) -1;

	lua_pushstring(L, "transpcolor");
	lua_gettable(L, index);

	if (lua_type(L, -1) == LUA_TNUMBER)
		color = (unsigned int)(long long)lua_tonumber(L, -1);

	lua_settop(L, -2);
	sprite.SetTransparency((eTransparencyMode)transparency, color);

	int pause = -1;

	lua_pushstring(L, "pause");
	lua_gettable(L, index);

	if (lua_type(L, -1) == LUA_TNUMBER)
		pause = (int)lua_tonumber(L, -1);

	lua_settop(L, -2);
	sprite.SetPause(pause);
	return true;
}

// Confirmed (asm lines 556849-557086)
void ConvertToLua(const TSprite &sprite) {
	lua_createtable(L, 0, 5);

	lua_pushstring(L, "path");
	lua_pushstring(L, sprite.GetPath().GetFullPath().mb_str());
	lua_settable(L, -3);

	lua_pushstring(L, "position");
	ConvertToLua(sprite.GetPosition());
	lua_settable(L, -3);

	setInteger("transparency", (int)sprite.GetTransparency());
	setInteger("transpcolor", sprite.GetTransparentColor());
	setInteger("pause", sprite.GetPause());
}

// Confirmed (asm lines 557096-557194): the sound and the text have to be there; the language is -1
// when it is not.
bool ConvertFromLua(TTextLanguage &text, int index) {
	lua_pushstring(L, "sound");
	lua_gettable(L, index);

	if (lua_type(L, -1) == LUA_TNIL) {
		lua_settop(L, -2);
		return false;
	}

	text.audioFile = TCharHolder(lua_tolstring(L, -1, nullptr));
	lua_settop(L, -2);

	lua_pushstring(L, "text");
	lua_gettable(L, index);

	if (lua_type(L, -1) == LUA_TNIL) {
		lua_settop(L, -2);
		return false;
	}

	text.text = TCharHolder(lua_tolstring(L, -1, nullptr));
	lua_settop(L, -2);

	int language = -1;

	lua_pushstring(L, "language");
	lua_gettable(L, index);

	if (lua_type(L, -1) == LUA_TNUMBER)
		language = (int)lua_tonumber(L, -1);

	lua_settop(L, -2);
	text.languageId = language;
	return true;
}

// The text of a TCharHolder for Lua ("" when it has none).
static const char *luaText(const TCharHolder &holder) {
	return holder.mb_str();
}

// Confirmed (asm lines 557204-557253)
void ConvertToLua(const TTextLanguage &text) {
	lua_createtable(L, 0, 3);

	lua_pushstring(L, "text");
	lua_pushstring(L, luaText(text.text));
	lua_settable(L, -3);

	lua_pushstring(L, "sound");
	lua_pushstring(L, luaText(text.audioFile));
	lua_settable(L, -3);

	setInteger("language", text.languageId);
}

// Confirmed (asm lines 557263-557303)
void ConvertToLua(const std::vector<int> &values) {
	lua_createtable(L, 0, (int)values.size());

	lua_Integer key = 1;

	for (int value : values) {
		lua_pushinteger(L, key++);
		lua_pushinteger(L, value);
		lua_settable(L, -3);
	}
}

// Confirmed (asm lines 557313-557354): (the original sets these with a raw set)
void ConvertToLua(const std::vector<float> &values) {
	lua_createtable(L, 0, (int)values.size());

	lua_Integer key = 1;

	for (float value : values) {
		lua_pushinteger(L, key++);
		lua_pushnumber(L, value);
		lua_rawset(L, -3);
	}
}

// Confirmed (asm lines 557364-557425)
void ConvertToLua(const std::vector<wxPoint> &values) {
	lua_createtable(L, 0, (int)values.size());

	lua_Integer key = 1;

	for (const wxPoint &value : values) {
		lua_pushinteger(L, key++);
		ConvertToLua(value);
		lua_settable(L, -3);
	}
}

// Confirmed (asm lines 557435-557477)
void ConvertToLua(const std::vector<TTextLanguage> &values) {
	lua_createtable(L, 0, (int)values.size());

	lua_Integer key = 1;

	for (const TTextLanguage &value : values) {
		lua_pushinteger(L, key++);
		ConvertToLua(value);
		lua_settable(L, -3);
	}
}

// Confirmed (asm lines 557487-557566)
void ConvertToLua(const std::vector<wxRect> &values) {
	lua_createtable(L, 0, (int)values.size());

	lua_Integer key = 1;

	for (const wxRect &value : values) {
		lua_pushinteger(L, key++);
		ConvertToLua(value);
		lua_settable(L, -3);
	}
}

// Confirmed (asm lines 557576-557616)
void ConvertToLua(const std::vector<TCharHolder> &values) {
	lua_createtable(L, 0, (int)values.size());

	lua_Integer key = 1;

	for (const TCharHolder &value : values) {
		lua_pushinteger(L, key++);
		lua_pushstring(L, luaText(value));
		lua_settable(L, -3);
	}
}

// Confirmed (asm lines 557632-557668)
void ConvertToLua(const std::vector<TSprite> &values) {
	lua_createtable(L, 0, (int)values.size());

	lua_Integer key = 1;

	for (const TSprite &value : values) {
		lua_pushinteger(L, key++);
		ConvertToLua(value);
		lua_settable(L, -3);
	}
}

// Confirmed (asm lines 557684-557810): the points 1, 2 ... of the table, until there is none; every
// one needs its x and y.
bool ConvertFromLua(std::vector<wxPoint> &values, int index) {
	for (lua_Integer key = 1;; key++) {
		lua_pushinteger(L, key);
		lua_gettable(L, index);

		if (lua_type(L, -1) == LUA_TNIL) {
			lua_settop(L, -2);
			return true;
		}

		wxPoint point;

		if (!readNumber("x", -2, point.x) || !readNumber("y", -2, point.y)) {
			lua_settop(L, -2);
			return false;
		}

		values.push_back(point);
		lua_settop(L, -2);
	}
}

// Confirmed (asm lines 557822-557915)
bool ConvertFromLua(std::vector<wxRect> &values, int index) {
	for (lua_Integer key = 1;; key++) {
		lua_pushinteger(L, key);
		lua_gettable(L, index);

		if (lua_type(L, -1) == LUA_TNIL) {
			lua_settop(L, -2);
			return true;
		}

		wxRect rect;

		if (!ConvertFromLua(rect, -2)) {
			lua_settop(L, -2);
			return false;
		}

		values.push_back(rect);
		lua_settop(L, -2);
	}
}

// Confirmed (asm lines 557925-558051): every entry has to be a table.
bool ConvertFromLua(std::vector<TSprite> &values, int index) {
	for (lua_Integer key = 1;; key++) {
		lua_pushinteger(L, key);
		lua_gettable(L, index);

		if (lua_type(L, -1) == LUA_TNIL) {
			lua_settop(L, -2);
			return true;
		}

		if (lua_type(L, -1) != LUA_TTABLE) {
			lua_settop(L, -2);
			return false;
		}

		TSprite sprite;

		if (!ConvertFromLua(sprite, -2)) {
			lua_settop(L, -2);
			return false;
		}

		values.push_back(sprite);
		lua_settop(L, -2);
	}
}

// Confirmed (asm lines 558061-558146): every entry has to be a number.
bool ConvertFromLua(std::vector<int> &values, int index) {
	for (lua_Integer key = 1;; key++) {
		lua_pushinteger(L, key);
		lua_gettable(L, index);

		if (lua_type(L, -1) == LUA_TNIL) {
			lua_settop(L, -2);
			return true;
		}

		if (lua_type(L, -1) != LUA_TNUMBER) {
			lua_settop(L, -2);
			return false;
		}

		values.push_back((int)lua_tonumber(L, -1));
		lua_settop(L, -2);
	}
}

// Confirmed (asm lines 558160-558320): the same, without going through the metatables.
bool ConvertFromLua(std::vector<float> &values, int index) {
	values.reserve(lua_objlen(L, index));

	for (lua_Integer key = 1;; key++) {
		lua_pushinteger(L, key);
		lua_rawget(L, index);

		if (lua_type(L, -1) == LUA_TNIL) {
			lua_settop(L, -2);
			return true;
		}

		if (lua_type(L, -1) != LUA_TNUMBER) {
			lua_settop(L, -2);
			return false;
		}

		values.push_back((float)lua_tonumber(L, -1));
		lua_settop(L, -2);
	}
}

// Confirmed (asm lines 558329-558440): every entry has to be a string.
bool ConvertFromLua(std::vector<TCharHolder> &values, int index) {
	for (lua_Integer key = 1;; key++) {
		lua_pushinteger(L, key);
		lua_gettable(L, index);

		if (lua_type(L, -1) == LUA_TNIL) {
			lua_settop(L, -2);
			return true;
		}

		if (lua_type(L, -1) != LUA_TSTRING) {
			lua_settop(L, -2);
			return false;
		}

		values.push_back(TCharHolder(lua_tolstring(L, -1, nullptr)));
		lua_settop(L, -2);
	}
}

// Confirmed (asm lines 558449-558575)
bool ConvertFromLua(std::vector<TTextLanguage> &values, int index) {
	for (lua_Integer key = 1;; key++) {
		lua_pushinteger(L, key);
		lua_gettable(L, index);

		if (lua_type(L, -1) == LUA_TNIL) {
			lua_settop(L, -2);
			return true;
		}

		TTextLanguage text;

		if (!ConvertFromLua(text, -2)) {
			lua_settop(L, -2);
			return false;
		}

		values.push_back(text);
		lua_settop(L, -2);
	}
}

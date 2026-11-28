#include "vscommon/scripting/argumentParser.h"

#include "Diagnostics.h"
#include "common/lua/lauxlib.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/luaConversion.h"
#include "vscommon/scripting/visLua.h"
#include "vscommon/scripting/visLuaObjects.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vscommon/scripting/argumentParser.cpp";

static wxString utf(const char *text, size_t length) {
	wxString result;
	std::string copy(text, length);

	toUTF(&result, copy.c_str());
	return result;
}

/** Whether the value of `key` in the table at `index` is of the type (LUA_TNONE: any but nil). */
static bool hasKey(int index, const char *key, int type) {
	lua_pushstring(L, key);
	lua_gettable(L, index);

	int found = lua_type(L, -1);

	lua_settop(L, -2);
	return type == LUA_TNONE ? found != LUA_TNIL : found == type;
}

// Confirmed (asm lines 1432257-1432314)
bool IsPointArgument(int index) {
	return hasKey(index, "x", LUA_TNONE) && hasKey(index, "y", LUA_TNONE);
}

// Confirmed (asm lines 1432314-1432404)
bool IsRectArgument(int index) {
	return hasKey(index, "x", LUA_TNONE) && hasKey(index, "y", LUA_TNONE) && hasKey(index, "width", LUA_TNONE) &&
	       hasKey(index, "height", LUA_TNONE);
}

// Confirmed (asm lines 1432404-1432444)
bool IsSpriteArgument(int index) {
	return hasKey(index, "path", LUA_TSTRING);
}

// Confirmed (asm lines 1432444-1432485)
bool IsTextArgument(int index) {
	return hasKey(index, "text", LUA_TSTRING);
}

// Confirmed (asm lines 1432485-1432697)
TArgType GetArgumentType(int index) {
	if (hasKey(index, "path", LUA_TSTRING))
		return TArgType::kSprite;

	if (IsRectArgument(index))
		return TArgType::kRect;

	if (IsPointArgument(index))
		return TArgType::kPoint;

	if (hasKey(index, "text", LUA_TSTRING))
		return TArgType::kText;

	// a list: by its first element
	lua_pushinteger(L, 1);
	lua_gettable(L, index);

	TArgType type = TArgType::kAny;

	switch (lua_type(L, -1)) {
	case LUA_TNUMBER:
		type = TArgType::kFloatList;
		break;
	case LUA_TSTRING:
		type = TArgType::kStringList;
		break;
	case LUA_TTABLE: {
		int element = lua_gettop(L);

		if (hasKey(element, "path", LUA_TSTRING))
			type = TArgType::kSpriteList;
		else if (IsRectArgument(element))
			type = TArgType::kRectList;
		else if (IsPointArgument(element))
			type = TArgType::kPointList;
		else if (IsTextArgument(element))
			type = TArgType::kTextList;
		break;
	}
	case LUA_TUSERDATA:
		type = TArgType::kObjectList;
		break;
	default:
		break;
	}

	lua_settop(L, -2);
	return type;
}

// Confirmed (asm lines 1433399-1433460)
bool IsArgumentFlags(int index) {
	return lua_type(L, index) == LUA_TTABLE && hasKey(index, "flags", LUA_TNONE);
}

/** Reads the table at `index` as a value of the type and sets the argument. */
template<typename T>
static bool tableValue(TArgument &argument, int index, T &value) {
	if (!ConvertFromLua(value, index))
		return false;

	argument.Set(value);
	return true;
}

// Confirmed (asm lines 1433460-1434417)
bool ConvertArgumentFromLua(TArgument &argument, TArgType type, int index) {
	switch (lua_type(L, index)) {
	case LUA_TBOOLEAN:
		if (type != TArgType::kAny && type != TArgType::kBool)
			return false;

		argument.Set(lua_toboolean(L, index) != 0);
		return true;
	case LUA_TNUMBER:
		if (type == TArgType::kInt) {
			argument.Set(static_cast<int>(lua_tointeger(L, index)));
			return true;
		}

		if (type != TArgType::kAny && type != TArgType::kFloat)
			return false;

		argument.Set(static_cast<double>(lua_tonumber(L, index)));
		return true;
	case LUA_TSTRING: {
		size_t length = 0;
		const char *text = lua_tolstring(L, index, &length);

		if (type == TArgType::kAny || type == TArgType::kString) {
			argument.Set(utf(text, length));
			return true;
		}

		if (type == TArgType::kPath) {
			argument.SetPath(utf(text, length));
			return true;
		}

		if (type == TArgType::kObject) {
			TVisObjRef object;

			// (an empty string is the empty object)
			if (*text && !FindObjectByNameOrId(utf(text, length), object, true))
				return false;

			argument.Set(object);
			return true;
		}

		return false;
	}
	case LUA_TUSERDATA: {
		if (type != TArgType::kAny && type != TArgType::kObject)
			return false;

		TVisObjRef object;

		if (!GetObjectFromLua(object, index))
			return false;

		argument.Set(object);
		return true;
	}
	case LUA_TTABLE:
		if (type == TArgType::kFlags)
			return true;

		if (type == TArgType::kAny)
			type = GetArgumentType(index);

		switch (type) {
		case TArgType::kPoint: {
			wxPoint value;

			return tableValue(argument, index, value);
		}
		case TArgType::kRect: {
			wxRect value;

			return tableValue(argument, index, value);
		}
		case TArgType::kSprite: {
			TSprite value;

			return tableValue(argument, index, value);
		}
		case TArgType::kText: {
			TTextLanguage value;

			return tableValue(argument, index, value);
		}
		case TArgType::kIntList: {
			std::vector<int> value;

			return tableValue(argument, index, value);
		}
		case TArgType::kFloatList: {
			std::vector<float> value;

			return tableValue(argument, index, value);
		}
		case TArgType::kPointList: {
			std::vector<wxPoint> value;

			return tableValue(argument, index, value);
		}
		case TArgType::kRectList: {
			std::vector<wxRect> value;

			return tableValue(argument, index, value);
		}
		case TArgType::kStringList: {
			std::vector<TCharHolder> value;

			return tableValue(argument, index, value);
		}
		case TArgType::kPathList: {
			std::vector<TCharHolder> value;

			if (!ConvertFromLua(value, index))
				return false;

			argument.SetPaths(value);
			return true;
		}
		case TArgType::kSpriteList: {
			std::vector<TSprite> value;

			return tableValue(argument, index, value);
		}
		case TArgType::kObjectList: {
			TVList value;

			return tableValue(argument, index, value);
		}
		case TArgType::kTextList: {
			std::vector<TTextLanguage> value;

			return tableValue(argument, index, value);
		}
		case TArgType::kAny:
			argument.Clear();
			return true;
		default:
			return false;
		}
	default:
		return false;
	}
}

// Confirmed: strArgType, the table of names in the data. It has no text and no list of texts, so from the
// list of ints on a name is that of the type before it (kept).
const wchar_t *ArgTypeName(TArgType type) {
	static const wchar_t *const kNames[] = {
		L"none", L"bool", L"int", L"float", L"point", L"rect", L"string", L"path", L"sprite", L"visobject",
		L"intlist", L"floatlist", L"pointlist", L"rectlist", L"stringlist", L"pathlist", L"spritelist",
		L"visobjectlist", L"variant",
	};
	int index = static_cast<int>(type);

	return (index >= 0 && index < 19) ? kNames[index] : L"";
}

TArgParser::~TArgParser() {
	Clear();
}

// Confirmed (asm lines 1432697-1432884)
void TArgParser::Clear() {
	for (TArgument *argument : _arguments)
		delete argument;

	_arguments.clear();

	for (auto &flag : _flags)
		delete flag.second;

	_flags.clear();
	_flagOrder.clear();
}

// Confirmed (asm lines 1434417-1435370)
bool TArgParser::ParseArguments(const TArgSyntax &syntax) {
	_syntax = &syntax;

	int count = lua_gettop(L);

	if (count < syntax.GetMinArgs()) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Command has too few arguments.");
		return false;
	}

	int flagsTable = 0;

	for (int i = 1; i <= count; i++) {
		if (IsArgumentFlags(i)) {
			// the flags come after the mandatory arguments, and last
			if (syntax.GetMinArgs() >= i) {
				if (wxLog::loglevel >= 0)
					wxLog::logexpanded(L"Command has too few arguments");
				return false;
			}

			if (count > i) {
				if (wxLog::loglevel >= 0)
					wxLog::logexpanded(L"Flags must be last parameter.");
				return false;
			}

			flagsTable = i;
			break;
		}

		if (static_cast<int>(syntax.GetArgTypes().size()) < i) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Command has more arguments than defined in command syntax.");
			return false;
		}

		TArgument *argument = new TArgument();

		if (!ConvertArgumentFromLua(*argument, syntax.GetArgTypes()[i - 1], i)) {
			delete argument;

			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Wrong argument type for Argument #%d (expected %s)", i,
				                   ArgTypeName(syntax.GetArgTypes()[i - 1]));
			return false;
		}

		_arguments.push_back(argument);
	}

	for (const TArgFlagSyntax &flag : syntax.GetFlags()) {
		bool found = false;

		if (flagsTable != 0) {
			// the value of the flag is under its long name, or else under its short name
			lua_pushstring(L, flag.longName.mb_str());
			lua_gettable(L, flagsTable);
			found = lua_type(L, -1) != LUA_TNIL;

			if (!found) {
				lua_settop(L, -2);
				lua_pushstring(L, flag.shortName.mb_str());
				lua_gettable(L, flagsTable);
				found = lua_type(L, -1) != LUA_TNIL;
			}

			if (!found)
				lua_settop(L, -2);
		}

		if (!found) {
			if (flag.mandatory) {
				if (wxLog::loglevel >= 0)
					wxLog::logexpanded(L"Mandatory flag %s is missing.", flag.longName.wc_str());
				return false;
			}

			continue;
		}

		TArgument *argument = new TArgument();

		if (!ConvertArgumentFromLua(*argument, flag.type, flagsTable + 1)) {
			delete argument;
			lua_settop(L, -2);

			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Wrong argument type for Flag %s (expected %s)", flag.longName.wc_str(),
				                   ArgTypeName(flag.type));
			return false;
		}

		std::wstring name = flag.longName.ToStdWstring();
		auto existing = _flags.find(name);

		if (existing != _flags.end()) {
			delete existing->second;
			existing->second = argument;
		} else {
			_flags[name] = argument;
			_flagOrder.push_back(name);
		}

		lua_settop(L, -2);
	}

	return true;
}

// Confirmed (asm lines 1432884-1433034)
const TArgument *TArgParser::FindFlagArgument(wxString name) const {
	x_assert(_syntax != nullptr, "Syntax != NULL", kSourceFile, 0xD4);

	// a name of up to 3 letters is a short name
	if (name.Length() <= 3) {
		if (!_syntax->GetLongFlagName(name)) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Invalid flag \"%s\".", name.wc_str());
			return nullptr;
		}
	}

	auto found = _flags.find(name.ToStdWstring());

	return found == _flags.end() ? nullptr : found->second;
}

// Confirmed (asm lines 1433034-1433113)
bool TArgParser::IsEdit() const {
	const TArgument *edit = FindFlagArgument(wxString(L"e"));

	return edit && edit->GetBool();
}

// Confirmed (asm lines 1433113-1433270)
bool TArgParser::GetFlagName(size_t position, wxString &name) const {
	if (position >= _flagOrder.size()) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Position out of range.");
		return false;
	}

	name = wxString(_flagOrder[position]);
	return true;
}

// Confirmed (asm lines 1433270-1433362)
bool TArgParser::GetFlagArgument(const wxString &name, const TArgument **argument) const {
	const TArgument *found = FindFlagArgument(name);

	if (!found || !argument)
		return false;

	*argument = found;
	return true;
}

// Confirmed (asm lines 1433362-1433399)
bool TArgParser::GetArgument(size_t position, const TArgument **argument) const {
	if (position >= _arguments.size())
		return false;

	if (argument)
		*argument = _arguments[position];

	return true;
}

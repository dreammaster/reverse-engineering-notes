#include "vscommon/scripting/luaStdLibs.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <cwctype>
#include <filesystem>
#include <string>
#include <vector>

#ifndef FORBIDDEN_SYMBOL_ALLOW_ALL
	#define FORBIDDEN_SYMBOL_ALLOW_ALL
#endif
#include <lauxlib.h>
#include <lua.h>

#ifdef _WIN32
	#include <direct.h>
	#include <io.h>
#else
	#include <sys/stat.h>
	#include <unistd.h>
#endif

namespace fs = std::filesystem;


// ---------------------------------------------------------------------------------------------------------------------
// lfs (LuaFileSystem 1.6.3)

static int lfsError(lua_State *state, const std::error_code &error, const char *name) {
	lua_pushnil(state);
	lua_pushfstring(state, "%s: %s", name, error.message().c_str());
	lua_pushinteger(state, error.value());
	return 3;
}

static int lfs_currentdir(lua_State *state) {
	std::error_code error;
	std::string path = fs::current_path(error).string();

	if (error)
		return lfsError(state, error, "");
	lua_pushlstring(state, path.data(), path.size());
	return 1;
}

static int lfs_chdir(lua_State *state) {
	std::error_code error;
	const char *path = luaL_checkstring(state, 1);

	fs::current_path(path, error);
	if (error)
		return lfsError(state, error, path);
	lua_pushboolean(state, 1);
	return 1;
}

static int lfs_mkdir(lua_State *state) {
	std::error_code error;
	const char *path = luaL_checkstring(state, 1);

	if (!fs::create_directory(path, error) && !error)
		error = std::make_error_code(std::errc::file_exists);
	if (error)
		return lfsError(state, error, path);
	lua_pushboolean(state, 1);
	return 1;
}

static int lfs_rmdir(lua_State *state) {
	std::error_code error;
	const char *path = luaL_checkstring(state, 1);

	if (!fs::is_directory(path, error))
		return lfsError(state, std::make_error_code(std::errc::not_a_directory), path);
	if (!fs::remove(path, error) || error)
		return lfsError(state, error ? error : std::make_error_code(std::errc::directory_not_empty), path);
	lua_pushboolean(state, 1);
	return 1;
}

// lfs.dir(path): an iterator over the names in the directory, "." and ".." first (as readdir gives them).
static int dirIterator(lua_State *state) {
	int index = (int)lua_tointeger(state, lua_upvalueindex(2));

	lua_rawgeti(state, lua_upvalueindex(1), index);
	if (lua_isnil(state, -1))
		return 0;

	lua_pushinteger(state, index + 1);
	lua_replace(state, lua_upvalueindex(2));
	return 1;
}

static int lfs_dir(lua_State *state) {
	const char *path = luaL_checkstring(state, 1);
	std::error_code error;

	if (!fs::is_directory(path, error))
		return luaL_error(state, "cannot open %s: %s", path, "No such file or directory");

	lua_newtable(state);
	int count = 0;

	lua_pushstring(state, ".");
	lua_rawseti(state, -2, ++count);
	lua_pushstring(state, "..");
	lua_rawseti(state, -2, ++count);
	for (fs::directory_iterator it(path, error), end; !error && it != end; it.increment(error)) {
		std::string name = it->path().filename().string();

		lua_pushlstring(state, name.data(), name.size());
		lua_rawseti(state, -2, ++count);
	}
	lua_pushinteger(state, 1);
	lua_pushcclosure(state, dirIterator, 2);
	return 1;
}

static int lfs_attributes(lua_State *state) {
	const char *path = luaL_checkstring(state, 1);
	std::error_code error;
	fs::file_status status = fs::status(path, error);

	if (error || status.type() == fs::file_type::not_found)
		return lfsError(state, error ? error : std::make_error_code(std::errc::no_such_file_or_directory), path);

	const char *mode = "other";

	switch (status.type()) {
	case fs::file_type::regular:
		mode = "file";
		break;
	case fs::file_type::directory:
		mode = "directory";
		break;
	case fs::file_type::symlink:
		mode = "link";
		break;
	case fs::file_type::character:
		mode = "char device";
		break;
	case fs::file_type::block:
		mode = "block device";
		break;
	case fs::file_type::fifo:
		mode = "named pipe";
		break;
	case fs::file_type::socket:
		mode = "socket";
		break;
	default:
		break;
	}

	long long modification = 0;
	unsigned long long size = 0;

	fs::file_time_type time = fs::last_write_time(path, error);
	if (!error) {
		// (seconds since 1970: the clock of the file system is converted through the system clock)
		auto system = std::chrono::time_point_cast<std::chrono::system_clock::duration>(
			time - fs::file_time_type::clock::now() + std::chrono::system_clock::now());

		modification = std::chrono::duration_cast<std::chrono::seconds>(system.time_since_epoch()).count();
	}
	if (status.type() == fs::file_type::regular) {
		size = fs::file_size(path, error);
		if (error)
			size = 0;
	}

	int perms = (int)status.permissions();
	std::string permissions;

	permissions += (perms & 0400) ? 'r' : '-';
	permissions += (perms & 0200) ? 'w' : '-';
	permissions += (perms & 0100) ? 'x' : '-';
	permissions += (perms & 0040) ? 'r' : '-';
	permissions += (perms & 0020) ? 'w' : '-';
	permissions += (perms & 0010) ? 'x' : '-';
	permissions += (perms & 0004) ? 'r' : '-';
	permissions += (perms & 0002) ? 'w' : '-';
	permissions += (perms & 0001) ? 'x' : '-';

	struct Entry {
		const char *name;
		bool isString;
		long long number;
		std::string text;
	};
	std::vector<Entry> entries = {
		{"dev", false, 0, ""},
		{"ino", false, 0, ""},
		{"mode", true, 0, mode},
		{"nlink", false, 1, ""},
		{"uid", false, 0, ""},
		{"gid", false, 0, ""},
		{"rdev", false, 0, ""},
		{"access", false, modification, ""},
		{"modification", false, modification, ""},
		{"change", false, modification, ""},
		{"size", false, (long long)size, ""},
		{"permissions", true, 0, permissions},
	};

	if (lua_isstring(state, 2)) {
		const char *wanted = lua_tostring(state, 2);

		for (const Entry &entry : entries) {
			if (std::strcmp(entry.name, wanted) == 0) {
				if (entry.isString)
					lua_pushlstring(state, entry.text.data(), entry.text.size());
				else
					lua_pushnumber(state, (lua_Number)entry.number);
				return 1;
			}
		}
		return luaL_error(state, "invalid attribute name '%s'", wanted);
	}

	lua_newtable(state);
	for (const Entry &entry : entries) {
		if (entry.isString)
			lua_pushlstring(state, entry.text.data(), entry.text.size());
		else
			lua_pushnumber(state, (lua_Number)entry.number);
		lua_setfield(state, -2, entry.name);
	}
	return 1;
}

static int lfs_touch(lua_State *state) {
	std::error_code error;
	const char *path = luaL_checkstring(state, 1);

	fs::last_write_time(path, fs::file_time_type::clock::now(), error);
	if (error)
		return lfsError(state, error, path);
	lua_pushboolean(state, 1);
	return 1;
}

static const luaL_Reg lfsFunctions[] = {
	{"attributes", lfs_attributes},
	{"chdir", lfs_chdir},
	{"currentdir", lfs_currentdir},
	{"dir", lfs_dir},
	{"mkdir", lfs_mkdir},
	{"rmdir", lfs_rmdir},
	{"symlinkattributes", lfs_attributes},
	{"touch", lfs_touch},
	{nullptr, nullptr}
};

static int openLfs(lua_State *state) {
	luaL_newlib(state, lfsFunctions);
	lua_pushliteral(state, "LuaFileSystem 1.6.3, a library developed to complement the set of functions related to file systems offered by the standard Lua distribution");
	lua_setfield(state, -2, "_DESCRIPTION");
	lua_pushliteral(state, "LuaFileSystem 1.6.3");
	lua_setfield(state, -2, "_VERSION");
	return 1;
}


// ---------------------------------------------------------------------------------------------------------------------
// utf8
//
// The original has the library `utf8` built in (luaopen_utf8, asm 284419: the 25 functions of lua-utf8, from `offset` to
// `match`, and `charpattern`). That the table is there before the scripts run matters: the globals that exist then are
// the ones of the engine (StoreNamesOfInternalGlobalVars) that a savegame does not save, and the game's own
// UTF8StringFix script puts its pure-Lua functions into the very same `utf8`; without the name there a new game would
// save the table without its functions and give it back empty. Here are the functions that are plain (len, sub,
// char, byte, reverse, lower, upper: the places are those of the characters, from 1, and negative from the end).
// TODO: offset, charpos, next, codes, codepoint, remove, insert, escape, title, fold, ncasecmp, width, widthindex and
// the four that work with patterns (find, match, gmatch, gsub) are not done; the scripts replace them all anyway.

/** The code point at the start of s (up to `end`) and the bytes it takes; a byte that does not start a sequence is itself. */
static unsigned decodeUtf8(const char *s, const char *end, int &size) {
	unsigned char c = static_cast<unsigned char>(*s);
	int more = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : (c >= 0xC0) ? 1 : 0;
	unsigned code = (more == 0) ? c : (c & (0x3F >> more));

	size = 1;
	for (int i = 1; i <= more; i++) {
		if (s + i >= end || (static_cast<unsigned char>(s[i]) & 0xC0) != 0x80) {
			size = 1;
			return c;
		}
		code = (code << 6) | (static_cast<unsigned char>(s[i]) & 0x3F);
	}
	size = more + 1;
	return code;
}

static void appendUtf8(std::string &out, unsigned code) {
	if (code < 0x80) {
		out += static_cast<char>(code);
	} else if (code < 0x800) {
		out += static_cast<char>(0xC0 | (code >> 6));
		out += static_cast<char>(0x80 | (code & 0x3F));
	} else if (code < 0x10000) {
		out += static_cast<char>(0xE0 | (code >> 12));
		out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
		out += static_cast<char>(0x80 | (code & 0x3F));
	} else {
		out += static_cast<char>(0xF0 | (code >> 18));
		out += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
		out += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
		out += static_cast<char>(0x80 | (code & 0x3F));
	}
}

static std::vector<unsigned> decodeAll(const char *s, size_t length) {
	std::vector<unsigned> codes;
	const char *end = s + length;

	while (s < end) {
		int size;

		codes.push_back(decodeUtf8(s, end, size));
		s += size;
	}
	return codes;
}

static std::string encodeAll(const std::vector<unsigned> &codes, size_t from, size_t to) {
	std::string out;

	for (size_t i = from; i < to; i++)
		appendUtf8(out, codes[i]);
	return out;
}

/** A place that counts from the end when negative, kept inside 0..count+1 (as Lua's string.sub does). */
static long utf8Place(long place, long count) {
	if (place < 0)
		place = count + place + 1;
	if (place < 0)
		place = 0;
	return place;
}

static int utf8_len(lua_State *state) {
	size_t length;
	const char *s = luaL_checklstring(state, 1, &length);
	std::vector<unsigned> codes = decodeAll(s, length);
	long count = static_cast<long>(codes.size());
	long from = utf8Place(luaL_optinteger(state, 2, 1), count);
	long to = utf8Place(luaL_optinteger(state, 3, -1), count);

	if (from < 1)
		from = 1;
	if (to > count)
		to = count;
	lua_pushinteger(state, to >= from ? to - from + 1 : 0);
	return 1;
}

static int utf8_sub(lua_State *state) {
	size_t length;
	const char *s = luaL_checklstring(state, 1, &length);
	std::vector<unsigned> codes = decodeAll(s, length);
	long count = static_cast<long>(codes.size());
	long from = utf8Place(luaL_optinteger(state, 2, 1), count);
	long to = utf8Place(luaL_optinteger(state, 3, -1), count);

	if (from < 1)
		from = 1;
	if (to > count)
		to = count;
	std::string result = (from <= to) ? encodeAll(codes, from - 1, to) : std::string();

	lua_pushlstring(state, result.data(), result.size());
	return 1;
}

static int utf8_char(lua_State *state) {
	int count = lua_gettop(state);
	std::string result;

	for (int i = 1; i <= count; i++)
		appendUtf8(result, static_cast<unsigned>(luaL_checkinteger(state, i)));
	lua_pushlstring(state, result.data(), result.size());
	return 1;
}

static int utf8_byte(lua_State *state) {
	size_t length;
	const char *s = luaL_checklstring(state, 1, &length);
	std::vector<unsigned> codes = decodeAll(s, length);
	long count = static_cast<long>(codes.size());
	long from = utf8Place(luaL_optinteger(state, 2, 1), count);
	long to = utf8Place(luaL_optinteger(state, 3, from), count);

	if (from < 1)
		from = 1;
	if (to > count)
		to = count;
	for (long i = from; i <= to; i++)
		lua_pushinteger(state, codes[i - 1]);
	return (to >= from) ? static_cast<int>(to - from + 1) : 0;
}

static int utf8_reverse(lua_State *state) {
	size_t length;
	const char *s = luaL_checklstring(state, 1, &length);
	std::vector<unsigned> codes = decodeAll(s, length);
	std::string result;

	for (size_t i = codes.size(); i > 0; i--)
		appendUtf8(result, codes[i - 1]);
	lua_pushlstring(state, result.data(), result.size());
	return 1;
}

/** The case of a code point: ASCII, Latin-1, Latin Extended-A, Greek and Cyrillic (the original has the whole table of Unicode). */
static unsigned toLowerCode(unsigned code) {
	if (code < 0x80)
		return (code >= 'A' && code <= 'Z') ? code + 32 : code;
	if ((code >= 0xC0 && code <= 0xDE && code != 0xD7) || (code >= 0x391 && code <= 0x3A9) || (code >= 0x410 && code <= 0x42F))
		return code + 32;
	if (code >= 0x400 && code <= 0x40F)
		return code + 80;
	if (code >= 0x100 && code <= 0x17F && code != 0x130 && code != 0x131 && code != 0x138 && code != 0x149 && code != 0x178 &&
	        code != 0x17F) {
		bool oddIsUpper = (code >= 0x139 && code <= 0x148) || (code >= 0x179 && code <= 0x17E);

		return ((code & 1) == (oddIsUpper ? 1 : 0)) ? code + 1 : code;
	}
	return code;
}

static unsigned toUpperCode(unsigned code) {
	if (code < 0x80)
		return (code >= 'a' && code <= 'z') ? code - 32 : code;
	if ((code >= 0xE0 && code <= 0xFE && code != 0xF7) || (code >= 0x3B1 && code <= 0x3C9 && code != 0x3C2) ||
	        (code >= 0x430 && code <= 0x44F))
		return code - 32;
	if (code >= 0x450 && code <= 0x45F)
		return code - 80;
	if (code == 0xFF)
		return 0x178;
	if (code >= 0x100 && code <= 0x17F && code != 0x130 && code != 0x131 && code != 0x138 && code != 0x149 && code != 0x178 &&
	        code != 0x17F) {
		bool oddIsUpper = (code >= 0x139 && code <= 0x148) || (code >= 0x179 && code <= 0x17E);

		return ((code & 1) == (oddIsUpper ? 0 : 1)) ? code - 1 : code;
	}
	return code;
}

static int utf8Case(lua_State *state, bool upper) {
	size_t length;
	const char *s = luaL_checklstring(state, 1, &length);
	std::vector<unsigned> codes = decodeAll(s, length);

	for (unsigned &code : codes) {
		code = upper ? toUpperCode(code) : toLowerCode(code);
	}
	std::string result = encodeAll(codes, 0, codes.size());

	lua_pushlstring(state, result.data(), result.size());
	return 1;
}

static int utf8_lower(lua_State *state) {
	return utf8Case(state, false);
}

static int utf8_upper(lua_State *state) {
	return utf8Case(state, true);
}

static const luaL_Reg utf8Extras[] = {
	{"sub", utf8_sub},
	{"byte", utf8_byte},
	{"reverse", utf8_reverse},
	{"lower", utf8_lower},
	{"upper", utf8_upper},
	{nullptr, nullptr}
};

void OpenLuaStdLibs(lua_State *state) {
	// utf8: the original has the lua-utf8 library, which has the functions of Lua 5.4's own and more; here Lua 5.4's utf8 gets the plain
	// functions of lua-utf8 that it has not (the others of lua-utf8 are in the TODO above)
	lua_getglobal(state, "utf8");

	if (lua_istable(state, -1))
		luaL_setfuncs(state, utf8Extras, 0);
	lua_settop(state, -2);

	// lfs is built in (luaopen_lfs, asm 5a73c0): a global, and in package.loaded for require("lfs")
	luaL_requiref(state, "lfs", openLfs, 1);
	lua_settop(state, -2);
}

#include "vscommon/scripting/luaStdLibs.h"

#include <cerrno>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctime>
#include <filesystem>
#include <string>
#include <vector>

#ifndef FORBIDDEN_SYMBOL_ALLOW_ALL
	#define FORBIDDEN_SYMBOL_ALLOW_ALL
#endif
#include "common/lua/lauxlib.h"
#include "common/lua/lua.h"

#ifdef _WIN32
	#include <direct.h>
	#include <io.h>
#else
	#include <sys/stat.h>
	#include <unistd.h>
#endif

namespace fs = std::filesystem;

// ---------------------------------------------------------------------------------------------------------------------
// io

static const char *const kFileHandle = "FILE*";

struct LuaFile {
	FILE *file;
	bool closable;
};

static int pushResult(lua_State *state, bool ok, const char *name) {
	if (ok) {
		lua_pushboolean(state, 1);
		return 1;
	}

	int error = errno;

	lua_pushnil(state);
	if (name)
		lua_pushfstring(state, "%s: %s", name, std::strerror(error));
	else
		lua_pushstring(state, std::strerror(error));
	lua_pushinteger(state, error);
	return 3;
}

static LuaFile *toLuaFile(lua_State *state, int index) {
	return static_cast<LuaFile *>(luaL_checkudata(state, index, kFileHandle));
}

static FILE *toOpenFile(lua_State *state, int index) {
	LuaFile *file = toLuaFile(state, index);

	if (!file->file)
		luaL_error(state, "attempt to use a closed file");
	return file->file;
}

static LuaFile *newFile(lua_State *state, FILE *file, bool closable) {
	LuaFile *result = static_cast<LuaFile *>(lua_newuserdata(state, sizeof(LuaFile)));

	result->file = file;
	result->closable = closable;
	luaL_getmetatable(state, kFileHandle);
	lua_setmetatable(state, -2);
	return result;
}

static int closeFile(lua_State *state, LuaFile *file) {
	if (!file->file)
		return 0;

	FILE *handle = file->file;

	file->file = nullptr;
	if (!file->closable) {
		// (the standard files stay open)
		lua_pushnil(state);
		lua_pushstring(state, "cannot close standard file");
		return 2;
	}
	return pushResult(state, std::fclose(handle) == 0, nullptr);
}

static int io_fclose(lua_State *state) {
	LuaFile *file = toLuaFile(state, 1);

	toOpenFile(state, 1);
	return closeFile(state, file);
}

static int io_gc(lua_State *state) {
	LuaFile *file = toLuaFile(state, 1);

	if (file->file && file->closable) {
		std::fclose(file->file);
		file->file = nullptr;
	}
	return 0;
}

static int io_tostring(lua_State *state) {
	LuaFile *file = toLuaFile(state, 1);

	if (!file->file)
		lua_pushliteral(state, "file (closed)");
	else
		lua_pushfstring(state, "file (%p)", file->file);
	return 1;
}

static int readNumber(lua_State *state, FILE *file) {
	double value;

	if (std::fscanf(file, "%lf", &value) == 1) {
		lua_pushnumber(state, value);
		return 1;
	}
	return 0;
}

static int readLine(lua_State *state, FILE *file, bool keepNewLine) {
	std::string line;
	int c;

	while ((c = std::fgetc(file)) != EOF) {
		if (c == '\n') {
			if (keepNewLine)
				line += '\n';
			lua_pushlstring(state, line.data(), line.size());
			return 1;
		}
		line += (char)c;
	}
	if (line.empty())
		return 0;
	lua_pushlstring(state, line.data(), line.size());
	return 1;
}

static int readChars(lua_State *state, FILE *file, size_t count) {
	std::string buffer(count, '\0');
	size_t got = std::fread(&buffer[0], 1, count, file);

	if (got == 0 && count > 0)
		return 0;
	lua_pushlstring(state, buffer.data(), got);
	return 1;
}

static int readAll(lua_State *state, FILE *file) {
	std::string result;
	char block[4096];
	size_t got;

	while ((got = std::fread(block, 1, sizeof(block), file)) > 0)
		result.append(block, got);
	lua_pushlstring(state, result.data(), result.size());
	return 1;
}

// The formats of read(): a number (that many bytes), "*n", "*l", "*L", "*a" (the star is optional in later versions).
static int readFormats(lua_State *state, FILE *file, int first) {
	int top = lua_gettop(state);
	int results = 0;

	std::clearerr(file);
	if (top < first) {
		results = readLine(state, file, false);
		if (!results)
			lua_pushnil(state);
		return 1;
	}

	for (int arg = first; arg <= top; arg++) {
		int ok;

		if (lua_type(state, arg) == LUA_TNUMBER) {
			ok = readChars(state, file, (size_t)lua_tointeger(state, arg));
		} else {
			const char *format = luaL_checkstring(state, arg);

			if (*format == '*')
				format++;
			switch (*format) {
			case 'n':
				ok = readNumber(state, file);
				break;
			case 'l':
				ok = readLine(state, file, false);
				break;
			case 'L':
				ok = readLine(state, file, true);
				break;
			case 'a':
				ok = readAll(state, file);
				break;
			default:
				return luaL_argerror(state, arg, "invalid format");
			}
		}
		results++;
		if (!ok) {
			lua_pushnil(state);
			break;
		}
	}
	if (std::ferror(file))
		return pushResult(state, false, nullptr);
	return results;
}

static int writeValues(lua_State *state, FILE *file, int first) {
	int top = lua_gettop(state);
	bool ok = true;

	for (int arg = first; arg <= top; arg++) {
		if (lua_type(state, arg) == LUA_TNUMBER) {
			ok = ok && std::fprintf(file, "%.14g", lua_tonumber(state, arg)) > 0;
		} else {
			size_t length;
			const char *text = luaL_checklstring(state, arg, &length);

			ok = ok && std::fwrite(text, 1, length, file) == length;
		}
	}
	return ok;
}

static FILE *g_defaultInput = nullptr;
static FILE *g_defaultOutput = nullptr;

static int io_open(lua_State *state) {
	const char *name = luaL_checkstring(state, 1);
	const char *mode = luaL_optstring(state, 2, "r");
	std::string checked(mode);

	// (as on Linux, where there is no text mode: a file is always binary)
	if (checked.empty() || !std::strchr("rwa", checked[0]))
		return luaL_argerror(state, 2, "invalid mode");
	if (checked.find('b') == std::string::npos)
		checked += 'b';

	FILE *file = std::fopen(name, checked.c_str());

	if (!file)
		return pushResult(state, false, name);
	newFile(state, file, true);
	return 1;
}

static int io_close(lua_State *state) {
	if (lua_isnone(state, 1)) {
		// (closes the default output)
		lua_pushnil(state);
		lua_pushstring(state, "no default output file to close");
		return 2;
	}
	return io_fclose(state);
}

static int io_type(lua_State *state) {
	luaL_checkany(state, 1);
	void *file = lua_touserdata(state, 1);

	lua_getmetatable(state, 1);
	luaL_getmetatable(state, kFileHandle);
	if (!file || !lua_rawequal(state, -2, -1)) {
		lua_pushnil(state);
	} else if (static_cast<LuaFile *>(file)->file) {
		lua_pushliteral(state, "file");
	} else {
		lua_pushliteral(state, "closed file");
	}
	return 1;
}

static int f_read(lua_State *state) {
	return readFormats(state, toOpenFile(state, 1), 2);
}

static int io_read(lua_State *state) {
	return readFormats(state, g_defaultInput ? g_defaultInput : stdin, 1);
}

static int f_write(lua_State *state) {
	FILE *file = toOpenFile(state, 1);

	if (writeValues(state, file, 2)) {
		lua_pushvalue(state, 1);
		return 1;
	}
	return pushResult(state, false, nullptr);
}

static int io_write(lua_State *state) {
	FILE *file = g_defaultOutput ? g_defaultOutput : stdout;

	if (writeValues(state, file, 1)) {
		lua_pushboolean(state, 1);
		return 1;
	}
	return pushResult(state, false, nullptr);
}

static int f_seek(lua_State *state) {
	static const int modes[] = {SEEK_SET, SEEK_CUR, SEEK_END};
	static const char *const names[] = {"set", "cur", "end", nullptr};
	FILE *file = toOpenFile(state, 1);
	int mode = luaL_checkoption(state, 2, "cur", names);
	long offset = (long)luaL_optnumber(state, 3, 0);

	if (std::fseek(file, offset, modes[mode]) != 0)
		return pushResult(state, false, nullptr);
	lua_pushnumber(state, (lua_Number)std::ftell(file));
	return 1;
}

static int f_flush(lua_State *state) {
	return pushResult(state, std::fflush(toOpenFile(state, 1)) == 0, nullptr);
}

static int io_flush(lua_State *state) {
	return pushResult(state, std::fflush(g_defaultOutput ? g_defaultOutput : stdout) == 0, nullptr);
}

static int f_setvbuf(lua_State *state) {
	static const int modes[] = {_IONBF, _IOFBF, _IOLBF};
	static const char *const names[] = {"no", "full", "line", nullptr};
	FILE *file = toOpenFile(state, 1);
	int mode = luaL_checkoption(state, 2, nullptr, names);
	size_t size = (size_t)luaL_optinteger(state, 3, BUFSIZ);

	return pushResult(state, std::setvbuf(file, nullptr, modes[mode], size) == 0, nullptr);
}

// The iterator of lines(): the upvalues are the file, whether to close it at the end and the formats.
static int linesIterator(lua_State *state) {
	LuaFile *file = static_cast<LuaFile *>(lua_touserdata(state, lua_upvalueindex(1)));

	if (!file->file)
		return luaL_error(state, "file is already closed");

	lua_settop(state, 0);
	int formats = (int)lua_tointeger(state, lua_upvalueindex(2));

	for (int i = 1; i <= formats; i++)
		lua_pushvalue(state, lua_upvalueindex(2 + i));

	int results = readFormats(state, file->file, 1);

	if (lua_toboolean(state, -results))
		return results;

	if (lua_toboolean(state, lua_upvalueindex(2 + formats + 1))) {
		lua_settop(state, 0);
		lua_pushvalue(state, lua_upvalueindex(1));
		closeFile(state, file);
	}
	return 0;
}

static int f_lines(lua_State *state) {
	toOpenFile(state, 1);
	// upvalues: file, formats count, formats..., close flag
	int formats = lua_gettop(state) - 1;

	lua_pushvalue(state, 1);
	lua_pushinteger(state, formats);
	for (int i = 0; i < formats; i++)
		lua_pushvalue(state, 2 + i);
	lua_pushboolean(state, 0);
	lua_pushcclosure(state, linesIterator, 2 + formats + 1);
	return 1;
}

static int io_lines(lua_State *state) {
	if (lua_isnoneornil(state, 1)) {
		LuaFile *input = newFile(state, g_defaultInput ? g_defaultInput : stdin, false);

		(void)input;
		lua_replace(state, 1);
		return f_lines(state);
	}

	const char *name = luaL_checkstring(state, 1);
	FILE *file = std::fopen(name, "rb");

	if (!file)
		return luaL_error(state, "%s: %s", name, std::strerror(errno));

	int formats = lua_gettop(state) - 1;

	newFile(state, file, true);
	int fileIndex = lua_gettop(state);

	lua_pushvalue(state, fileIndex);
	lua_pushinteger(state, formats);
	for (int i = 0; i < formats; i++)
		lua_pushvalue(state, 2 + i);
	lua_pushboolean(state, 1);
	lua_pushcclosure(state, linesIterator, 2 + formats + 1);
	return 1;
}

static int io_output(lua_State *state) {
	if (!lua_isnoneornil(state, 1)) {
		if (lua_isstring(state, 1)) {
			FILE *file = std::fopen(lua_tostring(state, 1), "wb");

			if (!file)
				return luaL_error(state, "%s: %s", lua_tostring(state, 1), std::strerror(errno));
			newFile(state, file, true);
			g_defaultOutput = static_cast<LuaFile *>(lua_touserdata(state, -1))->file;
		} else {
			g_defaultOutput = toOpenFile(state, 1);
			lua_pushvalue(state, 1);
		}
		return 1;
	}
	newFile(state, g_defaultOutput ? g_defaultOutput : stdout, false);
	return 1;
}

static int io_input(lua_State *state) {
	if (!lua_isnoneornil(state, 1)) {
		if (lua_isstring(state, 1)) {
			FILE *file = std::fopen(lua_tostring(state, 1), "rb");

			if (!file)
				return luaL_error(state, "%s: %s", lua_tostring(state, 1), std::strerror(errno));
			newFile(state, file, true);
			g_defaultInput = static_cast<LuaFile *>(lua_touserdata(state, -1))->file;
		} else {
			g_defaultInput = toOpenFile(state, 1);
			lua_pushvalue(state, 1);
		}
		return 1;
	}
	newFile(state, g_defaultInput ? g_defaultInput : stdin, false);
	return 1;
}

static const luaL_Reg ioFunctions[] = {
	{"close", io_close},
	{"flush", io_flush},
	{"input", io_input},
	{"lines", io_lines},
	{"open", io_open},
	{"output", io_output},
	{"read", io_read},
	{"type", io_type},
	{"write", io_write},
	{nullptr, nullptr}
};

static const luaL_Reg fileMethods[] = {
	{"close", io_fclose},
	{"flush", f_flush},
	{"lines", f_lines},
	{"read", f_read},
	{"seek", f_seek},
	{"setvbuf", f_setvbuf},
	{"write", f_write},
	{"__gc", io_gc},
	{"__tostring", io_tostring},
	{nullptr, nullptr}
};

static int openIo(lua_State *state) {
	luaL_newmetatable(state, kFileHandle);
	lua_pushvalue(state, -1);
	lua_setfield(state, -2, "__index");
	luaL_register(state, nullptr, fileMethods);
	lua_pop(state, 1);

	luaL_register(state, "io", ioFunctions);

	newFile(state, stdin, false);
	lua_setfield(state, -2, "stdin");
	newFile(state, stdout, false);
	lua_setfield(state, -2, "stdout");
	newFile(state, stderr, false);
	lua_setfield(state, -2, "stderr");
	return 1;
}

// ---------------------------------------------------------------------------------------------------------------------
// os

static int os_clock(lua_State *state) {
	lua_pushnumber(state, (lua_Number)std::clock() / (lua_Number)CLOCKS_PER_SEC);
	return 1;
}

static void setField(lua_State *state, const char *key, int value) {
	lua_pushinteger(state, value);
	lua_setfield(state, -2, key);
}

static void setBoolField(lua_State *state, const char *key, int value) {
	if (value < 0)
		return;
	lua_pushboolean(state, value);
	lua_setfield(state, -2, key);
}

static int getField(lua_State *state, const char *key, int fallback) {
	lua_getfield(state, -1, key);
	int result = fallback;

	if (lua_isnumber(state, -1)) {
		result = (int)lua_tointeger(state, -1);
	} else if (fallback < 0) {
		return luaL_error(state, "field '%s' missing in date table", key);
	}
	lua_pop(state, 1);
	return result;
}

static int os_date(lua_State *state) {
	const char *format = luaL_optstring(state, 1, "%c");
	time_t when = lua_isnoneornil(state, 2) ? std::time(nullptr) : (time_t)luaL_checknumber(state, 2);
	struct tm *date;
	struct tm copy;

	if (*format == '!') {
		date = std::gmtime(&when);
		format++;
	} else {
		date = std::localtime(&when);
	}

	if (!date) {
		lua_pushnil(state);
		return 1;
	}
	copy = *date;

	if (std::strcmp(format, "*t") == 0) {
		lua_createtable(state, 0, 9);
		setField(state, "sec", copy.tm_sec);
		setField(state, "min", copy.tm_min);
		setField(state, "hour", copy.tm_hour);
		setField(state, "day", copy.tm_mday);
		setField(state, "month", copy.tm_mon + 1);
		setField(state, "year", copy.tm_year + 1900);
		setField(state, "wday", copy.tm_wday + 1);
		setField(state, "yday", copy.tm_yday + 1);
		setBoolField(state, "isdst", copy.tm_isdst);
		return 1;
	}

	std::string result;
	char piece[256];

	for (const char *p = format; *p; p++) {
		if (*p != '%') {
			result += *p;
			continue;
		}
		p++;
		if (!*p)
			break;

		char spec[3] = {'%', *p, 0};

		if (!std::strchr("aAbBcdHIjmMpSUwWxXyYZ%", *p))
			return luaL_error(state, "invalid conversion specifier '%%%s'", spec + 1);
		size_t length = std::strftime(piece, sizeof(piece), spec, &copy);
		result.append(piece, length);
	}
	lua_pushlstring(state, result.data(), result.size());
	return 1;
}

static int os_time(lua_State *state) {
	time_t result;

	if (lua_isnoneornil(state, 1)) {
		result = std::time(nullptr);
	} else {
		struct tm date;

		luaL_checktype(state, 1, LUA_TTABLE);
		lua_settop(state, 1);
		date.tm_year = getField(state, "year", -1) - 1900;
		date.tm_mon = getField(state, "month", -1) - 1;
		date.tm_mday = getField(state, "day", -1);
		date.tm_hour = getField(state, "hour", 12);
		date.tm_min = getField(state, "min", 0);
		date.tm_sec = getField(state, "sec", 0);
		lua_getfield(state, 1, "isdst");
		date.tm_isdst = lua_isnil(state, -1) ? -1 : lua_toboolean(state, -1);
		lua_pop(state, 1);
		result = std::mktime(&date);
	}
	if (result == (time_t)-1)
		lua_pushnil(state);
	else
		lua_pushnumber(state, (lua_Number)result);
	return 1;
}

static int os_difftime(lua_State *state) {
	lua_pushnumber(state, std::difftime((time_t)luaL_checknumber(state, 1), (time_t)luaL_optnumber(state, 2, 0)));
	return 1;
}

static int os_getenv(lua_State *state) {
	lua_pushstring(state, std::getenv(luaL_checkstring(state, 1)));
	return 1;
}

static int os_remove(lua_State *state) {
	const char *name = luaL_checkstring(state, 1);

	return pushResult(state, std::remove(name) == 0, name);
}

static int os_rename(lua_State *state) {
	const char *from = luaL_checkstring(state, 1);

	return pushResult(state, std::rename(from, luaL_checkstring(state, 2)) == 0, from);
}

static int os_tmpname(lua_State *state) {
	char name[L_tmpnam];

	if (!std::tmpnam(name))
		return luaL_error(state, "unable to generate a unique filename");
	lua_pushstring(state, name);
	return 1;
}

static int os_exit(lua_State *state) {
	std::exit((int)luaL_optinteger(state, 1, EXIT_SUCCESS));
	return 0;
}

static int os_setlocale(lua_State *state) {
	const char *locale = luaL_optstring(state, 1, nullptr);

	// (the scripts do not change it)
	lua_pushstring(state, !locale || !*locale || std::strcmp(locale, "C") == 0 ? "C" : nullptr);
	return 1;
}

static int os_execute(lua_State *state) {
	// (a shell is not something the game needs: no command is run)
	if (lua_isnoneornil(state, 1)) {
		lua_pushboolean(state, 0);
		return 1;
	}
	lua_pushinteger(state, -1);
	return 1;
}

static const luaL_Reg osFunctions[] = {
	{"clock", os_clock},
	{"date", os_date},
	{"difftime", os_difftime},
	{"execute", os_execute},
	{"exit", os_exit},
	{"getenv", os_getenv},
	{"remove", os_remove},
	{"rename", os_rename},
	{"setlocale", os_setlocale},
	{"time", os_time},
	{"tmpname", os_tmpname},
	{nullptr, nullptr}
};

static int openOs(lua_State *state) {
	luaL_register(state, "os", osFunctions);
	return 1;
}

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
	luaL_register(state, "lfs", lfsFunctions);
	lua_pushliteral(state, "LuaFileSystem 1.6.3, a library developed to complement the set of functions related to file systems offered by the standard Lua distribution");
	lua_setfield(state, -2, "_DESCRIPTION");
	lua_pushliteral(state, "LuaFileSystem 1.6.3");
	lua_setfield(state, -2, "_VERSION");
	return 1;
}

// ---------------------------------------------------------------------------------------------------------------------
// package and require

// Looks in the templates of package.path for `name` (the dots are separators of directories).
static bool findInPath(lua_State *state, const char *name, const char *path, std::string &found, std::string &tried) {
	std::string module(name);

	for (char &c : module) {
		if (c == '.')
			c = '/';
	}

	std::string templates(path);
	size_t start = 0;

	while (start <= templates.size()) {
		size_t end = templates.find(';', start);

		if (end == std::string::npos)
			end = templates.size();

		std::string candidate = templates.substr(start, end - start);
		size_t mark;

		while ((mark = candidate.find('?')) != std::string::npos)
			candidate.replace(mark, 1, module);

		if (!candidate.empty()) {
			FILE *file = std::fopen(candidate.c_str(), "rb");

			if (file) {
				std::fclose(file);
				found = candidate;
				return true;
			}
			tried += "\n\tno file '" + candidate + "'";
		}
		start = end + 1;
	}
	(void)state;
	return false;
}

// The loader of a file of Lua: the file is read here (luaL_loadfile of ScummVM's Lua goes through its own file classes).
static int loadFileChunk(lua_State *state, const char *fileName) {
	FILE *file = std::fopen(fileName, "rb");

	if (!file) {
		lua_pushfstring(state, "cannot open %s", fileName);
		return LUA_ERRFILE;
	}

	std::string text;
	char block[4096];
	size_t got;

	while ((got = std::fread(block, 1, sizeof(block), file)) > 0)
		text.append(block, got);
	std::fclose(file);

	// (a first line starting with # is a comment, and a UTF-8 byte order mark is skipped)
	size_t skip = 0;

	if (text.compare(0, 3, "\xEF\xBB\xBF") == 0)
		skip = 3;
	if (text.size() > skip && text[skip] == '#') {
		while (skip < text.size() && text[skip] != '\n')
			skip++;
	}

	std::string chunkName = std::string("@") + fileName;

	return luaL_loadbuffer(state, text.data() + skip, text.size() - skip, chunkName.c_str());
}

// loadfile([name]) and dofile([name]): ScummVM's luaL_loadfile reads through its own file classes (the shim has none), so the
// two functions of the base library are replaced by ones that read the file here.
static int base_loadfile(lua_State *state) {
	const char *name = luaL_optstring(state, 1, nullptr);

	if (!name) {
		lua_pushnil(state);
		lua_pushliteral(state, "loadfile without a name is not supported");
		return 2;
	}
	if (loadFileChunk(state, name) == 0)
		return 1;

	lua_pushnil(state);
	lua_insert(state, -2);
	return 2;
}

static int base_dofile(lua_State *state) {
	const char *name = luaL_optstring(state, 1, nullptr);
	int top = lua_gettop(state);

	if (!name)
		return luaL_error(state, "dofile without a name is not supported");
	if (loadFileChunk(state, name) != 0)
		return lua_error(state);

	lua_call(state, 0, LUA_MULTRET);
	return lua_gettop(state) - top;
}

// require(name): the module is in package.loaded, or made by a loader: package.preload[name], else the file that package.path
// finds. The loader is called with the name; what it returns (or true) is kept in package.loaded.
static int require(lua_State *state) {
	const char *name = luaL_checkstring(state, 1);

	lua_settop(state, 1);
	lua_getfield(state, LUA_REGISTRYINDEX, "_LOADED");
	int loaded = lua_gettop(state);

	lua_getfield(state, loaded, name);
	if (lua_toboolean(state, -1))
		return 1;
	lua_pop(state, 1);

	lua_getfield(state, LUA_GLOBALSINDEX, "package");
	int package = lua_gettop(state);

	lua_getfield(state, package, "preload");
	lua_getfield(state, -1, name);

	if (lua_isnil(state, -1)) {
		lua_pop(state, 2);
		lua_getfield(state, package, "path");

		const char *path = lua_tostring(state, -1);
		std::string found;
		std::string tried = "\n\tno field package.preload['" + std::string(name) + "']";

		if (!path)
			return luaL_error(state, "'package.path' must be a string");
		if (!findInPath(state, name, path, found, tried))
			return luaL_error(state, "module '%s' not found:%s", name, tried.c_str());
		lua_pop(state, 1);

		if (loadFileChunk(state, found.c_str()) != 0)
			return luaL_error(state, "error loading module '%s' from file '%s':\n\t%s", name, found.c_str(),
			                  lua_tostring(state, -1));
	} else {
		lua_remove(state, -2);
	}

	// the loader is on the top
	lua_pushstring(state, name);
	lua_call(state, 1, 1);

	if (!lua_isnil(state, -1))
		lua_setfield(state, loaded, name);
	else
		lua_pop(state, 1);

	lua_getfield(state, loaded, name);
	if (lua_isnil(state, -1)) {
		lua_pop(state, 1);
		lua_pushboolean(state, 1);
		lua_pushvalue(state, -1);
		lua_setfield(state, loaded, name);
	}
	return 1;
}

static void openPackage(lua_State *state) {
	lua_newtable(state); // package
	lua_newtable(state);
	lua_setfield(state, -2, "preload");

	// package.loaded is the registry's _LOADED (the libraries that are open already are in it)
	lua_getfield(state, LUA_REGISTRYINDEX, "_LOADED");
	if (lua_isnil(state, -1)) {
		lua_pop(state, 1);
		lua_newtable(state);
		lua_pushvalue(state, -1);
		lua_setfield(state, LUA_REGISTRYINDEX, "_LOADED");
	}
	lua_setfield(state, -2, "loaded");

	lua_pushliteral(state, "./?.lua;./lua/?.lua");
	lua_setfield(state, -2, "path");
	lua_pushliteral(state, "");
	lua_setfield(state, -2, "cpath");
	lua_pushliteral(state, "/\n;\n?\n!\n-\n");
	lua_setfield(state, -2, "config");

	lua_pushvalue(state, -1);
	lua_setfield(state, LUA_GLOBALSINDEX, "package");

	lua_getfield(state, -1, "loaded");
	lua_pushvalue(state, -2);
	lua_setfield(state, -2, "package");
	lua_pop(state, 1);

	lua_pushcfunction(state, openLfs);
	lua_getfield(state, -2, "preload");
	lua_insert(state, -2);
	lua_setfield(state, -2, "lfs");
	lua_pop(state, 2);

	lua_pushcfunction(state, require);
	lua_setfield(state, LUA_GLOBALSINDEX, "require");
	lua_pushcfunction(state, base_loadfile);
	lua_setfield(state, LUA_GLOBALSINDEX, "loadfile");
	lua_pushcfunction(state, base_dofile);
	lua_setfield(state, LUA_GLOBALSINDEX, "dofile");
}

void OpenLuaStdLibs(lua_State *state) {
	lua_pushcfunction(state, openIo);
	lua_call(state, 0, 1);
	lua_pop(state, 1);

	lua_pushcfunction(state, openOs);
	lua_call(state, 0, 1);
	lua_pop(state, 1);

	openPackage(state);
}

#include <cstdio>
#include <fstream>

#include "AppGlobals.h"
#include "TStandardPaths.h"
#include "vscommon/scripting/visLua.h"
#include "vstables/visionaireGame.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static bool isTrue(const char *name) {
	lua_getfield(L, LUA_GLOBALSINDEX, name);
	bool result = lua_toboolean(L, -1) != 0;

	lua_settop(L, 0);
	return result;
}

static std::string text(const char *name) {
	lua_getfield(L, LUA_GLOBALSINDEX, name);
	const char *value = lua_tostring(L, -1);
	std::string result = value ? value : "(nil)";

	lua_settop(L, 0);
	return result;
}

// The libraries of the standard Lua that the scripts of the game use: io, os, require and lfs.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	TVisionaireGame game;

	InitLua(&game, wxString(L"."), wxString(L"."), wxString(L"."));

	// a module for require, in ./lua/
	std::ofstream("luastd_module.lua") << "local M = {}\nM.value = 42\nloaded_count = (loaded_count or 0) + 1\nreturn M\n";
	std::ofstream("luastd_nothing.lua") << "nothing_loaded = true\n";

	LuaDoString(
		"-- io: write, read, lines, seek, append\n"
		"local name = 'luastd_test.txt'\n"
		"local f = assert(io.open(name, 'w'))\n"
		"assert(io.type(f) == 'file')\n"
		"f:write('first\\n', 12, ' ', 'x\\n', 'last')\n"
		"f:close()\n"
		"assert(io.type(f) == 'closed file')\n"
		"assert(not pcall(function() return f:read('*a') end))\n"
		"f = assert(io.open(name, 'r'))\n"
		"assert(f:read('*l') == 'first')\n"
		"assert(f:read('*n') == 12)\n"
		"assert(f:read(2) == ' x')\n"
		"assert(f:read('*a') == '\\nlast')\n"
		"assert(f:read('*l') == nil)\n"
		"assert(f:seek('set', 2) == 2)\n"
		"assert(f:read(3) == 'rst')\n"
		"assert(f:seek('end') == 15)\n"
		"f:close()\n"
		"local n = 0\n"
		"for line in io.lines(name) do n = n + 1 end\n"
		"assert(n == 3)\n"
		"f = io.open(name, 'a'); f:write('\\nmore'); f:close()\n"
		"f = io.open(name, 'rb'); local all = f:read('*a'); io.close(f)\n"
		"assert(all == 'first\\n12 x\\nlast\\nmore')\n"
		"local missing, message, code = io.open('luastd_no_such_file.txt', 'r')\n"
		"assert(missing == nil and message:find('luastd_no_such_file.txt') and type(code) == 'number')\n"
		"assert(not pcall(io.open, name, 'q'))\n"
		"-- os\n"
		"assert(type(os.time()) == 'number' and os.time() > 1500000000)\n"
		"local t = os.date('*t', 86400 * 365)\n"
		"assert(t.year >= 1970 and t.month and t.day and t.hour and t.min and t.sec and t.wday and t.yday)\n"
		"assert(os.date('!%Y-%m-%d %H:%M:%S', 1000000000) == '2001-09-09 01:46:40')\n"
		"assert(os.time({year = 2001, month = 9, day = 9, hour = 12}) > 0)\n"
		"assert(os.difftime(10, 4) == 6)\n"
		"assert(type(os.clock()) == 'number')\n"
		"assert(os.getenv('LUASTD_NO_SUCH_VARIABLE') == nil)\n"
		"assert(os.rename(name, name .. '.2'))\n"
		"assert(io.open(name) == nil)\n"
		"assert(os.remove(name .. '.2'))\n"
		"local ok = os.remove(name .. '.2')\n"
		"assert(ok == nil)\n"
		"-- the scripts of the game add functions to os\n"
		"function os.isFileExist(fn) local f = io.open(fn, 'r') if f == nil then return false end f:close() return true end\n"
		"assert(os.isFileExist('luastd_module.lua') and not os.isFileExist('luastd_nope'))\n"
		"basics_ok = true\n",
		"basics");
	CHECK(isTrue("basics_ok"));

	LuaDoString(
		"-- require: a file of package.path, once; a module that returns nothing is true; the preloaded lfs\n"
		"local m = require('luastd_module')\n"
		"assert(m.value == 42)\n"
		"assert(require('luastd_module') == m)\n"
		"assert(loaded_count == 1)\n"
		"assert(package.loaded['luastd_module'] == m)\n"
		"assert(require('luastd_nothing') == true and nothing_loaded)\n"
		"local ok, err = pcall(require, 'luastd_not_there')\n"
		"assert(not ok and err:find('luastd_not_there') and err:find('no file'))\n"
		"package.preload['luastd_pre'] = function(name) return {name = name} end\n"
		"assert(require('luastd_pre').name == 'luastd_pre')\n"
		"local lfs = require('lfs')\n"
		"assert(lfs == require('lfs') and lfs.currentdir and lfs.attributes)\n"
		"require_ok = true\n",
		"require");
	CHECK(isTrue("require_ok"));

	LuaDoString(
		"-- lfs: dir, attributes, mkdir, rmdir\n"
		"local lfs = require('lfs')\n"
		"assert(type(lfs.currentdir()) == 'string' and #lfs.currentdir() > 0)\n"
		"assert(lfs.mkdir('luastd_dir'))\n"
		"assert(lfs.mkdir('luastd_dir') == nil)\n"
		"local f = io.open('luastd_dir/a.txt', 'w'); f:write('12345'); f:close()\n"
		"local a = lfs.attributes('luastd_dir/a.txt')\n"
		"assert(a.mode == 'file' and a.size == 5 and a.modification > 1500000000)\n"
		"assert(lfs.attributes('luastd_dir', 'mode') == 'directory')\n"
		"assert(lfs.attributes('luastd_dir/a.txt', 'size') == 5)\n"
		"assert(lfs.attributes('luastd_dir/none') == nil)\n"
		"local names = {}\n"
		"for name in lfs.dir('luastd_dir') do names[#names + 1] = name end\n"
		"table.sort(names)\n"
		"assert(#names == 3 and names[1] == '.' and names[2] == '..' and names[3] == 'a.txt')\n"
		"assert(not pcall(lfs.dir, 'luastd_no_dir'))\n"
		"assert(lfs.rmdir('luastd_dir') == nil)\n"
		"os.remove('luastd_dir/a.txt')\n"
		"assert(lfs.rmdir('luastd_dir'))\n"
		"assert(lfs.attributes('luastd_dir') == nil)\n"
		"lfs_ok = true\n",
		"lfs");
	CHECK(isTrue("lfs_ok"));

	std::remove("luastd_module.lua");
	std::remove("luastd_nothing.lua");

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

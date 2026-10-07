// What ScummVM's Lua (common/lua) needs from the rest of ScummVM, for building the reconstructed
// engine on its own (tools/pbuild.py). Inside ScummVM none of this is used. Only the core of Lua
// is built (no io, os, package and persistence libraries), so the library functions that open
// files are never called.
#include <cstdio>
#include <cstdlib>
#include <cstdarg>
#include <cctype>

struct lua_State;
int luaopen_io(lua_State *) { return 0; }
int luaopen_os(lua_State *) { return 0; }
int luaopen_package(lua_State *) { return 0; }

namespace Common {
bool isAlnum(int c) { return isalnum(c) != 0; }
bool isAlpha(int c) { return isalpha(c) != 0; }
bool isCntrl(int c) { return iscntrl(c) != 0; }
bool isDigit(int c) { return isdigit(c) != 0; }
bool isLower(int c) { return islower(c) != 0; }
bool isPunct(int c) { return ispunct(c) != 0; }
bool isSpace(int c) { return isspace(c) != 0; }
bool isUpper(int c) { return isupper(c) != 0; }
bool isXDigit(int c) { return isxdigit(c) != 0; }
} // namespace Common

void error(const char *s, ...) {
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	va_end(args);
	fputc('\n', stderr);
	exit(1);
}

void warning(const char *s, ...) {
	va_list args;
	va_start(args, s);
	vfprintf(stderr, s, args);
	va_end(args);
	fputc('\n', stderr);
}

// luaL_loadfile() (never called here) makes a Common::String and opens the file through ScummVM's
// file proxy; the symbols only have to exist.
#if defined(_WIN64) || !defined(_WIN32)
#define SYM(name) name
#else
#define SYM(name) "_" name
#endif
__asm__(".text\n"
        ".globl " SYM("_ZN6Common10BaseStringIcEC2EPKc") "\n" SYM("_ZN6Common10BaseStringIcEC2EPKc") ":\n ret\n"
        ".globl " SYM("_ZN6Common10BaseStringIcED2Ev") "\n" SYM("_ZN6Common10BaseStringIcED2Ev") ":\n ret\n"
        ".globl " SYM("_ZN3Lua12LuaFileProxy6createERKN6Common6StringES4_") "\n" SYM("_ZN3Lua12LuaFileProxy6createERKN6Common6StringES4_") ":\n xor %eax, %eax\n ret\n");

// Confirmed (Deponia_Linux.asm lines 286098-289015): the global `system` of the scripts (luaSystem.cpp).
#pragma once

#include "vscommon/scripting/visLua.h"

/** Makes the global `system` (metatable "Visionaire.TSystem"). */
void lua_open_system(lua_State *state);
/** The `__gc` of the objects of the scripts that need none: it does nothing (asm 438459, shared by `system`,
 *  `graphics`, `steam` ...). */
int graphics_gc(lua_State *state);

/** The memory of the video card the textures take, in bytes (asm 144410). */
unsigned long getGPUMem();
/** The memory the process takes (asm 144468: the Linux build answers 0). */
unsigned long getProcessMem();

/** The classes of the drawing (sprite, framebuffer, buffer, movie) and the global `graphics` (luaGraphics.cpp). */
void luaopen_Graphics(lua_State *state);
void luaopen_GraphicsObject(lua_State *state);

/** The Lua side of the drawing (asm 447452): the classes of the drawing, `sha1`, `graphics`, `system`, `setDelay`. The
 *  Box2D bindings (tolua_b2_open, a third-party library) are not here. */
void InitDrawLua(lua_State *state);
int lua_sha1(lua_State *state);
int lua_setDelay(lua_State *state);

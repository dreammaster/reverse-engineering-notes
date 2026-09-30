// Not yet assert-confirmed to a specific file; stays alongside argument.h/
// id.h in vscommon/scripting/.
#pragma once

#include <cstdint>
#include <string>

// Confirmed call shape only (TGameControl::InitScripts, Deponia_Linux.asm
// lines 458450, 458610) - runs a Lua chunk; the second argument is a
// "chunk name" used in error messages (confirmed from the two call sites:
// a fixed "controller" literal for controller.lua, an id-derived string
// via IdStrStd() for game-data scripts). Not reversed beyond that call
// shape.
void LuaDoString(const std::string &code, const std::string &chunkName);
// A second, one-argument overload, confirmed distinct by its own mangled
// signature (TGameControl::Update, Deponia_Linux.asm lines 469957, 470027) -
// used for the per-frame tween/delay "name = value" and delay-by-name
// script dispatches, which don't carry a separate chunk-name argument. Not
// reversed beyond that call shape.
void LuaDoString(const std::string &code);
// Confirmed call shape only (TGameControl::Update, Deponia_Linux.asm line
// 470535) - runs a Lua callback previously registered in the registry via a
// numeric reference (the delay-by-id counterpart to LuaDoString's delay-by-
// name path); not reversed beyond that call shape.
void LuaDoRef(int ref);

// Confirmed call shape only (TGameControl::InitScripts, asm lines
// 458599-458601): called directly on a TVisionaireObject::GetId() result.
// The real mangled signature takes a `TId const&`, but GetId() returns a
// `const std::uint8_t*` (confirmed elsewhere, e.g. PackVisId()'s callers) -
// this project doesn't yet know how TId and that packed-byte id relate (TId
// is still an empty placeholder - see its own header), so this is declared
// against the confirmed pointer type instead of guessing at that
// relationship.
std::string IdStrStd(const std::uint8_t *id);

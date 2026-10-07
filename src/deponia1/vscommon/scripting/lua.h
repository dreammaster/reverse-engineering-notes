// Not yet assert-confirmed to a specific file; stays alongside argument.h/
// id.h in vscommon/scripting/.
#pragma once

#include <cstdint>
#include <string>

#include "WxStub.h"
#include "vscommon/scripting/visLua.h"

class TVisionaire;
class TVisObjRef;

// Confirmed call shape only (TGameControl::InitScripts, Deponia_Linux.asm
// lines 458450, 458610) - runs a Lua chunk; the second argument is a
// "chunk name" used in error messages (confirmed from the two call sites:
// a fixed "controller" literal for controller.lua, an id-derived string
// via IdStrStd() for game-data scripts). Not reversed beyond that call
// shape.
// A second, one-argument overload, confirmed distinct by its own mangled
// signature (TGameControl::Update, Deponia_Linux.asm lines 469957, 470027) -
// used for the per-frame tween/delay "name = value" and delay-by-name
// script dispatches, which don't carry a separate chunk-name argument. Not
// reversed beyond that call shape.
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

// Confirmed call shape only (TGAnimation::Start()/ContinueAnimations()/
// HideAnimation(), Deponia_Linux.asm lines 149462, 150665, 150866) - sets the
// name the next Lua call is reported under in error messages (the animation
// hooks pass "AnimationStartedHook" / "AnimationStoppedHook"). Not reversed
// beyond that call shape.
void LuaDebugName(const char *name);
// Confirmed call shape only (same call sites) - calls the Lua function a
// registered event handler name stands for, with the object the event is
// about; not reversed beyond that call shape (same standing "Lua bridge
// contract" gap as LuaExecuteFunction elsewhere).

// Confirmed call shape only (TArgument::ConvertToObject, Deponia_Linux.asm
// line 1437560) - the game-data root associated with the current Lua state;
// not reversed beyond that call shape.
TVisionaire *GetLuaGame();

// Confirmed call shape only (TArgument::ConvertToObject/ConvertToObjectList,
// Deponia_Linux.asm lines 1437531, 1437790) - resolves a Lua-provided name
// or id string to a game-data object reference, writing it into outObject
// and returning whether it was found; not reversed beyond that call shape.
bool FindObjectByNameOrId(const wxString &nameOrId, TVisObjRef &outObject, bool flag);

// Confirmed call shape only (TGAction::Execute, Deponia_Linux.asm lines 201760-201830): makes
// the Lua global `currentAction` the action that runs a script (`ConvertToLua()` of the record and
// lua_setfield); an empty reference clears it. Not reversed beyond that call shape (the
// Lua bridge is not reconstructed).
void LuaSetCurrentAction(const TVisObjRef &action);

// Confirmed call shape only (TGameControl::Update, Deponia_Linux.asm lines 469917-470000, the
// plain-name branch of a named tween): sets the Lua global `name` to a number. Not reversed
// beyond that call shape.
void LuaSetNumber(const std::string &name, double value);

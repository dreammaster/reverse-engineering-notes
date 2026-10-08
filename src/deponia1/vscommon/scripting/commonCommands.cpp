// Confirmed (Deponia_Linux.asm lines 1429970-1431480): the two commands that both the editor and the player have,
// getObject and getTime.
#include "TTimer.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/command.h"
#include "vscommon/scripting/luaConversion.h"
#include "vscommon/scripting/visLuaObjects.h"

// getObject(path [, logWarning]): the Visionaire object of the path (see objAccess.h): `Conditions[door_open?]`
// finds the condition of that name in the whole table; `Scenes[office].SceneConditions[door_open?]` the one
// linked to the scene. The result is the empty object if there is none; a warning goes to the log if not
// found (unless `logWarning` is false).
class CmdGetObject : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxString _path;
	bool _warn = false;
};

// Confirmed (asm lines 1430740-1430860)
static void CmdGetObject_GetSyntax(TArgSyntax &syntax) {
	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kBool, false);
}

// Confirmed (asm lines 1430081-1430149)
bool CmdGetObject::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_path = argument->GetString();

	if (_parser.GetArgument(1, &argument))
		_warn = argument->GetBool();
	else
		_warn = true;

	return Redo();
}

// Confirmed (asm lines 1430038-1430081)
bool CmdGetObject::Redo() {
	TVisObjRef object;

	FindObjectByNameOrId(_path, object, _warn);
	Result.Set(object);
	Result.ToLua();
	return true;
}

// getTime([{reset = true}]): the milliseconds since the command was called the first time (or the timer was reset).
// Call it twice to measure a time.
class CmdGetTime : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	bool _reset = false;
};

// Confirmed (asm lines 1430860-1431096)
static void CmdGetTime_GetSyntax(TArgSyntax &syntax) {
	syntax.AddFlag(wxString(L"r"), wxString(L"reset"), TArgType::kBool, false);
}

// Confirmed (asm lines 1430149-1430234)
bool CmdGetTime::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetFlagArgument(wxString(L"reset"), &argument))
		_reset = argument->GetBool();
	else
		_reset = false;

	return Redo();
}

// Confirmed (asm lines 1429970-1430038)
bool CmdGetTime::Redo() {
	static TTimer timer;
	long time = timer.GetTime();

	if (_reset)
		timer.SetTime();

	Result.Set(static_cast<int>(time));
	Result.ToLua();
	return true;
}

static int lua_CmdGetObject(lua_State *state) {
	return RunCommand<CmdGetObject>(state, "getObject");
}

static int lua_CmdGetTime(lua_State *state) {
	return RunCommand<CmdGetTime>(state, "getTime");
}

// Confirmed (asm lines 1431096-1431480)
void Register_CmdGetObject() {
	RegisterCommandFunction("getObject", lua_CmdGetObject, CmdGetObject_GetSyntax);
}

void Register_CmdGetTime() {
	RegisterCommandFunction("getTime", lua_CmdGetTime, CmdGetTime_GetSyntax);
}

// Confirmed (asm lines 1431480-1431498)
void InitCommonCommands() {
	Register_CmdGetObject();
	Register_CmdGetTime();
}

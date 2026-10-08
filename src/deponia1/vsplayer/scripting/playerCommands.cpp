// Confirmed (Deponia_Linux.asm lines 388789-420390): the commands of the scripts that only the player has
// ("VisPlayerCmds" in the help of the editor), with InitPlayerCommands() (asm 425066) that registers them. The name
// of the file is not recovered. See vscommon/scripting/command.h for how a command is made and called.
#include <cstring>
#include <functional>

#include "AppGlobals.h"
#include "SdlStub.h"
#include "TGAction.h"
#include "TGameClientSDK.h"
#include "TGCharacter.h"
#include "TGObjectManager.h"
#include "TGText.h"
#include "TPolygonList.h"
#include "Easing.h"
#include "TManagedObject.h"
#include "TSoundInterface.h"
#include "vscommon/scripting/command.h"
#include "vscommon/objAccess.h"
#include "vscommon/scripting/luaConversion.h"
#include "vscommon/scripting/visLuaObjects.h"
#include "vsplayer/animationGame.h"
#include "graphicslib/graphics.h"
#include "graphicslib/picture.h"
#include "graphicslib/shader.h"
#include "zlibShim.h"
#include "vsplayer/control/gameControl.h"
#include "vsplayer/control/gameController.h"
#include "vsplayer/scripting/playerCommands.h"
#include "vstables/visionaireGame.h"

static TGameControl *gameControl() {
	return static_cast<TGameControl *>(g_pGameControl);
}

// ---------------------------------------------------------------------------------------------------------
// Actions and animations

// startAction(action): starts an action. If the action is already running, the active action that is
// running for it is given. The result is the object of the active action.
class CmdStartAction : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	TVisObjRef _action;
};

// Confirmed (asm lines 398250-398357)
static void CmdStartAction_GetSyntax(TArgSyntax &syntax) {
	syntax.AddArg(TArgType::kObject, true);
}

// Confirmed (asm lines 389936-389978)
bool CmdStartAction::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_action = argument->GetObject();

	return Redo();
}

// Confirmed (asm lines 391039-391094)
bool CmdStartAction::Redo() {
	TGAction *action = TGAction::AddRunningAction(_action);

	Result.Set(action ? action->GetActive() : TVisObjRef());
	Result.ToLua();
	return true;
}

// stopAction(action): stops a running action; nothing happens if it is not running.
class CmdStopAction : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	TVisObjRef _action;
};

// Confirmed (asm lines 398367-398470)
static void CmdStopAction_GetSyntax(TArgSyntax &syntax) {
	syntax.AddArg(TArgType::kObject, true);
}

// Confirmed (asm lines 389884-389926)
bool CmdStopAction::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_action = argument->GetObject();

	return Redo();
}

// Confirmed (asm lines 391018-391039)
bool CmdStopAction::Redo() {
	TGAction::DeleteRunningAction(_action, false);
	return true;
}

// startAnimation(animation [, reverse]): starts an animation (played backwards with `reverse`). If it is
// running already, the animation that runs is given. The result is the object of the active animation.
class CmdStartAnimation : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	TVisObjRef _animation;
	bool _reverse = false;
};

// Confirmed (asm lines 398480-398595)
static void CmdStartAnimation_GetSyntax(TArgSyntax &syntax) {
	syntax.AddArg(TArgType::kObject, true);
	syntax.AddArg(TArgType::kBool, false);
}

// Confirmed (asm lines 390036-390102)
bool CmdStartAnimation::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_animation = argument->GetObject();

	if (_parser.GetArgument(1, &argument))
		_reverse = argument->GetBool();
	else
		_reverse = false;

	return Redo();
}

// Confirmed (asm lines 390962-391018)
bool CmdStartAnimation::Redo() {
	TGAnimation *animation = TGAction::ShowAnimation(_animation, _reverse);

	Result.Set(animation ? animation->GetState() : TVisObjRef());
	Result.ToLua();
	return true;
}

// stopAnimation(animation): stops a running animation; nothing happens if it is not running.
class CmdStopAnimation : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	TVisObjRef _animation;
};

// Confirmed (asm lines 398605-398708)
static void CmdStopAnimation_GetSyntax(TArgSyntax &syntax) {
	syntax.AddArg(TArgType::kObject, true);
}

// Confirmed (asm lines 389832-389874)
bool CmdStopAnimation::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_animation = argument->GetObject();

	return Redo();
}

// Confirmed (asm lines 390942-390962)
bool CmdStopAnimation::Redo() {
	TGAction::HideAnimation(_animation);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Volume and cursor

enum {
	kMusicVolume = 0,
	kSoundVolume = 1,
	kSpeechVolume = 2,
	kMovieVolume = 3,
	kGlobalVolume = 4
};

// getVolume(type): the volume (0 to 100) of the music, the sounds, the speech, the movies or all (eMusicVolume ...
// eGlobalVolume).
class CmdGetVolume : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	unsigned int _type = 0;
};

// Confirmed (asm lines 398718-398829)
static void CmdGetVolume_GetSyntax(TArgSyntax &syntax) {
	syntax.AddArg(TArgType::kInt, true);
}

// Confirmed (asm lines 392329-392420)
bool CmdGetVolume::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_type = argument->GetInt();

	if (_type > kGlobalVolume) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Invalid sound type %d.", static_cast<int>(_type));

		return false;
	}

	return Redo();
}

// Confirmed (asm lines 390855-390942)
bool CmdGetVolume::Redo() {
	TSoundInterface *sound = gameControl()->GetSoundManager();
	int volume = -1;

	switch (_type) {
	case kMusicVolume:
		volume = sound->GetMusicVolume();
		break;
	case kSoundVolume:
		volume = sound->GetSoundVolume();
		break;
	case kSpeechVolume:
		volume = sound->GetSpeechVolume();
		break;
	case kMovieVolume:
		volume = sound->GetMovieVolume();
		break;
	case kGlobalVolume:
		volume = sound->GetGlobalVolume();
		break;
	default:
		break;
	}

	Result.Set(volume);
	Result.ToLua();
	return true;
}

// setVolume(type, volume): sets the volume (0 to 100) of one kind of sound (see getVolume).
class CmdSetVolume : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	unsigned int _type = 0;
	unsigned int _volume = 0;
};

// Confirmed (asm lines 398839-398946)
static void CmdSetVolume_GetSyntax(TArgSyntax &syntax) {
	syntax.AddArg(TArgType::kInt, true);
	syntax.AddArg(TArgType::kInt, true);
}

// Confirmed (asm lines 392190-392320)
bool CmdSetVolume::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_type = argument->GetInt();

	if (_type > kGlobalVolume) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Invalid sound type %d.", static_cast<int>(_type));

		return false;
	}

	if (_parser.GetArgument(1, &argument))
		_volume = argument->GetInt();

	if (_volume > 100) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Invalid volume %d. Volume must be between 0 and 100.", static_cast<int>(_volume));

		return false;
	}

	return Redo();
}

// Confirmed (asm lines 390745-390855): the other volumes stay as they are (-1).
bool CmdSetVolume::Redo() {
	TSoundInterface *sound = gameControl()->GetSoundManager();
	int volume = static_cast<int>(_volume);

	switch (_type) {
	case kMusicVolume:
		sound->SetVolume(volume, -1, -1, -1, -1);
		break;
	case kSoundVolume:
		sound->SetVolume(-1, volume, -1, -1, -1);
		break;
	case kSpeechVolume:
		sound->SetVolume(-1, -1, volume, -1, -1);
		break;
	case kMovieVolume:
		sound->SetVolume(-1, -1, -1, volume, -1);
		break;
	case kGlobalVolume:
		sound->SetVolume(-1, -1, -1, -1, volume);
		break;
	default:
		break;
	}

	return true;
}

// getCursorPos(): the position of the cursor in the game's coordinates, {x, y}.
class CmdGetCursorPos : public TCommand {
public:
	bool Do() override;
	bool Redo() override;
};

// Confirmed (asm lines 398956-399086)
static void CmdGetCursorPos_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"3.3"));
}

// Confirmed (asm lines 388789-388796)
bool CmdGetCursorPos::Do() {
	return Redo();
}

// Confirmed (asm lines 390713-390745)
bool CmdGetCursorPos::Redo() {
	Result.Set(gameControl()->GetScriptMousePosition());
	Result.ToLua();
	return true;
}

// setCursorPos(pos): puts the cursor at the position (in the game's coordinates).
class CmdSetCursorPos : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxPoint _position;
};

// Confirmed (asm lines 399096-399234)
static void CmdSetCursorPos_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"3.3"));
	syntax.AddArg(TArgType::kPoint, true);
}

// Confirmed (asm lines 389016-389045)
bool CmdSetCursorPos::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_position = argument->GetPoint();

	return Redo();
}

// Confirmed (asm lines 390628-390713): the position is scaled from the resolution of the game to the displayed
// area of the window, the cursor of the window is warped there, and a mouse motion event with the position
// is pushed so that the game sees the move.
bool CmdSetCursorPos::Redo() {
	TVisObjRef game = gameControl()->GetVisionaire()->GetGame();
	const wxPoint *resolution = game.GetPoint(kGameWindowResolution);
	double scaleX = static_cast<double>(_position.x) / resolution->x;
	double scaleY = static_cast<double>(_position.y) / resolution->y;
	int x = static_cast<int>(g_displayedArea.GetWidth() * scaleX) + g_displayedArea.x;
	int y = static_cast<int>(g_displayedArea.GetHeight() * scaleY) + g_displayedArea.y;

	SDL_WarpMouseInWindow(VSPlayerWindow, x, y);

	SDL_Event event;

	event.type = SDL_MOUSEMOTION;
	event.motion.x = x;
	event.motion.y = y;
	SDL_PushEvent(&event);
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Event handlers, hooks, events and screenshots

enum {
	kEventAnimationStarted = 0,
	kEventAnimationStopped = 1,
	kEventTextStarted = 2,
	kEventTextStopped = 3,
	kEventMainLoop = 4,
	kEventMouse = 5,
	kEventKey = 6,
	kEventEngine = 7
};

/** The code of an event handler's event from its name, or -1 (the first four are only for registering). */
static int eventFromName(const wxString &name, bool all) {
	static const struct {
		const wchar_t *name;
		int code;
	} kEvents[] = {
		{L"animationStarted", kEventAnimationStarted},
		{L"animationStopped", kEventAnimationStopped},
		{L"textStarted", kEventTextStarted},
		{L"textStopped", kEventTextStopped},
		{L"mainLoop", kEventMainLoop},
		{L"mouseEvent", kEventMouse},
		{L"keyEvent", kEventKey},
		{L"engineEvent", kEventEngine},
	};

	for (const auto &event : kEvents) {
		if ((all || event.code >= kEventMainLoop) && name.ToStdWstring() == event.name)
			return event.code;
	}

	return -1;
}

// registerEventHandler(event, handler [, eventFlags]): the Lua function `handler` is called when the event
// happens: "mainLoop" (each frame, no argument), "mouseEvent" (the mouse event and the position; `eventFlags` is a
// list of the mouse events, all when none), "keyEvent" (type, character, keycode and modifiers; true stops the
// engine from handling the key), "animationStarted", "animationStopped", "textStarted" and "textStopped" (the object
// concerned), and "engineEvent".
class CmdRegisterEventHandler : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _event = 0;
	wxString _handler;
	std::vector<int> _flags;
};

// Confirmed (asm lines 399908-400067)
static void CmdRegisterEventHandler_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"3.6"));
	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kIntList, false);
}

// Confirmed (asm lines 420390-420904)
bool CmdRegisterEventHandler::Do() {
	const TArgument *argument = nullptr;
	wxString event;

	if (_parser.GetArgument(0, &argument))
		event = argument->GetString();

	if (_parser.GetArgument(1, &argument))
		_handler = argument->GetString();

	_event = eventFromName(event, true);

	if (_event < 0) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"Unsupported event '%s'.", event.wc_str());

		return false;
	}

	if (_parser.GetArgument(2, &argument)) {
		if (_event != kEventMouse) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"The parameter \"eventFlags\" cannot be specified for the event '%s'.", event.wc_str());

			return false;
		}

		_flags = argument->GetIntList();
	} else {
		// all the mouse events
		_flags.clear();

		for (int flag = 1; flag <= 18; flag++)
			_flags.push_back(flag);
	}

	return Redo();
}

// Confirmed (asm lines 390525-390618)
bool CmdRegisterEventHandler::Redo() {
	switch (_event) {
	case kEventAnimationStarted:
		TGAnimation::RegisterEventHandlerAnimStarted(_handler);
		break;
	case kEventAnimationStopped:
		TGAnimation::RegisterEventHandlerAnimStopped(_handler);
		break;
	case kEventTextStarted:
		TGText::RegisterEventHandlerTextStarted(_handler);
		break;
	case kEventTextStopped:
		TGText::RegisterEventHandlerTextStopped(_handler);
		break;
	case kEventMainLoop:
		gameControl()->RegisterEventHandlerMainLoop(_handler);
		break;
	case kEventMouse:
		g_pGameControl->RegisterMouseEventHandler(_handler, _flags);
		break;
	case kEventKey:
		g_pGameControl->RegisterKeyboardEventHandler(_handler);
		break;
	case kEventEngine:
		g_pGameControl->RegisterEngineEventHandler(_handler);
		break;
	default:
		break;
	}

	return true;
}

// unregisterEventHandler(event, handler): the function is not called any more ("mainLoop", "mouseEvent", "keyEvent"
// and "engineEvent").
class CmdUnregisterEventHandler : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _event = 0;
	wxString _handler;
};

// Confirmed (asm lines 400077-400219)
static void CmdUnregisterEventHandler_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kString, true);
}

// Confirmed (asm lines 395395-395663)
bool CmdUnregisterEventHandler::Do() {
	const TArgument *argument = nullptr;
	wxString event;

	if (_parser.GetArgument(0, &argument))
		event = argument->GetString();

	if (_parser.GetArgument(1, &argument))
		_handler = argument->GetString();

	_event = eventFromName(event, false);

	if (_event < 0) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"Unsupported event '%s'.", event.wc_str());

		return false;
	}

	return Redo();
}

// Confirmed (asm lines 391094-391149)
bool CmdUnregisterEventHandler::Redo() {
	switch (_event) {
	case kEventMainLoop:
		gameControl()->UnregisterEventHandlerMainLoop(_handler);
		break;
	case kEventMouse:
		g_pGameControl->UnregisterMouseEventHandler(_handler);
		break;
	case kEventKey:
		g_pGameControl->UnregisterKeyboardEventHandler(_handler);
		break;
	case kEventEngine:
		g_pGameControl->UnregisterEngineEventHandler(_handler);
		break;
	default:
		break;
	}

	return true;
}

enum {
	kHookSetTextPosition = 0,
	kHookGetActionText = 1,
	kHookGetCharacterAnimationIndex = 2,
	kHookSceneMousePosition = 3,
	kHookTextRender = 4,
	kHookTextText = 5,
	kHookRenderObject = 6
};

// registerHookFunction(hook, function): the Lua function is called in the place of the engine's own operation:
// "setTextPosition" (a text is put in place; true if the function did it), "getActionText" (gives the text of the
// action line), "getCharacterAnimationIndex" (the walk animation for a new direction; -1 leaves it to the
// engine), "sceneMousePosition", "textRender", "textText" and "renderObject".
class CmdRegisterHookFunction : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _hook = 0;
	wxString _function;
};

// Confirmed (asm lines 400229-400340)
static void CmdRegisterHookFunction_GetSyntax(TArgSyntax &syntax) {
	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kString, true);
}

// Confirmed (asm lines 395028-395385)
bool CmdRegisterHookFunction::Do() {
	static const struct {
		const wchar_t *name;
		int code;
	} kHooks[] = {
		{L"setTextPosition", kHookSetTextPosition},
		{L"getActionText", kHookGetActionText},
		{L"getCharacterAnimationIndex", kHookGetCharacterAnimationIndex},
		{L"sceneMousePosition", kHookSceneMousePosition},
		{L"textRender", kHookTextRender},
		{L"textText", kHookTextText},
		{L"renderObject", kHookRenderObject},
	};
	const TArgument *argument = nullptr;
	wxString hook;

	if (_parser.GetArgument(0, &argument))
		hook = argument->GetString();

	if (_parser.GetArgument(1, &argument))
		_function = argument->GetString();

	_hook = -1;

	for (const auto &entry : kHooks) {
		if (hook.ToStdWstring() == entry.name)
			_hook = entry.code;
	}

	if (_hook < 0) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"Unsupported hook '%s'.", hook.wc_str());

		return false;
	}

	return Redo();
}

// Confirmed (asm lines 396129-396240)
bool CmdRegisterHookFunction::Redo() {
	switch (_hook) {
	case kHookSetTextPosition:
		TGText::RegisterHookFunctionSetTextPosition(_function);
		break;
	case kHookGetActionText:
		gameControl()->GetObjectManager()->RegisterHookFunctionGetActionText(_function);
		break;
	case kHookGetCharacterAnimationIndex:
		TGCharacter::RegisterHookFunctionGetCharacterAnimationIndex(_function);
		break;
	case kHookSceneMousePosition:
		gameControl()->RegisterHookFunctionSceneMousePosition(_function);
		break;
	case kHookTextRender:
		TGText::RegisterHookFunctionRender(_function);
		break;
	case kHookTextText:
		TGText::RegisterHookFunctionText(_function);
		break;
	case kHookRenderObject:
		TManagedObject::HookFunctionRender = std::string(_function.mb_str());
		break;
	default:
		break;
	}

	return true;
}

// createScreenshot([saveTo] [, {clear = true}]): without a path the screenshot is the one that the next
// savegames use; with a path (it has to end in .png) the screenshot is saved to that file; with the flag
// `clear` the screenshot for the savegames is cleared (the engine then makes one by itself when the scene
// changes from a playable one to a menu).
class CmdCreateScreenshot : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxFileName _path;
	bool _saveToFile = false;
	bool _clear = false;
};

// Confirmed (asm lines 399244-399480)
static void CmdCreateScreenshot_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"3.7"));
	syntax.AddArg(TArgType::kString, false);
	syntax.AddFlag(wxString(L"clr"), wxString(L"clear"), TArgType::kBool, false);
}

// Confirmed (asm lines 397447-397789)
bool CmdCreateScreenshot::Do() {
	const TArgument *argument = nullptr;

	_saveToFile = false;

	if (_parser.GetArgument(0, &argument)) {
		_saveToFile = true;

		wxString path = argument->GetString();

		_path = wxFileName(path.ToStdWstring());

		if (!_path.IsOk()) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"'%s' is not a valid path.", path.wc_str());

			return false;
		}

		if (_path.GetExt().CmpNoCase(wxString(L"png")) != 0) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"Screenshot can only be saved to a file with .png file extension.");

			return false;
		}
	}

	if (_parser.GetFlagArgument(wxString(L"clear"), &argument)) {
		if (_saveToFile) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"When the flag '%s' is specified the 'saveTo' Parameter is not allowed.", L"clear");

			return false;
		}

		_clear = argument->GetBool();
	} else {
		_clear = false;
	}

	return Redo();
}

// Confirmed (asm lines 392054-392180)
bool CmdCreateScreenshot::Redo() {
	if (!_saveToFile) {
		if (_clear)
			graphics->ClearSavegameScreenshot(true);
		else
			graphics->CreateSavegameScreenshot(true);

		return true;
	}

	TPictureIO picture;

	if (!graphics->CaptureScreen(picture)) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Creating screenshot failed.");

		return true;
	}

	if (!picture.SavePicture(_path, false) && wxLog::loglevel >= 0)
		wxLog::logexpanded(L"Saving screenshot failed.");

	return true;
}

enum {
	kCreateMouseLeftButtonDown = 0,
	kCreateMouseLeftButtonUp = 1,
	kCreateMouseMiddleButtonDown = 2,
	kCreateMouseMiddleButtonUp = 3,
	kCreateMouseRightButtonDown = 4,
	kCreateMouseRightButtonUp = 5,
	kCreateMouseWheelDown = 6,
	kCreateMouseWheelUp = 7,
	kCreateControllerAxisMouseMove = 8,
	kCreateControllerAxisCharacterMove = 9,
	kCreateKeyDown = 10,
	kCreateKeyUp = 11,
	kCreateControllerKeyDown = 12,
	kCreateControllerKeyUp = 13,
	kCreateControllerAxis = 14
};

// createEvent(event [, pos [, param1 [, param2]]]): puts an event in the queue of the engine, as if the player had done
// it: a mouse button or the wheel (eEvtMouseLeftButtonDown ...), a key (eEvtKeyDown/Up: key code, modifiers), a
// controller key or axis (eEvtControllerKeyDown/Up, eEvtControllerAxis: button or axis, controller), or the movement
// of the mouse or of the character by a controller axis (the position of the axis, a threshold and for the mouse an
// acceleration). The event is given by its name as a string. The result tells whether it worked.
class CmdCreateEvent : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _event = 0;
	wxPoint _position;
	int _param1 = 0;
	int _param2 = 0;
};

// Confirmed (asm lines 407059-407221)
static void CmdCreateEvent_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.3"));
	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kPoint, false);
	syntax.AddArg(TArgType::kInt, false);
	syntax.AddArg(TArgType::kInt, false);
}

// Confirmed (asm lines 392420-393033)
bool CmdCreateEvent::Do() {
	static const struct {
		const wchar_t *name;
		int code;
	} kEvents[] = {
		{L"eEvtMouseLeftButtonDown", kCreateMouseLeftButtonDown},
		{L"eEvtMouseLeftButtonUp", kCreateMouseLeftButtonUp},
		{L"eEvtMouseMiddleButtonDown", kCreateMouseMiddleButtonDown},
		{L"eEvtMouseMiddleButtonUp", kCreateMouseMiddleButtonUp},
		{L"eEvtMouseRightButtonDown", kCreateMouseRightButtonDown},
		{L"eEvtMouseRightButtonUp", kCreateMouseRightButtonUp},
		{L"eEvtMouseWheelDown", kCreateMouseWheelDown},
		{L"eEvtMouseWheelUp", kCreateMouseWheelUp},
		{L"eEvtControllerAxisMouseMove", kCreateControllerAxisMouseMove},
		{L"eEvtControllerAxisCharacterMove", kCreateControllerAxisCharacterMove},
		{L"eEvtKeyDown", kCreateKeyDown},
		{L"eEvtKeyUp", kCreateKeyUp},
		{L"eEvtControllerKeyDown", kCreateControllerKeyDown},
		{L"eEvtControllerKeyUp", kCreateControllerKeyUp},
		{L"eEvtControllerAxis", kCreateControllerAxis},
	};
	const TArgument *argument = nullptr;
	wxString event;

	_param1 = 0;
	_param2 = 0;

	if (_parser.GetArgument(0, &argument))
		event = argument->GetString();

	_event = -1;

	for (const auto &entry : kEvents) {
		if (event.ToStdWstring() == entry.name)
			_event = entry.code;
	}

	if (_event < 0) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"Unsupported event '%s'.", event.wc_str());

		return false;
	}

	if (_parser.GetArgument(1, &argument))
		_position = argument->GetPoint();

	if (_parser.GetArgument(2, &argument))
		_param1 = argument->GetInt();

	if (_parser.GetArgument(3, &argument))
		_param2 = argument->GetInt();

	return Redo();
}

// Confirmed (asm lines 410841-411382)
bool CmdCreateEvent::Redo() {
	// (the result of the command is whether it worked)
	auto finish = [](bool success) {
		Result.Set(success);
		Result.ToLua();
		return true;
	};
	SDL_Event sdlEvent;

	std::memset(&sdlEvent, 0, sizeof(sdlEvent));

	switch (_event) {
	case kCreateMouseLeftButtonDown:
	case kCreateMouseMiddleButtonDown:
	case kCreateMouseRightButtonDown:
		sdlEvent.type = SDL_MOUSEBUTTONDOWN;
		sdlEvent.button.button = static_cast<Uint8>((_event - kCreateMouseLeftButtonDown) / 2 + 1);
		break;
	case kCreateMouseLeftButtonUp:
	case kCreateMouseMiddleButtonUp:
	case kCreateMouseRightButtonUp:
		sdlEvent.type = SDL_MOUSEBUTTONUP;
		sdlEvent.button.button = static_cast<Uint8>((_event - kCreateMouseLeftButtonUp) / 2 + 1);
		break;
	case kCreateMouseWheelDown:
		sdlEvent.type = SDL_MOUSEWHEEL;
		sdlEvent.wheel.y = -1;
		break;
	case kCreateMouseWheelUp:
		sdlEvent.type = SDL_MOUSEWHEEL;
		sdlEvent.wheel.y = 1;
		break;
	case kCreateControllerAxisMouseMove:
		TGameController::ControllerAxisMouseMove(_position, _param1, _param2);
		return finish(true);
	case kCreateControllerAxisCharacterMove:
		TGameController::ControllerAxisCharacterMove(_position, _param1);
		return finish(true);
	case kCreateKeyDown:
	case kCreateKeyUp:
		sdlEvent.type = (_event == kCreateKeyDown) ? SDL_KEYDOWN : SDL_KEYUP;
		sdlEvent.key.keysym.sym = _param1;
		sdlEvent.key.keysym.mod = static_cast<Uint16>(_param2);
		break;
	case kCreateControllerKeyDown:
	case kCreateControllerKeyUp:
		sdlEvent.type = (_event == kCreateControllerKeyDown) ? SDL_CONTROLLERBUTTONDOWN : SDL_CONTROLLERBUTTONUP;
		sdlEvent.cbutton.button = static_cast<Uint8>(_param1);
		sdlEvent.cbutton.which = _param2;
		break;
	case kCreateControllerAxis:
		sdlEvent.type = SDL_CONTROLLERAXISMOTION;
		sdlEvent.caxis.value = static_cast<Sint16>(_position.x);
		sdlEvent.caxis.axis = static_cast<Uint8>(_param1);
		sdlEvent.caxis.which = _param2;
		break;
	default:
		return finish(false);
	}

	if (SDL_PushEvent(&sdlEvent) == -1) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"SDL_PushEvent failed: %s", wxString(SDL_GetError()).wc_str());

		return finish(false);
	}

	return finish(true);
}

// ---------------------------------------------------------------------------------------------------------
// Sounds

// startSound(sounditem [, {volume = , balance = , loop = , offset = }]): plays a sound file (volume 0 to 100, 100 when
// not given; balance -100 left to 100 right; loop; offset: where to start, in milliseconds). The result is the id of
// the sound, or -1 if it could not be started.
class CmdStartSound : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxFileName _sound;
	int _volume = 100;
	int _balance = 0;
	bool _loop = false;
	int _offset = 0;
};

// Confirmed (asm lines 401646-402175)
static void CmdStartSound_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
	syntax.AddArg(TArgType::kPath, true);
	syntax.AddFlag(wxString(L"vol"), wxString(L"volume"), TArgType::kInt, false);
	syntax.AddFlag(wxString(L"bal"), wxString(L"balance"), TArgType::kInt, false);
	syntax.AddFlag(wxString(L"lop"), wxString(L"loop"), TArgType::kBool, false);
	syntax.AddFlag(wxString(L"ofs"), wxString(L"offset"), TArgType::kInt, false);
}

// Confirmed (asm lines 399619-399898)
bool CmdStartSound::Do() {
	const TArgument *argument = nullptr;

	_volume = 100;
	_balance = 0;
	_loop = false;
	_offset = 0;

	if (!_parser.GetArgument(0, &argument)) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"startSound without sounditem");

		return false;
	}

	_sound = wxFileName(argument->GetPath().ToStdWstring());

	if (_parser.GetFlagArgument(wxString(L"vol"), &argument))
		_volume = argument->GetInt();

	if (_parser.GetFlagArgument(wxString(L"bal"), &argument))
		_balance = argument->GetInt();

	if (_parser.GetFlagArgument(wxString(L"lop"), &argument))
		_loop = argument->GetBool();

	if (_parser.GetFlagArgument(wxString(L"ofs"), &argument))
		_offset = argument->GetInt();

	return Redo();
}

// Confirmed (asm lines 390425-390473)
bool CmdStartSound::Redo() {
	TSoundInterface *sound = gameControl()->GetSoundManager();

	Result.Set(sound->Play(_sound, _volume, _balance, _loop, TSoundTypeEnum::kSound, false, _offset));
	Result.ToLua();
	return true;
}

// stopSound(soundID): stops a sound; the result tells whether it was stopped.
class CmdStopSound : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _id = -1;
};

// Confirmed (asm lines 402185-402323)
static void CmdStopSound_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
	syntax.AddArg(TArgType::kInt, true);
}

// Confirmed (asm lines 389173-389202)
bool CmdStopSound::Do() {
	const TArgument *argument = nullptr;

	_id = -1;

	if (_parser.GetArgument(0, &argument))
		_id = argument->GetInt();

	return Redo();
}

// Confirmed (asm lines 390390-390415)
bool CmdStopSound::Redo() {
	bool stopped = false;

	if (_id != -1)
		stopped = gameControl()->GetSoundManager()->Stop(_id);

	Result.Set(stopped);
	Result.ToLua();
	return true;
}

// getSoundId(sounditem): the id of the sound of that file if it is active, else -1.
class CmdGetSoundId : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxFileName _sound;
};

// Confirmed (asm lines 402333-402471)
static void CmdGetSoundId_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
	syntax.AddArg(TArgType::kPath, true);
}

// Confirmed (asm lines 399490-399609)
bool CmdGetSoundId::Do() {
	const TArgument *argument = nullptr;

	if (!_parser.GetArgument(0, &argument)) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"getSoundId without sounditem");

		return false;
	}

	_sound = wxFileName(argument->GetPath().ToStdWstring());
	return Redo();
}

// Confirmed (asm lines 390360-390390)
bool CmdGetSoundId::Redo() {
	Result.Set(gameControl()->GetSoundManager()->GetExistingSoundID(_sound));
	Result.ToLua();
	return true;
}

// toggleSoundPause(soundID): pauses a sound that plays, and continues one that is paused; the result tells
// whether that worked.
class CmdToggleSoundPause : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _id = -1;
};

// Confirmed (asm lines 403171-403309)
static void CmdToggleSoundPause_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
	syntax.AddArg(TArgType::kInt, true);
}

// Confirmed (asm lines 389134-389163)
bool CmdToggleSoundPause::Do() {
	const TArgument *argument = nullptr;

	_id = -1;

	if (_parser.GetArgument(0, &argument))
		_id = argument->GetInt();

	return Redo();
}

// Confirmed (asm lines 390141-390177)
bool CmdToggleSoundPause::Redo() {
	bool done = false;

	if (_id != -1)
		done = gameControl()->GetSoundManager()->TogglePause(_id);

	Result.Set(done);
	Result.ToLua();
	return true;
}

enum {
	kSoundPropertyVolume = 0,
	kSoundPropertyBalance = 1,
	kSoundPropertyOffset = 2,
	kSoundPropertyDuration = 3,
	kSoundPropertyLoop = 4,
	kSoundPropertyPlaying = 5,
	kSoundPropertyPaused = 6
};

// getSoundProperty(soundID, property): "volume" (0 to 100, -1 if it could not be got), "balance" (-100 to 100),
// "offset" (the position in milliseconds, -1), "duration" (the length in milliseconds, -1), "loop", "playing" and "paused"
// (true or false). For no sound (-1) the answer is -1.
class CmdGetSoundProperty : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _id = -1;
	int _property = 0;
};

// Confirmed (asm lines 402481-402627)
static void CmdGetSoundProperty_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
	syntax.AddArg(TArgType::kInt, true);
	syntax.AddArg(TArgType::kString, true);
}

// Confirmed (asm lines 394305-394662)
bool CmdGetSoundProperty::Do() {
	static const struct {
		const wchar_t *name;
		int code;
	} kProperties[] = {
		{L"volume", kSoundPropertyVolume},
		{L"balance", kSoundPropertyBalance},
		{L"offset", kSoundPropertyOffset},
		{L"duration", kSoundPropertyDuration},
		{L"loop", kSoundPropertyLoop},
		{L"playing", kSoundPropertyPlaying},
		{L"paused", kSoundPropertyPaused},
	};
	const TArgument *argument = nullptr;
	wxString property;

	_id = -1;

	if (_parser.GetArgument(0, &argument))
		_id = argument->GetInt();

	if (_parser.GetArgument(1, &argument))
		property = argument->GetString();

	_property = -1;

	for (const auto &entry : kProperties) {
		if (property.ToStdWstring() == entry.name)
			_property = entry.code;
	}

	if (_property < 0) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"Unsupported property '%s'.", property.wc_str());

		return false;
	}

	return Redo();
}

// Confirmed (asm lines 390224-390350)
bool CmdGetSoundProperty::Redo() {
	if (_id == -1) {
		Result.Set(-1);
		Result.ToLua();
		return true;
	}

	TSoundInterface *sound = gameControl()->GetSoundManager();

	switch (_property) {
	case kSoundPropertyVolume:
		Result.Set(sound->GetVolume(_id));
		break;
	case kSoundPropertyBalance:
		Result.Set(sound->GetBalance(_id));
		break;
	case kSoundPropertyOffset:
		Result.Set(sound->GetOffset(_id));
		break;
	case kSoundPropertyDuration:
		Result.Set(sound->GetDuration(_id));
		break;
	case kSoundPropertyLoop:
		Result.Set(sound->IsLoop(_id));
		break;
	case kSoundPropertyPlaying:
		Result.Set(sound->IsPlaying(_id));
		break;
	case kSoundPropertyPaused:
		Result.Set(sound->IsPaused(_id));
		break;
	default:
		Result.Set(-1);
		break;
	}

	Result.ToLua();
	return true;
}

// setSoundProperty(soundID, {volume = , balance = , loop = , offset = }): changes a sound that plays; the result
// tells whether that worked.
class CmdSetSoundProperty : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _id = -1;
	int _volume = 100;
	int _balance = 0;
	bool _loop = false;
	int _offset = 0;
};

// Confirmed (asm lines 402637-403161)
static void CmdSetSoundProperty_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
	syntax.AddArg(TArgType::kInt, true);
	syntax.AddFlag(wxString(L"vol"), wxString(L"volume"), TArgType::kInt, false);
	syntax.AddFlag(wxString(L"bal"), wxString(L"balance"), TArgType::kInt, false);
	syntax.AddFlag(wxString(L"lop"), wxString(L"loop"), TArgType::kBool, false);
	syntax.AddFlag(wxString(L"ofs"), wxString(L"offset"), TArgType::kInt, false);
}

// Confirmed (asm lines 391716-391944)
bool CmdSetSoundProperty::Do() {
	const TArgument *argument = nullptr;

	_id = -1;
	_volume = 100;
	_balance = 0;
	_loop = false;
	_offset = 0;

	if (_parser.GetArgument(0, &argument))
		_id = argument->GetInt();

	if (_parser.GetFlagArgument(wxString(L"vol"), &argument))
		_volume = argument->GetInt();

	if (_parser.GetFlagArgument(wxString(L"bal"), &argument))
		_balance = argument->GetInt();

	if (_parser.GetFlagArgument(wxString(L"lop"), &argument))
		_loop = argument->GetBool();

	if (_parser.GetFlagArgument(wxString(L"ofs"), &argument))
		_offset = argument->GetInt();

	return Redo();
}

// Confirmed (asm lines 390177-390214)
bool CmdSetSoundProperty::Redo() {
	bool done = false;

	if (_id != -1)
		done = gameControl()->GetSoundManager()->SetStats(_id, _volume, _balance, TSoundTypeEnum::kSound, _loop, _offset);

	Result.Set(done);
	Result.ToLua();
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// The game client (Steam, GOG Galaxy)

// initGameClient(clientID, clientSecret): starts GOG Galaxy with the values; the result tells whether it did.
class CmdInitGameClient : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxString _clientId;
	wxString _clientSecret;
};

// Confirmed (asm lines 400350-400492)
static void CmdInitGameClient_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.1"));
	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kString, true);
}

// Confirmed (asm lines 391473-391524)
bool CmdInitGameClient::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_clientId = argument->GetString();

	if (_parser.GetArgument(1, &argument))
		_clientSecret = argument->GetString();

	return Redo();
}

// Confirmed (asm lines 390483-390515)
bool CmdInitGameClient::Redo() {
	TGalaxySDK *galaxy = g_pGameControl->GetGameClientSDK()->GetGalaxy();

	Result.Set(galaxy->IsActive() && galaxy->InitSDK(_clientId, _clientSecret));
	Result.ToLua();
	return true;
}

// getGameClientStat(apiName): the integer value of the stat of the game client, or -1.
class CmdGetGameClientStat : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxString _name;
};

// Confirmed (asm lines 400502-400636)
static void CmdGetGameClientStat_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.1"));
	syntax.AddArg(TArgType::kString, true);
}

// Confirmed (asm lines 391290-391321)
bool CmdGetGameClientStat::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_name = argument->GetString();

	return Redo();
}

// Confirmed (asm lines 397799-397829)
bool CmdGetGameClientStat::Redo() {
	Result.Set(g_pGameControl->GetGameClientSDK()->GetStat(_name));
	Result.ToLua();
	return true;
}

// setGameClientStat(apiName, value): sets the stat; the result tells whether that worked.
class CmdSetGameClientStat : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxString _name;
	int _value = 0;
};

// Confirmed (asm lines 400646-400788)
static void CmdSetGameClientStat_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.1"));
	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kInt, true);
}

// Confirmed (asm lines 391413-391463)
bool CmdSetGameClientStat::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_name = argument->GetString();

	if (_parser.GetArgument(1, &argument))
		_value = argument->GetInt();

	return Redo();
}

// Confirmed (asm lines 397829-397869)
bool CmdSetGameClientStat::Redo() {
	Result.Set(g_pGameControl->GetGameClientSDK()->SetStat(_name, _value));
	Result.ToLua();
	return true;
}

// resetGameClientStats([resetAchievements]): resets all stats of the game client (the achievements too when
// `resetAchievements` is true; with GOG Galaxy it is always so).
class CmdResetGameClientStats : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	bool _resetAchievements = false;
};

// Confirmed (asm lines 400798-400933)
static void CmdResetGameClientStats_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.1"));
	syntax.AddArg(TArgType::kBool, false);
}

// Confirmed (asm lines 389988-390026)
bool CmdResetGameClientStats::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_resetAchievements = argument->GetBool();
	else
		_resetAchievements = false;

	return Redo();
}

// Confirmed (asm lines 397869-397899)
bool CmdResetGameClientStats::Redo() {
	Result.Set(g_pGameControl->GetGameClientSDK()->ResetStats(_resetAchievements));
	Result.ToLua();
	return true;
}

// getGameClientAchievement(apiName): whether the achievement is set.
class CmdGetGameClientAchievement : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxString _name;
};

// Confirmed (asm lines 400943-401077)
static void CmdGetGameClientAchievement_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.1"));
	syntax.AddArg(TArgType::kString, true);
}

// Confirmed (asm lines 391331-391362)
bool CmdGetGameClientAchievement::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_name = argument->GetString();

	return Redo();
}

// Confirmed (asm lines 397899-397929)
bool CmdGetGameClientAchievement::Redo() {
	Result.Set(g_pGameControl->GetGameClientSDK()->GetAchievement(_name));
	Result.ToLua();
	return true;
}

// setGameClientAchievement(apiName [, {clear = true}]): sets an achievement to done (or clears it with the flag);
// the result tells whether that worked.
class CmdSetGameClientAchievement : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxString _name;
	bool _clear = false;
};

// Confirmed (asm lines 401087-401319)
static void CmdSetGameClientAchievement_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.1"));
	syntax.AddArg(TArgType::kString, true);
	syntax.AddFlag(wxString(L"clr"), wxString(L"clear"), TArgType::kBool, false);
}

// Confirmed (asm lines 391954-392046)
bool CmdSetGameClientAchievement::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_name = argument->GetString();

	if (_parser.GetFlagArgument(wxString(L"clear"), &argument))
		_clear = argument->GetBool();
	else
		_clear = false;

	return Redo();
}

// Confirmed (asm lines 397929-397961)
bool CmdSetGameClientAchievement::Redo() {
	TGameClientSDK *client = g_pGameControl->GetGameClientSDK();

	Result.Set(_clear ? client->ClearAchievement(_name) : client->SetAchievement(_name));
	Result.ToLua();
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Properties of the system, the window, and the game

enum {
	kPropertyPlatform = 0,
	kPropertySteamInitialized = 1,
	kPropertyGalaxyInitialized = 2,
	kPropertyGalaxyReady = 3,
	kPropertySystemLanguage = 4,
	kPropertySystemLanguageCode = 5,
	kPropertyDisplayResolution = 6
};

// getProperty(property): "platform" (the system: "linux" here), "steam_initialized", "galaxy_initialized" and
// "galaxy_ready" (true or false), "system_language" (the English name of the language of the system, or "unknown"),
// "system_language_code", and "display_resolution" (the part of the window the game is drawn in, a rect).
class CmdGetProperty : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _property = 0;
};

// Confirmed (asm lines 401477-401636)
static void CmdGetProperty_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.2"));
	syntax.AddArg(TArgType::kString, true);
}

// Confirmed (asm lines 394672-395018)
bool CmdGetProperty::Do() {
	static const struct {
		const wchar_t *name;
		int code;
	} kProperties[] = {
		{L"platform", kPropertyPlatform},
		{L"steam_initialized", kPropertySteamInitialized},
		{L"galaxy_initialized", kPropertyGalaxyInitialized},
		{L"galaxy_ready", kPropertyGalaxyReady},
		{L"system_language", kPropertySystemLanguage},
		{L"system_language_code", kPropertySystemLanguageCode},
		{L"display_resolution", kPropertyDisplayResolution},
	};
	const TArgument *argument = nullptr;
	wxString property;

	if (_parser.GetArgument(0, &argument))
		property = argument->GetString();

	_property = -1;

	for (const auto &entry : kProperties) {
		if (property.ToStdWstring() == entry.name)
			_property = entry.code;
	}

	if (_property < 0) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"Unsupported property '%s'.", property.wc_str());

		return false;
	}

	return Redo();
}

// Confirmed (asm lines 397971-398203)
bool CmdGetProperty::Redo() {
	switch (_property) {
	case kPropertyPlatform:
		Result.Set(wxString(L"linux"));
		break;
	case kPropertySteamInitialized:
		Result.Set(g_pGameControl->GetGameClientSDK()->GetStatus(kGameClientSteamInitialized));
		break;
	case kPropertyGalaxyInitialized:
		Result.Set(g_pGameControl->GetGameClientSDK()->GetStatus(kGameClientGalaxyInitialized));
		break;
	case kPropertyGalaxyReady:
		Result.Set(g_pGameControl->GetGameClientSDK()->GetStatus(kGameClientGalaxyReady));
		break;
	case kPropertySystemLanguage:
	case kPropertySystemLanguageCode: {
		wxString language(L"unknown");
		int system = wxLocale::GetSystemLanguage();

		if (system != wxLANGUAGE_UNKNOWN) {
			language = (_property == kPropertySystemLanguage) ? wxLocale::GetLanguageName(system)
			           : wxLocale::GetLanguageCode(system);
		}

		Result.Set(language);
		break;
	}
	case kPropertyDisplayResolution:
		Result.Set(g_displayedArea);
		break;
	default:
		return true;
	}

	Result.ToLua();
	return true;
}

// startDefaultBrowser(url): opens the URL in the overlay of the game client, or else in the default browser; the
// result tells whether that worked.
class CmdStartDefaultBrowser : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxString _url;
};

// Confirmed (asm lines 401329-401467)
static void CmdStartDefaultBrowser_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.3"));
	syntax.AddArg(TArgType::kString, true);
}

// Confirmed (asm lines 391372-391403)
bool CmdStartDefaultBrowser::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_url = argument->GetString();

	return Redo();
}

// Confirmed (asm lines 398213-398250)
bool CmdStartDefaultBrowser::Redo() {
	bool done = true;

	if (!g_pGameControl->GetGameClientSDK()->ActivateGameOverlayToWebPage(_url))
		done = wxLaunchDefaultBrowser(_url);

	Result.Set(done);
	Result.ToLua();
	return true;
}

// setWindowTitle(title): the title of the window (an empty title leaves it as it is).
class CmdSetWindowTitle : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxString _title;
};

// Confirmed (asm lines 403319-403457)
static void CmdSetWindowTitle_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
	syntax.AddArg(TArgType::kString, true);
}

// Confirmed (asm lines 391250-391280)
bool CmdSetWindowTitle::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_title = argument->GetString();

	return Redo();
}

// Confirmed (asm lines 395772-395814)
bool CmdSetWindowTitle::Redo() {
	if (!_title.IsEmpty())
		SDL_SetWindowTitle(VSPlayerWindow, _title.mb_str());

	return true;
}

// getWindowBrightness(): the brightness (gamma) of the window, 0 (dark) to 100 (normal).
class CmdGetWindowBrightness : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _brightness = 0;
};

// Confirmed (asm lines 403467-403589)
static void CmdGetWindowBrightness_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
}

// Confirmed (asm lines 388806-388814)
bool CmdGetWindowBrightness::Do() {
	return Redo();
}

// Confirmed (asm lines 390112-390141)
bool CmdGetWindowBrightness::Redo() {
	_brightness = static_cast<int>(SDL_GetWindowBrightness(VSPlayerWindow) * 100.0f);
	Result.Set(_brightness);
	Result.ToLua();
	return true;
}

// setWindowBrightness(brightness): sets the brightness (gamma) of the window, 0 (dark) to 100 (normal), up
// to 200; false if the system cannot.
class CmdSetWindowBrightness : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	unsigned int _brightness = 100;
};

// Confirmed (asm lines 403599-403733)
static void CmdSetWindowBrightness_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
	syntax.AddArg(TArgType::kInt, true);
}

// Confirmed (asm lines 391635-391706)
bool CmdSetWindowBrightness::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_brightness = argument->GetInt();

	if (_brightness > 200) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Window brightness is out of range.");

		return false;
	}

	return Redo();
}

// Confirmed (asm lines 395671-395762)
bool CmdSetWindowBrightness::Redo() {
	float brightness = static_cast<float>(static_cast<int>(_brightness)) / 100.0f;

	if (brightness == SDL_GetWindowBrightness(VSPlayerWindow))
		return true;

	if (SDL_SetWindowBrightness(VSPlayerWindow, brightness) == 0)
		return true;

	if (wxLog::loglevel >= 0)
		wxLog::logexpanded(L"Unable to set brightness: %s", wxString(SDL_GetError()).wc_str());

	return false;
}

// replaceGame(path): the game is replaced by the game file (relative to the current one).
class CmdReplaceGame : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxFileName _file;
};

// Confirmed (asm lines 403743-403877)
static void CmdReplaceGame_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
	syntax.AddArg(TArgType::kPath, true);
}

// Confirmed (asm lines 397297-397437)
bool CmdReplaceGame::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_file = wxFileName(argument->GetPath().ToStdWstring());

	return Redo();
}

// Confirmed (asm lines 397214-397287)
bool CmdReplaceGame::Redo() {
	wxFileName file = _file;

	file.NormalizePath();
	return gameControl()->ReplaceGame(file, true);
}

// ---------------------------------------------------------------------------------------------------------
// Tweens

// Confirmed (asm lines 407771-408355): the number of an easing in the scripts (the ease... globals of
// SetEnums: Back, Bounce, Circ, Cubic, Elastic, Linear, None, Quad, Quart, Quint and Sine, each In, Out and InOut) is
// the function of Easing.h; a number outside 0 to 32 is not an easing.
static std::function<double(double)> easingByNumber(int number) {
	static double (*const kEasings[])(double) = {
		Easing::BackIn, Easing::BackOut, Easing::BackInOut,
		Easing::BounceIn, Easing::BounceOut, Easing::BounceInOut,
		Easing::CircIn, Easing::CircOut, Easing::CircInOut,
		Easing::CubicIn, Easing::CubicOut, Easing::CubicInOut,
		Easing::ElasticIn, Easing::ElasticOut, Easing::ElasticInOut,
		Easing::LinearIn, Easing::LinearOut, Easing::LinearInOut,
		Easing::NoneIn, Easing::NoneOut, Easing::NoneInOut,
		Easing::QuadIn, Easing::QuadOut, Easing::QuadInOut,
		Easing::QuartIn, Easing::QuartOut, Easing::QuartInOut,
		Easing::QuintIn, Easing::QuintOut, Easing::QuintInOut,
		Easing::SineIn, Easing::SineOut, Easing::SineInOut
	};

	if (number < 0 || number > 32)
		return nullptr;

	return kEasings[number];
}

/// Confirmed (asm lines 408978-410841, `CmdVisObjTo`): the object method `to`, object:to(duration, {field = value ...}
// [, easing [, repeat [, reverseRepeat]]]): changes the fields of the object to the values over `duration`
// milliseconds, by the easing (eEase..., linear in and out when there is none or it is not one). A value is a number
// (for a field that is an integer or a float) or a point {x=, y=} (for a point field); the tween that works on the
// object already is replaced (see TGameControl::StartTween).
void CmdVisObjTo(lua_State *state) {
	LuaVisionaireObject *self = CheckVisionaireObject(state, 1, true);
	int duration = static_cast<int>(luaL_checkinteger(state, 1));

	lua_remove(state, 1);

	std::function<double(double)> easing = Easing::LinearInOut;
	bool repeat = false;
	bool reverseRepeat = false;

	if (lua_gettop(state) > 1) {
		std::function<double(double)> chosen = easingByNumber(static_cast<int>(luaL_checkinteger(state, 2)));

		lua_remove(state, 2);

		if (chosen)
			easing = chosen;
	}

	if (lua_gettop(state) > 1) {
		repeat = lua_toboolean(state, 2) != 0;
		lua_remove(state, 2);
	}

	if (lua_gettop(state) > 1) {
		reverseRepeat = lua_toboolean(state, 2) != 0;
		lua_remove(state, 2);
	}

	lua_pushnil(state);

	while (lua_next(state, 1) != 0) {
		enum {
			kNumber = 1,
			kPoint = 2
		} kind = kNumber;
		double number = 0.0;
		wxPoint point;
		const char *name = nullptr;
		bool valid = false;

		if (lua_isstring(state, -2) && lua_isnumber(state, -1)) {
			number = lua_tonumber(state, -1);
			name = lua_tolstring(state, -2, nullptr);
			kind = kNumber;
			valid = true;
		} else if (lua_isstring(state, -2) && lua_type(state, -1) == LUA_TTABLE && ConvertFromLua(point, -2)) {
			name = lua_tolstring(state, -2, nullptr);
			kind = kPoint;
			valid = true;
		}

		if (!valid) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"VisObjTo: An option has not the format string=integer/string=point");

			lua_settop(state, -2);
			continue;
		}

		if (name) {
			TVisionaireObject *object = self->object;
			TTypeGroup *group = GetTypeGroup(static_cast<signed char>(object->GetId()[3]));
			int field = getFieldFromString(self, name);

			if (field == -1) {
				if (wxLog::loglevel >= 0)
					wxLog::logexpanded(L"VisObjTo: field %s not found", wxString(name).wc_str());
			} else if (kind == kNumber) {
				eTypeData type = group->GetType(field, true);
				Tween none(0.0, 0.0, 0.0, Easing::NoneIn, false, false);

				if (type == eTypeData::kInt) {
					Tween x(object->GetInt(field), number, duration, easing, repeat, reverseRepeat);

					gameControl()->StartTween(TVisObjTween(x, TVisObjRef(object), field, none));
				} else if (type == eTypeData::kFloat) {
					Tween x(object->GetFloat(field), number, duration, easing, repeat, reverseRepeat);

					gameControl()->StartTween(TVisObjTween(x, TVisObjRef(object), field, none));
				}
			} else if (group->GetType(field, true) == eTypeData::kPoint) {
				const wxPoint *current = object->GetPoint(field);
				Tween y(current->y, point.y, duration, easing, repeat, reverseRepeat);
				Tween x(current->x, point.x, duration, easing, repeat, reverseRepeat);

				gameControl()->StartTween(TVisObjTween(x, TVisObjRef(object), field, y));
			}
		}

		lua_settop(state, -2);
	}
}

// startTween(target, from, to, duration, easing [, repeat [, reverseRepeat [, onFinish]]]): changes a number from
// `from` to `to` in `duration` milliseconds, by the easing (eEase...). The number is read with the name `target`
// (it is the name that another tween with the same one replaces). With `repeat` the tween starts again when it is
// over, with `reverseRepeat` as well it goes back first.
class CmdStartTween : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	float _duration = 0.0f;
	float _from = 0.0f;
	float _to = 0.0f;
	std::function<double(double)> _easing;
	std::string _target;
	std::string _onFinish;  // (read, but the tween does not use it)
	bool _repeat = false;
	bool _reverseRepeat = false;
};

// Confirmed (asm lines 403887-404077)
static void CmdStartTween_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kFloat, true);
	syntax.AddArg(TArgType::kFloat, true);
	syntax.AddArg(TArgType::kFloat, true);
	syntax.AddArg(TArgType::kInt, true);
	syntax.AddArg(TArgType::kBool, false);
	syntax.AddArg(TArgType::kBool, false);
	syntax.AddArg(TArgType::kString, false);
}

// Confirmed (asm lines 408355-408800)
bool CmdStartTween::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_target = std::string(argument->GetString().mb_str());

	if (_parser.GetArgument(1, &argument))
		_from = argument->GetFloat();

	if (_parser.GetArgument(2, &argument))
		_to = argument->GetFloat();

	if (_parser.GetArgument(3, &argument))
		_duration = argument->GetFloat();

	if (_parser.GetArgument(4, &argument)) {
		int number = argument->GetInt();

		_easing = easingByNumber(number);

		if (!_easing) {
			if (wxLog::loglevel > 0)
				wxLog::logexpanded(L"Unsupported property '%d'.", number);

			return false;
		}
	}

	_repeat = false;

	if (_parser.GetArgument(5, &argument))
		_repeat = argument->GetBool();

	_reverseRepeat = false;

	if (_parser.GetArgument(6, &argument))
		_reverseRepeat = argument->GetBool();

	_onFinish.clear();

	if (_parser.GetArgument(7, &argument))
		_onFinish = std::string(argument->GetString().mb_str());

	return Redo();
}

// Confirmed (asm lines 407247-407351)
bool CmdStartTween::Redo() {
	Tween tween(_from, _to, _duration, _easing, _repeat, _reverseRepeat);

	gameControl()->StartTween(tween, _target);
	return true;
}

// startObjectTween(object, field, from, to, duration, easing [, repeat [, reverseRepeat]]): changes the number (or
// the point, `from` and `to` are then points) in the field of the object (a field id, eField...) from `from` to `to`
// in `duration` milliseconds, by the easing (eEase...). The tween that works on the object already is replaced.
class CmdStartObjectTween : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	float _duration = 0.0f;
	float _from = 0.0f;
	float _to = 0.0f;
	wxPoint _fromPoint;
	wxPoint _toPoint;
	int _kind = 0;  ///< 0: a number, 1: a point
	std::function<double(double)> _easing;
	TVisObjRef _target;
	int _field = 0;
	bool _repeat = false;
	bool _reverseRepeat = false;
};

// Confirmed (asm lines 404087-404277)
static void CmdStartObjectTween_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
	syntax.AddArg(TArgType::kObject, true);
	syntax.AddArg(TArgType::kInt, true);
	syntax.AddArg(TArgType::kAny, true);
	syntax.AddArg(TArgType::kAny, true);
	syntax.AddArg(TArgType::kFloat, true);
	syntax.AddArg(TArgType::kInt, true);
	syntax.AddArg(TArgType::kBool, false);
	syntax.AddArg(TArgType::kBool, false);
}

// Confirmed (asm lines 407771-408355)
bool CmdStartObjectTween::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_target = argument->GetObject();

	if (_parser.GetArgument(1, &argument))
		_field = argument->GetInt();

	_kind = 0;

	if (_parser.GetArgument(2, &argument)) {
		if (argument->GetType() == TArgType::kFloat) {
			_from = argument->GetFloat();
		} else if (argument->GetType() == TArgType::kPoint) {
			_kind = 1;
			_fromPoint = argument->GetPoint();
		}
	}

	if (_parser.GetArgument(3, &argument)) {
		if (argument->GetType() == TArgType::kFloat)
			_to = argument->GetFloat();
		else if (argument->GetType() == TArgType::kPoint)
			_toPoint = argument->GetPoint();
	}

	if (_parser.GetArgument(4, &argument))
		_duration = argument->GetFloat();

	if (_parser.GetArgument(5, &argument)) {
		int number = argument->GetInt();

		_easing = easingByNumber(number);

		if (!_easing) {
			if (wxLog::loglevel > 0)
				wxLog::logexpanded(L"Unsupported property '%d'.", number);

			return false;
		}
	}

	_repeat = false;

	if (_parser.GetArgument(6, &argument))
		_repeat = argument->GetBool();

	_reverseRepeat = false;

	if (_parser.GetArgument(7, &argument))
		_reverseRepeat = argument->GetBool();

	return Redo();
}

// Confirmed (asm lines 407361-407761): a point is two tweens (x and y), a number only the first.
bool CmdStartObjectTween::Redo() {
	if (_kind == 1) {
		Tween x(_fromPoint.x, _toPoint.x, _duration, _easing, _repeat, _reverseRepeat);
		Tween y(_fromPoint.y, _toPoint.y, _duration, _easing, _repeat, _reverseRepeat);

		gameControl()->StartTween(TVisObjTween(x, _target, _field, y));
	} else {
		Tween x(_from, _to, _duration, _easing, _repeat, _reverseRepeat);
		Tween y(0.0, 0.0, 0.0, Easing::NoneIn, false, false);

		gameControl()->StartTween(TVisObjTween(x, _target, _field, y));
	}

	return true;
}

// ---------------------------------------------------------------------------------------------------------
// The window and the polygons

// toggleWindowMode(): changes between the window and the full screen.
class CmdToggleWindowMode : public TCommand {
public:
	bool Do() override;
	bool Redo() override;
};

// Confirmed (asm lines 404929-405056)
static void CmdToggleWindowMode_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.1"));
}

// Confirmed (asm lines 388857-388864)
bool CmdToggleWindowMode::Do() {
	return Redo();
}

// Confirmed (asm lines 388874-388886)
bool CmdToggleWindowMode::Redo() {
	graphics->ToggleWindowMode();
	return true;
}

// getWindowMode(): true when the window is a full screen one, false when it is windowed.
class CmdGetWindowMode : public TCommand {
public:
	bool Do() override;
	bool Redo() override;
};

// Confirmed (asm lines 405066-405188)
static void CmdGetWindowMode_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.1"));
}

// Confirmed (asm lines 388896-388903)
bool CmdGetWindowMode::Do() {
	return Redo();
}

// Confirmed (asm lines 388951-388967)
bool CmdGetWindowMode::Redo() {
	Result.Set(graphics->IsFullscreen());
	Result.ToLua();
	return true;
}

// setWindowSize(size): sets the size of the window (only in the windowed mode).
class CmdSetWindowSize : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxPoint _size;
};

// Confirmed (asm lines 405198-405332)
static void CmdSetWindowSize_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.1"));
	syntax.AddArg(TArgType::kPoint, true);
}

// Confirmed (asm lines 388977-389006)
bool CmdSetWindowSize::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_size = argument->GetPoint();

	return Redo();
}

// Confirmed (asm lines 388913-388924): the result is the one of the graphics.
bool CmdSetWindowSize::Redo() {
	return graphics->SetWindowSize(_size.x, _size.y);
}

// isPointInsidePolygon(point, polygon): whether the point is inside the polygon (a list of points; a field with
// several polygons, as the border of a way or the polygon of an object, has to be given one by one).
class CmdIsPointInsidePolygon : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	wxPoint _point;
	std::vector<wxPoint> _polygon;
};

// Confirmed (asm lines 404777-404919)
static void CmdIsPointInsidePolygon_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.2"));
	syntax.AddArg(TArgType::kPoint, true);
	syntax.AddArg(TArgType::kPointList, true);
}

// Confirmed (asm lines 411891-411942)
bool CmdIsPointInsidePolygon::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_point = argument->GetPoint();

	if (_parser.GetArgument(1, &argument))
		_polygon = argument->GetPointList();

	return Redo();
}

// Confirmed (asm lines 389805-389822)
bool CmdIsPointInsidePolygon::Redo() {
	Result.Set(IsPointInsidePolygon(_point, _polygon));
	Result.ToLua();
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Shaders

// shaderCompile(vsh [, fsh]): makes a shader; the result is its number (from 1, the last one made), which the
// other shader commands take. With one text the shader is made from it, a text that begins with "VSCBIN" is a
// compressed shader (base64 of the size and the zlib data).
class CmdShaderCompile : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	std::string _fragmentShader;  ///< argument 2 ("fsh")
	std::string _vertexShader;    ///< argument 1 ("vsh")
};

// Confirmed (asm lines 404287-404429)
static void CmdShaderCompile_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kString, false);
}

// Confirmed (asm lines 395824-396119)
bool CmdShaderCompile::Do() {
	const TArgument *argument = nullptr;

	if (_parser.GetArgument(0, &argument))
		_vertexShader = std::string(argument->GetString().mb_str());

	if (_parser.GetArgument(1, &argument))
		_fragmentShader = std::string(argument->GetString().mb_str());

	TShader *shader = CreateShader();

	shader_list.push_back(shader);

	if (!_fragmentShader.empty()) {
		if (shader)
			shader->Compile(_fragmentShader, _vertexShader, 0, 0);
	} else if (_vertexShader.find("VSCBIN") != std::string::npos) {
		// (the text after the first 6 characters, whatever the place of the marker; the data begin with the size)
		std::string data = base64_decode(_vertexShader.substr(6));
		bool success = false;
		std::string source;

		if (data.size() >= 4) {
			int size;

			std::memcpy(&size, data.data(), sizeof(size));

			if (size >= 0) {
				source.resize(size);

				unsigned long length = size;

				success = zlibUncompress(reinterpret_cast<unsigned char *>(&source[0]), &length,
				                         reinterpret_cast<const unsigned char *>(data.data()) + 4, data.size() - 4) == 0;
			}
		}

		if (!success) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"decompressing shader failed");

			return false;
		}

		if (shader)
			shader->CompileFromMemory(source, "memory-file");
	} else if (shader) {
		shader->Compile(_vertexShader);
	}

	return Redo();
}

// Confirmed (asm lines 389262-389292): the number of the shaders, that is the number of the new one.
bool CmdShaderCompile::Redo() {
	Result.Set(static_cast<int>(shader_list.size()));
	Result.ToLua();
	return true;
}

// shaderUniform(shader, name, value): sets a uniform of the shader (the number that shaderCompile gave). The value
// is a number (a float; an integer when the name starts with "_i_", as Lua has only numbers), a list of 2, 3 or 4
// numbers (vec2, vec3, vec4), of 9 or 16 (mat3, mat4), or, when the name starts with "_t_", the path of a texture
// (with or without "vispath:"). The prefixes are not part of the name of the uniform.
class CmdShaderUniform : public TCommand {
public:
	bool Do() override;
	bool Redo() override;
};

// Confirmed (asm lines 404616-404767)
static void CmdShaderUniform_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4."));
	syntax.AddArg(TArgType::kInt, true);
	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kAny, true);
}

// Confirmed (asm lines 411382-411881)
bool CmdShaderUniform::Do() {
	const TArgument *argument = nullptr;
	int index = -1;
	std::string name;
	bool isInteger = false;

	if (_parser.GetArgument(0, &argument))
		index = argument->GetInt() - 1;

	if (_parser.GetArgument(1, &argument)) {
		name = std::string(argument->GetString().mb_str());

		if (name.size() > 3 && name.compare(0, 3, "_i_") == 0) {
			name = name.substr(3);
			isInteger = true;
		}
	}

	_parser.GetArgument(2, &argument);

	if (index < 0 || index >= static_cast<int>(shader_list.size()))
		return Redo();

	auto it = shader_list.begin();

	std::advance(it, index);

	TShader *shader = *it;

	switch (argument->GetType()) {
	case TArgType::kString:
		if (name.size() > 3 && name.compare(0, 3, "_t_") == 0) {
			wxString path = argument->GetString();

			if (path.StartsWith(wxString(L"vispath:")))
				path = path.Mid(8, -1);

			if (shader)
				shader->SetUniformTexture(name.substr(3).c_str(), path.mb_str());
		}

		return Redo();
	case TArgType::kFloat:
		if (shader) {
			if (isInteger)
				shader->SetUniform(name.c_str(), static_cast<int>(argument->GetFloat()));
			else
				shader->SetUniform(name.c_str(), argument->GetFloat());
		}

		return Redo();
	case TArgType::kFloatList: {
		const std::vector<float> values = argument->GetFloatList();

		if (shader) {
			switch (values.size()) {
			case 2:
				shader->SetUniform(name.c_str(), values[0], values[1]);
				break;
			case 3:
				shader->SetUniform(name.c_str(), values[0], values[1], values[2]);
				break;
			case 4:
				shader->SetUniform(name.c_str(), values[0], values[1], values[2], values[3]);
				break;
			case 9:
				shader->SetUniformMatrix3(name.c_str(), values.data());
				break;
			case 16:
				shader->SetUniformMatrix4(name.c_str(), values.data());
				break;
			default:
				break;
			}
		}

		return Redo();
	}
	default:
		return false;
	}
}

// Confirmed (asm lines 388841-388847)
bool CmdShaderUniform::Redo() {
	return true;
}

// shaderSetOptions({[renderbuffers=n,] [transition=n,] {shader=, downsize=, source=, target=, comp_src=, comp_dst=,
// clear=}, ...} [, id]): sets the passes of the render configuration `id` (0 without it; one more than the ones
// that exist is a new configuration) and the number of the buffers and the transition of all of them.
class CmdShaderSetOptions : public TCommand {
public:
	bool Do() override;
	bool Redo() override;
};

// Confirmed (asm lines 404439-404585)
static void CmdShaderSetOptions_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.0.2"));
	syntax.AddArg(TArgType::kFlags, true);
	syntax.AddArg(TArgType::kInt, false);
}

// Confirmed (asm lines 425255-425813)
bool CmdShaderSetOptions::Do() {
	const TArgument *argument = nullptr;
	size_t config = 0;
	int pass = 0;

	if (_parser.GetArgument(1, &argument)) {
		// (the id leaves the stack, the table of the options is the first element of it)
		lua_remove(L, 2);
		pass = argument->GetInt();

		if (pass < 0)
			pass = 0;

		config = pass;
	}

	if (shader_renderpasses.size() == config)
		shader_renderpasses.emplace_back();

	if (pass >= static_cast<int>(shader_renderpasses.size())) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"%s %d", L"invalid pass number", pass);

		return false;
	}

	std::vector<TRenderPass> &passes = shader_renderpasses[config];

	passes.clear();
	lua_pushnil(L);

	while (lua_next(L, 1) != 0) {
		if (lua_isnumber(L, 2)) {
			// A pass: a table of numbers by name.
			TRenderPass renderPass;

			lua_pushnil(L);

			while (lua_next(L, 3) != 0) {
				if (lua_isstring(L, 4) && lua_isnumber(L, 5)) {
					double value = lua_tonumber(L, 5);
					std::string option(lua_tolstring(L, 4, nullptr));

					if (option == "shader") {
						renderPass.shader = static_cast<int>(value);
					} else if (option == "downsize") {
						renderPass.downsize = static_cast<float>(value);
					} else if (option == "source") {
						renderPass.source = static_cast<int>(value);
					} else if (option == "target") {
						renderPass.target = static_cast<int>(value);
					} else if (option == "comp_src") {
						renderPass.compSrc = static_cast<unsigned short>(CompositeEnums(static_cast<int>(value)));
					} else if (option == "comp_dst") {
						renderPass.compDst = static_cast<unsigned short>(CompositeEnums(static_cast<int>(value)));
					} else if (option == "clear") {
						renderPass.clear = static_cast<unsigned short>(static_cast<int>(value));
					} else if (wxLog::loglevel >= 0) {
						wxLog::logexpanded(L"ShaderSetOptions: %s option not recognized", wxString(option.c_str()).wc_str());
					}
				} else if (wxLog::loglevel >= 0) {
					wxLog::logexpanded(L"ShaderSetOptions: An option has not the format string=integer");
				}

				lua_settop(L, -2);
			}

			passes.push_back(renderPass);
		} else if (lua_isstring(L, 2) && lua_isnumber(L, 3)) {
			// An option of all the configurations.
			int value = static_cast<int>(lua_tonumber(L, 3));
			std::string option(lua_tolstring(L, 2, nullptr));

			if (option == "renderbuffers")
				shader_buffers = value;
			else if (option == "transition")
				shader_transition = value;
		} else if (wxLog::loglevel >= 0) {
			wxLog::logexpanded(L"ShaderSetOptions: An option has not the format string=integer");
		}

		lua_settop(L, -2);
	}

	return Redo();
}

// Confirmed (asm lines 388824-388831)
bool CmdShaderSetOptions::Redo() {
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Haptic feedback (the rumble and the effects of the controllers)

// Confirmed (asm lines 396734-397204 and the ones of the other effects): the name of the direction of an effect,
// "none" (0), "polar" (1), "cartesian" (2) or "spherical" (3); any other text leaves the direction as it was.
static void hapticDirection(const wxString &name, int &direction) {
	const std::wstring text = name.ToStdWstring();

	if (text == L"none")
		direction = 0;
	else if (text == L"polar")
		direction = TCED_Polar;
	else if (text == L"cartesian")
		direction = TCED_Cartesian;
	else if (text == L"spherical")
		direction = TCED_Spherical;
}

static void hapticUnsupported(const wxString &type) {
	if (wxLog::loglevel > 0)
		wxLog::logexpanded(L"Unsupported property '%s'.", type.wc_str());
}

// startHapticRumble(strength, length): a simple rumble on the controllers; the strength is 0 to 100, the length is in
// milliseconds (-1 for ever). The result tells whether it was done.
class CmdStartHapticRumble : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _strength = -1;
	int _length = 0;
};

// Confirmed (asm lines 405342-405488)
static void CmdStartHapticRumble_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.3"));
	syntax.AddArg(TArgType::kInt, true);
	syntax.AddArg(TArgType::kInt, true);
}

// Confirmed (asm lines 391532-391625): a strength above 200 (or below 0) is refused.
bool CmdStartHapticRumble::Do() {
	const TArgument *argument = nullptr;

	_strength = -1;
	_length = 0;

	if (_parser.GetArgument(0, &argument)) {
		_strength = argument->GetInt();

		if (static_cast<unsigned int>(_strength) > 200) {
			if (wxLog::loglevel > 0)
				wxLog::logexpanded(L"Strength is out of range.");

			return false;
		}
	}

	if (_parser.GetArgument(1, &argument))
		_length = argument->GetInt();

	return Redo();
}

// Confirmed (asm lines 389766-389795): the strength goes to the controller as a part of 1 (strength / 100).
bool CmdStartHapticRumble::Redo() {
	bool started = gameControl()->GetGameController()->HapticStartRumble(static_cast<float>(_strength) / 100.0f,
	    static_cast<unsigned int>(_length));

	Result.Set(started);
	Result.ToLua();
	return true;
}

// stopHapticRumble(): stops the rumble of startHapticRumble. The result tells whether it was done.
class CmdStopHapticRumble : public TCommand {
public:
	bool Do() override;
	bool Redo() override;
};

// Confirmed (asm lines 405498-405628)
static void CmdStopHapticRumble_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.3"));
}

// Confirmed (asm lines 388934-388941)
bool CmdStopHapticRumble::Do() {
	return Redo();
}

// Confirmed (asm lines 389738-389756)
bool CmdStopHapticRumble::Redo() {
	Result.Set(gameControl()->GetGameController()->HapticStopRumble());
	Result.ToLua();
	return true;
}

// startHapticEffect(effectID): starts an effect that createHapticEffect... made. The result tells whether it was done.
class CmdStartHapticEffect : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _effect = -1;
};

// Confirmed (asm lines 405638-405776)
static void CmdStartHapticEffect_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.3"));
	syntax.AddArg(TArgType::kInt, true);
}

// Confirmed (asm lines 389094-389124)
bool CmdStartHapticEffect::Do() {
	const TArgument *argument = nullptr;

	_effect = -1;

	if (_parser.GetArgument(0, &argument))
		_effect = argument->GetInt();

	return Redo();
}

// Confirmed (asm lines 389703-389728): -1 (no effect) is not started, and tells that it was not done.
bool CmdStartHapticEffect::Redo() {
	bool started = false;

	if (_effect != -1)
		started = gameControl()->GetGameController()->HapticStartEffect(_effect);

	Result.Set(started);
	Result.ToLua();
	return true;
}

// stopHapticEffect(effectID): stops an effect (-1: all of them). The result tells whether it was done.
class CmdStopHapticEffect : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _effect = -1;
};

// Confirmed (asm lines 405786-405924)
static void CmdStopHapticEffect_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.3"));
	syntax.AddArg(TArgType::kInt, true);
}

// Confirmed (asm lines 389055-389084)
bool CmdStopHapticEffect::Do() {
	const TArgument *argument = nullptr;

	_effect = -1;

	if (_parser.GetArgument(0, &argument))
		_effect = argument->GetInt();

	return Redo();
}

// Confirmed (asm lines 389661-389693)
bool CmdStopHapticEffect::Redo() {
	TGameController *controller = gameControl()->GetGameController();
	bool stopped;

	if (_effect != -1)
		stopped = controller->HapticStopEffect(_effect);
	else
		stopped = controller->HapticStopAll();

	Result.Set(stopped);
	Result.ToLua();
	return true;
}

// createHapticEffectConstant(type, length, delay, level, attackLength, attackLevel, fadeLength, fadeLevel,
// direction [, dir0 [, dir1 [, dir2]]]): makes a constant effect ("constant") on the controllers; the result is the
// number of the effect for startHapticEffect, -1 if no controller supports it. `direction` is "none", "polar",
// "cartesian" or "spherical" (with one, three or two of the numbers dir0 to dir2).
class CmdCreateHapticEffectConstant : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _type = 0;
	int _direction = 0;
	int _length = -1;
	int _delay = -1;
	int _level = -1;
	int _attackLength = -1;
	int _attackLevel = -1;
	int _fadeLength = -1;
	int _fadeLevel = -1;
	int _dir[3] = {0, 0, 0};
};

// Confirmed (asm lines 405934-406160)
static void CmdCreateHapticEffectConstant_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.3"));
	syntax.AddArg(TArgType::kString, true);

	for (int i = 0; i < 7; i++)
		syntax.AddArg(TArgType::kInt, true);

	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kInt, false);
	syntax.AddArg(TArgType::kInt, false);
	syntax.AddArg(TArgType::kInt, false);
}

// Confirmed (asm lines 396734-397204)
bool CmdCreateHapticEffectConstant::Do() {
	const TArgument *argument = nullptr;
	wxString type;
	wxString direction;

	_length = _delay = _level = _attackLength = _attackLevel = _fadeLength = _fadeLevel = -1;
	_dir[0] = _dir[1] = _dir[2] = 0;

	if (_parser.GetArgument(0, &argument))
		type = argument->GetString();

	if (type.ToStdWstring() != L"constant") {
		hapticUnsupported(type);
		return false;
	}

	_type = TCET_Constant;

	int *const members[] = {&_length, &_delay, &_level, &_attackLength, &_attackLevel, &_fadeLength, &_fadeLevel};

	for (int i = 0; i < 7; i++) {
		if (_parser.GetArgument(1 + i, &argument))
			*members[i] = argument->GetInt();
	}

	if (_parser.GetArgument(8, &argument))
		direction = argument->GetString();

	hapticDirection(direction, _direction);

	for (int i = 0; i < 3; i++) {
		if (_parser.GetArgument(9 + i, &argument))
			_dir[i] = argument->GetInt();
	}

	return Redo();
}

// Confirmed (asm lines 389574-389651)
bool CmdCreateHapticEffectConstant::Redo() {
	int effect = gameControl()->GetGameController()->HapticNewEffectConstant(
	                 static_cast<TControllerEffectType>(_type), static_cast<TControllerEffectDirection>(_direction), _dir,
	                 _length, _delay, _level, _attackLength, _attackLevel, _fadeLength, _fadeLevel);

	Result.Set(effect);
	Result.ToLua();
	return true;
}

// createHapticEffectPeriodic(type, length, delay, period, magnitude, offset, phase, attackLength, attackLevel,
// fadeLength, fadeLevel, direction [, dir0 [, dir1 [, dir2]]]): makes a periodic effect ("sine", "leftright",
// "triangle", "sawtoothup" or "sawtoothdown"); see createHapticEffectConstant.
class CmdCreateHapticEffectPeriodic : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _type = 0;
	int _direction = 0;
	int _length = -1;
	int _delay = -1;
	int _period = -1;
	int _magnitude = -1;
	int _offset = -1;
	int _phase = -1;
	int _attackLength = -1;
	int _attackLevel = -1;
	int _fadeLength = -1;
	int _fadeLevel = -1;
	int _dir[3] = {0, 0, 0};
};

// Confirmed (asm lines 406170-406420)
static void CmdCreateHapticEffectPeriodic_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.3"));
	syntax.AddArg(TArgType::kString, true);

	for (int i = 0; i < 10; i++)
		syntax.AddArg(TArgType::kInt, true);

	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kInt, false);
	syntax.AddArg(TArgType::kInt, false);
	syntax.AddArg(TArgType::kInt, false);
}

// Confirmed (asm lines 393624-394295)
bool CmdCreateHapticEffectPeriodic::Do() {
	static const struct {
		const wchar_t *name;
		TControllerEffectType type;
	} kTypes[] = {
		{L"sine", TCET_Sine},
		{L"leftright", TCET_LeftRight},
		{L"triangle", TCET_Triangle},
		{L"sawtoothup", TCET_SawtoothUp},
		{L"sawtoothdown", TCET_SawtoothDown}
	};
	const TArgument *argument = nullptr;
	wxString type;
	wxString direction;
	bool known = false;

	_length = _delay = _period = _magnitude = _offset = _phase = -1;
	_attackLength = _attackLevel = _fadeLength = _fadeLevel = -1;
	_dir[0] = _dir[1] = _dir[2] = 0;

	if (_parser.GetArgument(0, &argument))
		type = argument->GetString();

	for (const auto &entry : kTypes) {
		if (type.ToStdWstring() == entry.name) {
			_type = entry.type;
			known = true;
			break;
		}
	}

	if (!known) {
		hapticUnsupported(type);
		return false;
	}

	int *const members[] = {&_length, &_delay, &_period, &_magnitude, &_offset, &_phase, &_attackLength, &_attackLevel,
	                        &_fadeLength, &_fadeLevel
	                       };

	for (int i = 0; i < 10; i++) {
		if (_parser.GetArgument(1 + i, &argument))
			*members[i] = argument->GetInt();
	}

	if (_parser.GetArgument(11, &argument))
		direction = argument->GetString();

	hapticDirection(direction, _direction);

	for (int i = 0; i < 3; i++) {
		if (_parser.GetArgument(12 + i, &argument))
			_dir[i] = argument->GetInt();
	}

	return Redo();
}

// Confirmed (asm lines 389470-389564)
bool CmdCreateHapticEffectPeriodic::Redo() {
	int effect = gameControl()->GetGameController()->HapticNewEffectPeriodic(
	                 static_cast<TControllerEffectType>(_type), static_cast<TControllerEffectDirection>(_direction), _dir,
	                 _length, _delay, _period, _magnitude, _offset, _phase, _attackLength, _attackLevel, _fadeLength,
	                 _fadeLevel);

	Result.Set(effect);
	Result.ToLua();
	return true;
}

// createHapticEffectCondition(type, length, delay, rightSat, leftSat, rightCoeff, leftCoeff, deadband, center):
// makes a condition effect ("spring", "damper", "inertia" or "friction"); see createHapticEffectConstant.
class CmdCreateHapticEffectCondition : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _type = 0;
	int _length = -1;
	int _delay = -1;
	int _rightSat = -1;
	int _leftSat = -1;
	int _rightCoeff = -1;
	int _leftCoeff = -1;
	int _deadband = -1;
	int _center = -1;
};

// Confirmed (asm lines 406430-406632)
static void CmdCreateHapticEffectCondition_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.3"));
	syntax.AddArg(TArgType::kString, true);

	for (int i = 0; i < 8; i++)
		syntax.AddArg(TArgType::kInt, true);
}

// Confirmed (asm lines 393257-393616)
bool CmdCreateHapticEffectCondition::Do() {
	static const struct {
		const wchar_t *name;
		TControllerEffectType type;
	} kTypes[] = {
		{L"spring", TCET_Spring},
		{L"damper", TCET_Damper},
		{L"inertia", TCET_Inertia},
		{L"friction", TCET_Friction}
	};
	const TArgument *argument = nullptr;
	wxString type;
	bool known = false;

	_length = _delay = _rightSat = _leftSat = _rightCoeff = _leftCoeff = _deadband = _center = -1;

	if (_parser.GetArgument(0, &argument))
		type = argument->GetString();

	for (const auto &entry : kTypes) {
		if (type.ToStdWstring() == entry.name) {
			_type = entry.type;
			known = true;
			break;
		}
	}

	if (!known) {
		hapticUnsupported(type);
		return false;
	}

	int *const members[] = {&_length, &_delay, &_rightSat, &_leftSat, &_rightCoeff, &_leftCoeff, &_deadband, &_center};

	for (int i = 0; i < 8; i++) {
		if (_parser.GetArgument(1 + i, &argument))
			*members[i] = argument->GetInt();
	}

	return Redo();
}

// Confirmed (asm lines 389394-389460)
bool CmdCreateHapticEffectCondition::Redo() {
	int effect = gameControl()->GetGameController()->HapticNewEffectCondition(
	                 static_cast<TControllerEffectType>(_type), _length, _delay, _rightSat, _leftSat, _rightCoeff,
	                 _leftCoeff, _deadband, _center);

	Result.Set(effect);
	Result.ToLua();
	return true;
}

// createHapticEffectRamp(type, length, delay, start, end, attackLength, attackLevel, fadeLength, fadeLevel,
// direction [, dir0 [, dir1 [, dir2]]]): makes a ramp effect ("ramp"); see createHapticEffectConstant.
class CmdCreateHapticEffectRamp : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _type = 0;
	int _direction = 0;
	int _length = -1;
	int _delay = -1;
	int _start = -1;
	int _end = -1;
	int _attackLength = -1;
	int _attackLevel = -1;
	int _fadeLength = -1;
	int _fadeLevel = -1;
	int _dir[3] = {0, 0, 0};
};

// Confirmed (asm lines 406642-406877)
static void CmdCreateHapticEffectRamp_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.3"));
	syntax.AddArg(TArgType::kString, true);

	for (int i = 0; i < 8; i++)
		syntax.AddArg(TArgType::kInt, true);

	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kInt, false);
	syntax.AddArg(TArgType::kInt, false);
	syntax.AddArg(TArgType::kInt, false);
}

// Confirmed (asm lines 396240-396724)
bool CmdCreateHapticEffectRamp::Do() {
	const TArgument *argument = nullptr;
	wxString type;
	wxString direction;

	_length = _delay = _start = _end = _attackLength = _attackLevel = _fadeLength = _fadeLevel = -1;
	_dir[0] = _dir[1] = _dir[2] = 0;

	if (_parser.GetArgument(0, &argument))
		type = argument->GetString();

	if (type.ToStdWstring() != L"ramp") {
		hapticUnsupported(type);
		return false;
	}

	_type = TCET_Ramp;

	int *const members[] = {&_length, &_delay, &_start, &_end, &_attackLength, &_attackLevel, &_fadeLength, &_fadeLevel};

	for (int i = 0; i < 8; i++) {
		if (_parser.GetArgument(1 + i, &argument))
			*members[i] = argument->GetInt();
	}

	if (_parser.GetArgument(9, &argument))
		direction = argument->GetString();

	hapticDirection(direction, _direction);

	for (int i = 0; i < 3; i++) {
		if (_parser.GetArgument(10 + i, &argument))
			_dir[i] = argument->GetInt();
	}

	return Redo();
}

// Confirmed (asm lines 389302-389384)
bool CmdCreateHapticEffectRamp::Redo() {
	int effect = gameControl()->GetGameController()->HapticNewEffectRamp(
	                 static_cast<TControllerEffectType>(_type), static_cast<TControllerEffectDirection>(_direction), _dir,
	                 _length, _delay, _start, _end, _attackLength, _attackLevel, _fadeLength, _fadeLevel);

	Result.Set(effect);
	Result.ToLua();
	return true;
}

// createHapticEffectLeftRight(type, length, largeMagnitude, smallMagnitude): makes an effect that drives the large and
// the small motor of a controller ("leftright"); see createHapticEffectConstant.
class CmdCreateHapticEffectLeftRight : public TCommand {
public:
	bool Do() override;
	bool Redo() override;

	int _type = 0;
	int _length = -1;
	int _largeMagnitude = -1;
	int _smallMagnitude = -1;
};

// Confirmed (asm lines 406887-407049)
static void CmdCreateHapticEffectLeftRight_GetSyntax(TArgSyntax &syntax) {
	syntax.SetMinVersion(wxString(L"4.3"));
	syntax.AddArg(TArgType::kString, true);
	syntax.AddArg(TArgType::kInt, true);
	syntax.AddArg(TArgType::kInt, true);
	syntax.AddArg(TArgType::kInt, true);
}

// Confirmed (asm lines 393043-393247)
bool CmdCreateHapticEffectLeftRight::Do() {
	const TArgument *argument = nullptr;
	wxString type;

	_length = _largeMagnitude = _smallMagnitude = -1;

	if (_parser.GetArgument(0, &argument))
		type = argument->GetString();

	if (type.ToStdWstring() != L"leftright") {
		hapticUnsupported(type);
		return false;
	}

	_type = TCET_LeftRight;

	if (_parser.GetArgument(1, &argument))
		_length = argument->GetInt();

	if (_parser.GetArgument(2, &argument))
		_largeMagnitude = argument->GetInt();

	if (_parser.GetArgument(3, &argument))
		_smallMagnitude = argument->GetInt();

	return Redo();
}

// Confirmed (asm lines 389212-389252)
bool CmdCreateHapticEffectLeftRight::Redo() {
	int effect = gameControl()->GetGameController()->HapticNewEffectLeftRight(
	                 static_cast<TControllerEffectType>(_type), _length, _largeMagnitude, _smallMagnitude);

	Result.Set(effect);
	Result.ToLua();
	return true;
}

// ---------------------------------------------------------------------------------------------------------
// Registration

static int lua_CmdStartAction(lua_State *state) {
	return RunCommand<CmdStartAction>(state, "startAction");
}

static int lua_CmdStopAction(lua_State *state) {
	return RunCommand<CmdStopAction>(state, "stopAction");
}

static int lua_CmdStartAnimation(lua_State *state) {
	return RunCommand<CmdStartAnimation>(state, "startAnimation");
}

static int lua_CmdStopAnimation(lua_State *state) {
	return RunCommand<CmdStopAnimation>(state, "stopAnimation");
}

static int lua_CmdGetVolume(lua_State *state) {
	return RunCommand<CmdGetVolume>(state, "getVolume");
}

static int lua_CmdSetVolume(lua_State *state) {
	return RunCommand<CmdSetVolume>(state, "setVolume");
}

static int lua_CmdGetCursorPos(lua_State *state) {
	return RunCommand<CmdGetCursorPos>(state, "getCursorPos");
}

static int lua_CmdSetCursorPos(lua_State *state) {
	return RunCommand<CmdSetCursorPos>(state, "setCursorPos");
}

void Register_CmdStartAction() {
	RegisterCommandFunction("startAction", lua_CmdStartAction, CmdStartAction_GetSyntax);
}

void Register_CmdStopAction() {
	RegisterCommandFunction("stopAction", lua_CmdStopAction, CmdStopAction_GetSyntax);
}

void Register_CmdStartAnimation() {
	RegisterCommandFunction("startAnimation", lua_CmdStartAnimation, CmdStartAnimation_GetSyntax);
}

void Register_CmdStopAnimation() {
	RegisterCommandFunction("stopAnimation", lua_CmdStopAnimation, CmdStopAnimation_GetSyntax);
}

void Register_CmdGetVolume() {
	RegisterCommandFunction("getVolume", lua_CmdGetVolume, CmdGetVolume_GetSyntax);
}

void Register_CmdSetVolume() {
	RegisterCommandFunction("setVolume", lua_CmdSetVolume, CmdSetVolume_GetSyntax);
}

void Register_CmdGetCursorPos() {
	RegisterCommandFunction("getCursorPos", lua_CmdGetCursorPos, CmdGetCursorPos_GetSyntax);
}

void Register_CmdSetCursorPos() {
	RegisterCommandFunction("setCursorPos", lua_CmdSetCursorPos, CmdSetCursorPos_GetSyntax);
}

static int lua_CmdRegisterEventHandler(lua_State *state) {
	return RunCommand<CmdRegisterEventHandler>(state, "registerEventHandler");
}

static int lua_CmdUnregisterEventHandler(lua_State *state) {
	return RunCommand<CmdUnregisterEventHandler>(state, "unregisterEventHandler");
}

static int lua_CmdRegisterHookFunction(lua_State *state) {
	return RunCommand<CmdRegisterHookFunction>(state, "registerHookFunction");
}

static int lua_CmdCreateScreenshot(lua_State *state) {
	return RunCommand<CmdCreateScreenshot>(state, "createScreenshot");
}

static int lua_CmdCreateEvent(lua_State *state) {
	return RunCommand<CmdCreateEvent>(state, "createEvent");
}

void Register_CmdRegisterEventHandler() {
	RegisterCommandFunction("registerEventHandler", lua_CmdRegisterEventHandler, CmdRegisterEventHandler_GetSyntax);
}

void Register_CmdUnregisterEventHandler() {
	RegisterCommandFunction("unregisterEventHandler", lua_CmdUnregisterEventHandler, CmdUnregisterEventHandler_GetSyntax);
}

void Register_CmdRegisterHookFunction() {
	RegisterCommandFunction("registerHookFunction", lua_CmdRegisterHookFunction, CmdRegisterHookFunction_GetSyntax);
}

void Register_CmdCreateScreenshot() {
	RegisterCommandFunction("createScreenshot", lua_CmdCreateScreenshot, CmdCreateScreenshot_GetSyntax);
}

void Register_CmdCreateEvent() {
	RegisterCommandFunction("createEvent", lua_CmdCreateEvent, CmdCreateEvent_GetSyntax);
}

#define COMMAND_FUNCTION(Name, name) \
	static int lua_Cmd##Name(lua_State *state) { \
		return RunCommand<Cmd##Name>(state, name); \
	} \
	void Register_Cmd##Name() { \
		RegisterCommandFunction(name, lua_Cmd##Name, Cmd##Name##_GetSyntax); \
	}

COMMAND_FUNCTION(StartSound, "startSound")
COMMAND_FUNCTION(StopSound, "stopSound")
COMMAND_FUNCTION(GetSoundId, "getSoundId")
COMMAND_FUNCTION(ToggleSoundPause, "toggleSoundPause")
COMMAND_FUNCTION(GetSoundProperty, "getSoundProperty")
COMMAND_FUNCTION(SetSoundProperty, "setSoundProperty")
COMMAND_FUNCTION(InitGameClient, "initGameClient")
COMMAND_FUNCTION(GetGameClientStat, "getGameClientStat")
COMMAND_FUNCTION(SetGameClientStat, "setGameClientStat")
COMMAND_FUNCTION(ResetGameClientStats, "resetGameClientStats")
COMMAND_FUNCTION(GetGameClientAchievement, "getGameClientAchievement")
COMMAND_FUNCTION(SetGameClientAchievement, "setGameClientAchievement")
COMMAND_FUNCTION(GetProperty, "getProperty")
COMMAND_FUNCTION(StartDefaultBrowser, "startDefaultBrowser")
COMMAND_FUNCTION(SetWindowTitle, "setWindowTitle")
COMMAND_FUNCTION(GetWindowBrightness, "getWindowBrightness")
COMMAND_FUNCTION(SetWindowBrightness, "setWindowBrightness")
COMMAND_FUNCTION(ReplaceGame, "replaceGame")
COMMAND_FUNCTION(StartTween, "startTween")
COMMAND_FUNCTION(StartObjectTween, "startObjectTween")
COMMAND_FUNCTION(ToggleWindowMode, "toggleWindowMode")
COMMAND_FUNCTION(GetWindowMode, "getWindowMode")
COMMAND_FUNCTION(SetWindowSize, "setWindowSize")
COMMAND_FUNCTION(IsPointInsidePolygon, "isPointInsidePolygon")
COMMAND_FUNCTION(ShaderCompile, "shaderCompile")
COMMAND_FUNCTION(ShaderUniform, "shaderUniform")
COMMAND_FUNCTION(ShaderSetOptions, "shaderSetOptions")
COMMAND_FUNCTION(StartHapticRumble, "startHapticRumble")
COMMAND_FUNCTION(StopHapticRumble, "stopHapticRumble")
COMMAND_FUNCTION(StartHapticEffect, "startHapticEffect")
COMMAND_FUNCTION(StopHapticEffect, "stopHapticEffect")
COMMAND_FUNCTION(CreateHapticEffectConstant, "createHapticEffectConstant")
COMMAND_FUNCTION(CreateHapticEffectPeriodic, "createHapticEffectPeriodic")
COMMAND_FUNCTION(CreateHapticEffectCondition, "createHapticEffectCondition")
COMMAND_FUNCTION(CreateHapticEffectRamp, "createHapticEffectRamp")
COMMAND_FUNCTION(CreateHapticEffectLeftRight, "createHapticEffectLeftRight")

// Confirmed (asm lines 425066-425195): the commands of the scripts and then the ones of the player, in this order.
// TODO: InitDrawLua() (the drawing functions of the scripts, graphics_*, sprite_*, ...) and the Lua libraries that
// the original has built in (luaopen_utf8, luaopen_luacurl, luaopen_rex_pcre and luaopen_lfs) are not reconstructed.
void InitPlayerCommands(TVisionaireGame *game, const wxString &appDir, const wxString &resourcesDir,
                        const wxString &unused) {
	InitCommands(game, appDir, resourcesDir, unused);

	Register_CmdStartAction();
	Register_CmdStopAction();
	Register_CmdStartAnimation();
	Register_CmdStopAnimation();
	Register_CmdGetVolume();
	Register_CmdSetVolume();
	Register_CmdGetCursorPos();
	Register_CmdSetCursorPos();
	Register_CmdCreateScreenshot();
	Register_CmdRegisterEventHandler();
	Register_CmdUnregisterEventHandler();
	Register_CmdRegisterHookFunction();
	Register_CmdInitGameClient();
	Register_CmdGetGameClientStat();
	Register_CmdSetGameClientStat();
	Register_CmdResetGameClientStats();
	Register_CmdGetGameClientAchievement();
	Register_CmdSetGameClientAchievement();
	Register_CmdStartDefaultBrowser();
	Register_CmdGetProperty();
	Register_CmdStartSound();
	Register_CmdStopSound();
	Register_CmdGetSoundId();
	Register_CmdGetSoundProperty();
	Register_CmdSetSoundProperty();
	Register_CmdToggleSoundPause();
	Register_CmdSetWindowTitle();
	Register_CmdGetWindowBrightness();
	Register_CmdSetWindowBrightness();
	Register_CmdReplaceGame();
	Register_CmdStartObjectTween();
	Register_CmdStartTween();
	Register_CmdShaderCompile();
	Register_CmdShaderUniform();
	Register_CmdShaderSetOptions();
	Register_CmdIsPointInsidePolygon();
	Register_CmdToggleWindowMode();
	Register_CmdGetWindowMode();
	Register_CmdSetWindowSize();
	Register_CmdStartHapticRumble();
	Register_CmdStopHapticRumble();
	Register_CmdStartHapticEffect();
	Register_CmdStopHapticEffect();
	Register_CmdCreateHapticEffectConstant();
	Register_CmdCreateHapticEffectPeriodic();
	Register_CmdCreateHapticEffectCondition();
	Register_CmdCreateHapticEffectRamp();
	Register_CmdCreateHapticEffectLeftRight();
	Register_CmdCreateEvent();
	StoreNamesOfInternalGlobalVars();
}

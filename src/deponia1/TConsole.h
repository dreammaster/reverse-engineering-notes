// Not yet assert-confirmed to a specific file; stays at the top level.
// Embedded by value in TGameControl (confirmed: TGameControl's ctor calls
// TConsole::TConsole() directly on an interior pointer). The in-game/dev
// console, matching TGameControl::GetConsole()/DisplayConsole()/
// DisplayInSceneConsole().
//
// Draw()/DrawInScene() are confirmed as tail-called directly from
// TGameControl::DisplayConsole()/DisplayInSceneConsole() (Deponia_Linux.asm
// lines 455750-455779) - only the fact that they exist and are called with
// no arguments is confirmed, not their internal drawing logic.
#pragma once

#include "WxStub.h"
#include "vsplayer/control/masterControl.h"

class TConsole {
public:
	TConsole() = default;

	bool Draw();
	void DrawInScene();
	// Confirmed call shape only (TGameControl::Init, asm lines 467226-
	// 467624) - not reversed beyond that.
	void Init();
	// Confirmed call shape only (TGameControl::HandleKeyEvent,
	// Deponia_Linux.asm lines 471037-471050) - returns whether the console
	// consumed the event, not reversed beyond that call shape.
	bool HandleKeyEvent(TKeyboardMessageEnum msg, wxString name, int a, unsigned short b);
};

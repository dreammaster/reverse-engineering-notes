#pragma once

#include <string>

#include "WxStub.h"

// x_assert(condition, expressionText, fileText, line) - used throughout the
// engine as its assert macro; call sites embed __FILE__/__LINE__/the
// stringified expression (which is how we recovered several original source
// paths, e.g. "vsplayer/control/gameController.cpp" - see NOTES.md).
void x_assert(bool condition, const char *expression, const char *file, int line);

// A named profiling/instrumentation region marker - confirmed call shapes
// only (TGameControl::LoadAndInitGame, Deponia_Linux.asm lines 467759-467762,
// 468880), bracketing the TVisionaireGame::LoadDataGame call with the fixed
// label "Struktur" (German for "structure"). Modeled as static entry points
// since no TDiagnostic object is ever constructed at either call site.
class TDiagnostic {
public:
	static void BeginFixedRegion(wxString name);
	static void EndFixedRegion();
};

// A second, distinct network-facing profiler (TGameControl::Update,
// Deponia_Linux.asm - used throughout to bracket named per-frame sections:
// "Animations", "Texts", "DeleteActions", a sprite-creation area, "Tweens").
// Unlike TDiagnostic above, this one is a real instance (the global
// `debugger` below), takes a ProfileArea enum plus a name and a frame
// number, and - going by the name - streams timing data to an external
// debugger/profiler over the network; no gameplay-visible effect, so
// stubbed as a pure no-op rather than reversed further.
enum class ProfileArea {
	kValue1 = 1, // used once, bracketing sprite/picture creation
	kValue2 = 2, // the Lua scripts (visLua.cpp)
	kValue3 = 3, // drawing a frame (ShowFrame)
	kValue4 = 4, // used for every other observed section
};

class TCPDebuggerClient {
public:
	void BeginArea(ProfileArea area, const std::string &name, int frame);
	void EndArea(ProfileArea area, int frame);
	void NextFrame();
	/** Confirmed call shape only (Init(), asm 494700): connects to the debugger at `address`:`port`. The network debugger is
	 *  not reconstructed (TODO.md), so nothing connects. */
	void Activate(const char *address, int port);
};

extern TCPDebuggerClient debugger;

// Confirmed recovered globals that the command line of the player sets (Init(), asm 493841 and 495185-495240): the
// address and port of the debugger (`-deb host:port`), and whether the profiler is on (`-prof lua` for the Lua profile,
// `-prof frame` for the areas of a frame; both need an address).
extern std::string debugger_addr;
extern int debugger_port;
extern bool profile;
extern bool profileAreas;

/** Confirmed call shape only (CleanUp(), asm 126189-127244): writes what the profiler has collected when the player ends.
 *  The profiler is not reconstructed (TODO.md), so this does nothing. */
void collectProfileData();

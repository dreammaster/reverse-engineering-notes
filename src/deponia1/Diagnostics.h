#pragma once

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

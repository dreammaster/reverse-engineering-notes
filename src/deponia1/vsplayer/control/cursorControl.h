// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vsplayer/control/cursorControl.cpp - see manifest/source_layout.tsv.
#pragma once

#include "TPaintControl.h"
#include "WxStub.h"

class TCursorControl : public TPaintControl {
public:
	wxPoint GetPositionNextToCursor() const;

	// Confirmed two distinct overloads (TGameControl::StartDialog/EndDialog,
	// Deponia_Linux.asm lines 460983-461192) - purpose of the extra bool
	// params not resolved.
	void SetCursor(int cursorId, bool flag);
	void SetCursor(bool flag1, int cursorId, bool flag2);
	// Confirmed call shape only (TGameControl::ReplaceGame, Deponia_Linux.asm
	// line 468779) - not reversed beyond that.
	void Clear();
};

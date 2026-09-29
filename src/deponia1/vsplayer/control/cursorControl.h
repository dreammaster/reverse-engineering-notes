// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vsplayer/control/cursorControl.cpp - see manifest/source_layout.tsv.
#pragma once

#include "TPaintControl.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"

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
	// Confirmed call shapes only (TGameControl::LoadAndInitGame,
	// Deponia_Linux.asm lines 467983-467985, 468050-468065): LoadCursor is
	// called once per non-empty element of a field-0xF list; LinkButtonCursor
	// takes two packed 3-byte ids (the same packing PackVisId() in
	// gameControl.cpp uses) - first the linked object's, then the object's
	// own. Neither reversed beyond that call shape.
	void LoadCursor(const TVisObjRef &cursor);
	void LinkButtonCursor(int linkedId, int objectId);
	// Confirmed call shape only (TGameControl::HandleMouseMove, Deponia_Linux.
	// asm line 472146) - not reversed beyond that.
	void SetCursorPosition(int x, int y);
	// Confirmed call shape only (TGameControl::HandleMouseUp, Deponia_Linux.
	// asm line 472904) - not reversed beyond that.
	bool IsActiveMoveObject() const;
};

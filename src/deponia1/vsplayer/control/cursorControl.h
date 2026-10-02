// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vsplayer/control/cursorControl.cpp - see manifest/source_layout.tsv.
//
// Confirmed in full except GetSCursor() (Deponia_Linux.asm lines
// 483805-485469): a TPaintControl that independently implements the real,
// named TAnimationOwner interface (confirmed via its own `_ZThn72_`-style
// this-adjusting thunks - see TAnimationOwner.h) for its own cursor
// animation; owns a loaded table of SCursor entries (the per-cursor
// down/up images and linked button ids), tracks which one is currently
// playing, and can hold a TGItem being dragged (drawn/positioned at the
// cursor instead of its own spot while held).
//
// GetSCursor() builds a new SCursor's images and linked-ids list by name-
// based resource lookup (string concatenation against two unidentified
// wide-string table suffixes, plus a virtual TManagedObject-family call at
// vtable slot 0xB0 - the same slot left ambiguous by identical-code-folding
// on TManagedObject.h's own cross-check); left as a confirmed-call-shape
// stub. LoadCursor()/LinkButtonCursor() - the two real entry points that
// use it - are both implemented in full around that one stub, since their
// own dedup/lookup logic doesn't depend on what GetSCursor() actually
// fills in.
#pragma once

#include <vector>

#include "SCursor.h"
#include "TAnimationOwner.h"
#include "TPaintControl.h"
#include "WxStub.h"
#include "datastruct/visobjref.h"
#include "vscommon/scripting/id.h"

class TGAnimation;
class TGItem;

class TCursorControl : public TPaintControl, public TAnimationOwner {
public:
	~TCursorControl() override;

	wxPoint GetPositionNextToCursor() const;

	// Confirmed two distinct overloads (TGameControl::StartDialog/EndDialog,
	// Deponia_Linux.asm lines 460983-461192) - purpose of the extra bool
	// params not resolved. Confirmed in full here regardless: `activate`
	// picks which overload's search strategy (SetCursor(int,bool) looks up
	// the currently-active entry's own id and forwards into this one);
	// `byId` picks id-match vs linked-id-match; `useActiveImage` selects
	// which of the found entry's two images to show.
	void SetCursor(bool useActiveImage, int cursorId, bool byId);
	// Confirmed (Deponia_Linux.asm lines 623550-623579): forwards into the
	// overload above, reusing whatever _activeCursor's own `active` flag
	// currently is as `useActiveImage` (false if there is no active entry).
	void SetCursor(int cursorId, bool byId);
	// Confirmed call shape only (TGameControl::ReplaceGame, Deponia_Linux.asm
	// line 468779) - not reversed beyond that.
	void Clear();
	// Confirmed call shapes only (TGameControl::LoadAndInitGame,
	// Deponia_Linux.asm lines 467983-467985, 468050-468065): LoadCursor is
	// called once per non-empty element of a field-0xF list; LinkButtonCursor
	// takes two packed 3-byte ids (the same packing PackVisId() in
	// gameControl.cpp uses) - confirmed in full now: the first is the id of
	// the cursor entry to modify, the second the id to add to its own
	// linked-ids list (the manifest's original "linked object's, then the
	// object's own" guess had the two swapped).
	void LoadCursor(const TVisObjRef &cursor);
	void LinkButtonCursor(int cursorId, int linkedId);
	// Confirmed call shape only (TGameControl::HandleMouseMove, Deponia_Linux.
	// asm line 472146) - not reversed beyond that.
	void SetCursorPosition(int x, int y);
	// Confirmed call shape only (TGameControl::HandleMouseUp, Deponia_Linux.
	// asm line 472904) - not reversed beyond that.
	bool IsActiveMoveObject() const;
	// Confirmed call shapes only (TGObjectManager::SetItem/ResetEventInfo/
	// RemoveItem(bool)/RemoveItem(TVisObjRef const&), Deponia_Linux.asm
	// lines 187267-187628) - picks up/drops whatever object the cursor is
	// currently "holding" (dragging).
	void SetMoveObject(const TVisObjRef &object);
	void ReleaseMoveObject();
	// Confirmed call shapes only (TGameControl::Update, Deponia_Linux.asm
	// lines 469674, 470075) - switches the cursor's appearance depending on
	// whether something detectable is under it.
	void SetInactiveCursor();
	void SetActiveCursor();
	// Confirmed call shape only (TGameControl::Update, Deponia_Linux.asm
	// line 469703) - whether the entry SetCursor()'s family last selected
	// is the one currently playing its "active" image.
	bool IsCursorActive();

	void Draw() override;

	// Confirmed (Deponia_Linux.asm line 623060) - a plain field reset; kept
	// public since it has no confirmed caller in this codebase yet (no
	// CODE XREF turned up for it, only a DATA XREF - presumably called
	// from a TGameControl method not yet reversed).
	void ClearCurrentAnimation();

	// TAnimationOwner
	void AnimationStopped(TGAnimation *animation) override;
	wxString GetOwnerName() const override;
	TId GetOwnerId() const override;

private:
	// Confirmed call shape only (TCursorControl::LoadCursor, Deponia_Linux.
	// asm line 623C78) - see this class's own header comment.
	void GetSCursor(const TVisObjRef &source, SCursor &out);

	// Shared by SetActiveCursor()/SetInactiveCursor()/ReleaseMoveObject(),
	// which each just pick one fixed `useActiveImage` value for whatever
	// SetCursor()'s own family last selected as _activeCursor, rather than
	// a general lookup - invented helper, not a recovered name.
	void setActiveCursorImage(bool useActiveImage);

	std::vector<SCursor *> _cursors;
	std::vector<SCursor *>::iterator _activeCursor = _cursors.end();
	wxPoint _position;
	TGAnimation *_currentAnimation = nullptr;
	TGItem *_heldItem = nullptr;
};

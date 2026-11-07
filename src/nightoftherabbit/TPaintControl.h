// Not yet assert-confirmed to a specific file; stays at the top level of
// src/deponia1 until evidence pins it down (manifest/README.md policy).
//
// Implemented in full (Deponia_Linux.asm lines 143284-749311, all 27
// manifest-listed methods). TPaintControl is a base "drawable surface"
// interface: TMasterControl multiply-inherits it (confirmed via its vtable
// dump - a secondary vtable section whose two slots default to
// TPaintControl::Prepare/TPaintControl::Draw), and TGScene, TCursorControl
// and TLoadingControl all derive from it too (each gets called through the
// same Prepare@slot0/Draw@slot1 pattern). Both defaults are empty.
//
// Real layout (0x44 bytes): vtable (+0x00), an "active" flag (+0x08, default
// TRUE) and a "scrollable" flag (+0x09, default false), the scroll position
// as floats (+0x0C/0x10) and as ints (+0x14/0x18 - the int pair is always
// the truncated float pair), the origin (+0x1C/0x20), the worktop size
// (+0x24/0x28 - the scene's whole drawable extent), the worktop area (+0x2C-
// 0x3B, a wxRect - the part of it the window may scroll over) and the
// visible size (+0x3C/0x40 - the window's size). The scroll position is kept
// so the visible window stays inside the worktop area.
//
// This also owns the engine's "current paint control" (TPictureIO::
// s_pPaintControl): SetCurrent() makes this the one, the destructor clears
// it if it still is.
#pragma once

#include "WxStub.h"

struct FloatPoint {
	float x = 0.0f;
	float y = 0.0f;
};

class TPaintControl {
public:
	TPaintControl() = default;
	virtual ~TPaintControl();
	virtual void Prepare();
	virtual void Draw();

	// Confirmed called (Deponia_Linux.asm line 457443, from
	// TGameControl::UpdateAspectRatio) with the resolved width/height: resets
	// the scroll position and origin and makes the worktop, its area and the
	// visible window all exactly that size.
	void InitControl(int width, int height);

	static TPaintControl *GetCurrent();
	void SetCurrent();
	bool IsActive() const;
	void SetActive(bool active);
	const wxPoint &GetScrollPos() const;
	void SetScrollPos(const wxPoint &pos);
	void SetScrollPos(const wxRealPoint &pos);
	bool IsScrollable() const;
	void SetIsScrollable(bool scrollable);
	int GetWorktopWidth() const;
	int GetWorktopHeight() const;
	// Sets the worktop size and makes its area the whole of it.
	void SetWorktopSize(int width, int height);
	// Sets the worktop size and an area within it (clamped to the worktop:
	// a negative or past-the-end edge snaps to the worktop's own), then
	// re-clamps the scroll position.
	void SetWorktopArea(const wxRect &area, int width, int height);
	wxRect GetWorktopArea() const;
	const FloatPoint &GetFloatScrollPos() const;
	// Despite the name, `position` is the new absolute scroll coordinate
	// (TGScene::InitialiseBackground() passes a saved scroll position), kept
	// inside the worktop area for the current visible size.
	void AdjustWindowHorizontal(float position);
	void AdjustWindowVertical(float position);
	// Confirmed a reference-returning accessor, not a by-value wxSize
	// (TGameControl::CenterScene dereferences the returned address as
	// [ptr]/[ptr+4] rather than reading a register pair, Deponia_Linux.asm
	// lines 460533-460715) - same "logical const, physical mutable
	// accessor" shape as GetScrollPos() above.
	const wxSize &GetVisibleSize() const;
	void SetVisibleSize(int width, int height);
	const wxPoint &GetOrigin() const;
	void SetOrigin(int x, int y);
	// Converts a screen-space position to one relative to this surface
	// (confirmed, asm lines 749311+): scroll position added, origin
	// subtracted.
	wxPoint GetRelativePoint(const wxPoint &pos) const;

private:
	bool _active = true;
	bool _scrollable = false;
	FloatPoint _floatScrollPos;
	wxPoint _scrollPos;
	wxPoint _origin;
	wxSize _worktopSize;
	wxRect _worktopArea;
	wxSize _visibleSize;
};

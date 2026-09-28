// Not yet assert-confirmed to a specific file; stays at the top level of
// src/deponia1 until evidence pins it down (manifest/README.md policy).
//
// TPaintControl is a base "drawable surface" interface: TMasterControl
// multiply-inherits it (confirmed via its vtable dump - a secondary vtable
// section whose two slots default to TPaintControl::Prepare/TPaintControl::
// Draw), and TSceneControl::GetScene(), TCursorControl and TLoadingControl
// all appear to return/derive from it too (each gets called through the
// same Prepare@slot0/Draw@slot1 pattern). Only the methods observed being
// called from TMasterControl are stubbed here.
#pragma once

#include "WxStub.h"

struct FloatPoint {
	float x = 0.0f;
	float y = 0.0f;
};

class TPaintControl {
public:
	virtual ~TPaintControl() = default;
	virtual void Prepare();
	virtual void Draw();

	// Confirmed called (Deponia_Linux.asm line 457443, from
	// TGameControl::UpdateAspectRatio) with the resolved width/height as
	// plain ints - presumably (re)configures the render surface, but its
	// internal behavior wasn't traced further.
	void InitControl(int width, int height);

	void SetCurrent();
	bool IsActive() const;
	void SetActive(bool active);
	const wxPoint &GetScrollPos() const;
	void SetScrollPos(const wxPoint &pos);
	bool IsScrollable() const;
	int GetWorktopWidth() const;
	int GetWorktopHeight() const;
	const FloatPoint &GetFloatScrollPos() const;
	void AdjustWindowHorizontal(float amount);
	void AdjustWindowVertical(float amount);
	// Confirmed a reference-returning accessor, not a by-value wxSize
	// (TGameControl::CenterScene dereferences the returned address as
	// [ptr]/[ptr+4] rather than reading a register pair, Deponia_Linux.asm
	// lines 460533-460715) - same "logical const, physical mutable
	// accessor" shape as GetScrollPos() above.
	const wxSize &GetVisibleSize() const;

private:
	wxPoint _scrollPos{};
	FloatPoint _floatScrollPos{};
	wxSize _visibleSize{};
	bool _active = false;
};

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
	// Confirmed call shape only (TGameControl::MoveScene, Deponia_Linux.asm
	// line 459715) - not reversed beyond that.
	void SetIsScrollable(bool scrollable);
	int GetWorktopWidth() const;
	int GetWorktopHeight() const;
	// Confirmed call shape only (TGameControl::AdjustInterfacesOnScreen,
	// Deponia_Linux.asm lines 465184-465210) - not reversed beyond that.
	void SetWorktopSize(int width, int height);
	// Confirmed call shape only (TGScene::InitialiseBackground(), Deponia_
	// Linux.asm line 54ED5x) - the scene's scrollable area plus its
	// background sprite's own width/height; not reversed beyond that.
	void SetWorktopArea(const wxRect &area, int width, int height);
	const FloatPoint &GetFloatScrollPos() const;
	void AdjustWindowHorizontal(float amount);
	void AdjustWindowVertical(float amount);
	// Confirmed a reference-returning accessor, not a by-value wxSize
	// (TGameControl::CenterScene dereferences the returned address as
	// [ptr]/[ptr+4] rather than reading a register pair, Deponia_Linux.asm
	// lines 460533-460715) - same "logical const, physical mutable
	// accessor" shape as GetScrollPos() above.
	const wxSize &GetVisibleSize() const;
	// Confirmed call shape only (TGameControl::AdjustInterfacesOnScreen,
	// asm line 465210) - not reversed beyond that.
	void SetVisibleSize(int width, int height);
	// Confirmed a reference-returning accessor, same shape as
	// GetScrollPos()/GetVisibleSize() above (TGameControl::
	// AdjustInterfacesOnScreen, asm lines 465220-465223: dereferences the
	// returned address as [ptr]/[ptr+4]).
	const wxPoint &GetOrigin() const;
	// Confirmed call shape only (TGameControl::AdjustInterfacesOnScreen,
	// asm line 465179).
	void SetOrigin(int x, int y);
	// Confirmed call shape only (TGameControl::HandleMouseUp, Deponia_Linux.
	// asm lines 473029-473037, 473083-473091) - converts a screen-space
	// click position to one relative to this surface (presumably subtracting
	// _origin/_scrollPos); not reversed beyond that call shape.
	wxPoint GetRelativePoint(const wxPoint &pos) const;

private:
	wxPoint _scrollPos{};
	FloatPoint _floatScrollPos{};
	wxSize _visibleSize{};
	wxSize _worktopSize{};
	wxPoint _origin{};
	bool _active = false;
	bool _scrollable = false;
};

#include "TPaintControl.h"

#include "graphicslib/picture.h"

TPaintControl::~TPaintControl() {
	if (TPictureIO::s_pPaintControl == this)
		TPictureIO::s_pPaintControl = nullptr;
}

void TPaintControl::Prepare() {
}

void TPaintControl::Draw() {
}

void TPaintControl::InitControl(int width, int height) {
	_scrollPos = wxPoint{0, 0};
	_origin = wxPoint{0, 0};
	_floatScrollPos = FloatPoint();
	_worktopArea = wxRect{0, 0, width, height};
	_worktopSize.width = width;
	_worktopSize.height = height;
	_visibleSize.width = width;
	_visibleSize.height = height;
}

TPaintControl *TPaintControl::GetCurrent() {
	return TPictureIO::s_pPaintControl;
}

void TPaintControl::SetCurrent() {
	TPictureIO::s_pPaintControl = this;
}

bool TPaintControl::IsActive() const {
	return _active;
}

void TPaintControl::SetActive(bool active) {
	_active = active;
}

const wxPoint &TPaintControl::GetScrollPos() const {
	return _scrollPos;
}

void TPaintControl::SetScrollPos(const wxPoint &pos) {
	_scrollPos = pos;
	_floatScrollPos.x = static_cast<float>(pos.x);
	_floatScrollPos.y = static_cast<float>(pos.y);
}

void TPaintControl::SetScrollPos(const wxRealPoint &pos) {
	_scrollPos.x = static_cast<int>(pos.x);
	_scrollPos.y = static_cast<int>(pos.y);
	_floatScrollPos.x = pos.x;
	_floatScrollPos.y = pos.y;
}

bool TPaintControl::IsScrollable() const {
	return _scrollable;
}

void TPaintControl::SetIsScrollable(bool scrollable) {
	_scrollable = scrollable;
}

int TPaintControl::GetWorktopWidth() const {
	return _worktopSize.width;
}

int TPaintControl::GetWorktopHeight() const {
	return _worktopSize.height;
}

void TPaintControl::SetWorktopSize(int width, int height) {
	_worktopArea = wxRect{0, 0, width, height};
	_worktopSize.width = width;
	_worktopSize.height = height;
}

// Keeps a scroll coordinate `position` (the window spans `visible` from it)
// inside [low, high] - the area's own edges - returning the clamped float.
static float clampWindow(float position, int visible, int low, int high) {
	if (position + static_cast<float>(visible) > static_cast<float>(high))
		position = static_cast<float>(high + 1 - visible);
	if (static_cast<float>(low) > position)
		position = static_cast<float>(low);
	return position;
}

void TPaintControl::SetWorktopArea(const wxRect &area, int width, int height) {
	_worktopSize.width = width;
	_worktopSize.height = height;

	// Every edge test reads the caller's rect; the corrections go to a copy.
	wxRect clamped = area;
	if (area.GetLeft() < 0 || area.GetLeft() > _worktopSize.width)
		clamped.x = 0;
	if (area.GetRight() < 0 || area.GetRight() > _worktopSize.width)
		clamped.width = width;
	if (area.GetTop() < 0 || area.GetTop() > _worktopSize.height)
		clamped.y = 0;
	if (area.GetBottom() < 0 || area.GetBottom() > _worktopSize.height)
		clamped.height = height;
	_worktopArea = clamped;

	_floatScrollPos.x = clampWindow(_floatScrollPos.x, _visibleSize.width, _worktopArea.GetLeft(),
	                                _worktopArea.GetRight());
	_scrollPos.x = static_cast<int>(_floatScrollPos.x);
	_floatScrollPos.y = clampWindow(_floatScrollPos.y, _visibleSize.height, _worktopArea.GetTop(),
	                                _worktopArea.GetBottom());
	_scrollPos.y = static_cast<int>(_floatScrollPos.y);
}

wxRect TPaintControl::GetWorktopArea() const {
	return _worktopArea;
}

const FloatPoint &TPaintControl::GetFloatScrollPos() const {
	return _floatScrollPos;
}

void TPaintControl::AdjustWindowHorizontal(float position) {
	_floatScrollPos.x = clampWindow(position, _visibleSize.width, _worktopArea.GetLeft(), _worktopArea.GetRight());
	_scrollPos.x = static_cast<int>(_floatScrollPos.x);
}

void TPaintControl::AdjustWindowVertical(float position) {
	_floatScrollPos.y = clampWindow(position, _visibleSize.height, _worktopArea.GetTop(), _worktopArea.GetBottom());
	_scrollPos.y = static_cast<int>(_floatScrollPos.y);
}

const wxSize &TPaintControl::GetVisibleSize() const {
	return _visibleSize;
}

void TPaintControl::SetVisibleSize(int width, int height) {
	_visibleSize.width = width;
	_visibleSize.height = height;
}

const wxPoint &TPaintControl::GetOrigin() const {
	return _origin;
}

void TPaintControl::SetOrigin(int x, int y) {
	_origin.x = x;
	_origin.y = y;
}

wxPoint TPaintControl::GetRelativePoint(const wxPoint &pos) const {
	return (pos + _scrollPos) - _origin;
}

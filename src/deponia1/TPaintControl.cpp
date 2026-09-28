#include "TPaintControl.h"

void TPaintControl::Prepare() {
}

void TPaintControl::Draw() {
}

void TPaintControl::InitControl(int /*width*/, int /*height*/) {
}

void TPaintControl::SetCurrent() {
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
}

bool TPaintControl::IsScrollable() const {
	return false;
}

int TPaintControl::GetWorktopWidth() const {
	return 0;
}

int TPaintControl::GetWorktopHeight() const {
	return 0;
}

const FloatPoint &TPaintControl::GetFloatScrollPos() const {
	return _floatScrollPos;
}

const wxSize &TPaintControl::GetVisibleSize() const {
	return _visibleSize;
}

void TPaintControl::AdjustWindowHorizontal(float /*amount*/) {
}

void TPaintControl::AdjustWindowVertical(float /*amount*/) {
}

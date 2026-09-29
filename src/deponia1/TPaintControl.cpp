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
	return _worktopSize.width;
}

int TPaintControl::GetWorktopHeight() const {
	return _worktopSize.height;
}

void TPaintControl::SetWorktopSize(int width, int height) {
	_worktopSize.width = width;
	_worktopSize.height = height;
}

const FloatPoint &TPaintControl::GetFloatScrollPos() const {
	return _floatScrollPos;
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

void TPaintControl::AdjustWindowHorizontal(float /*amount*/) {
}

void TPaintControl::AdjustWindowVertical(float /*amount*/) {
}

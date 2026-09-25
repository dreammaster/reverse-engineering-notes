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
    return m_active;
}

void TPaintControl::SetActive(bool active) {
    m_active = active;
}

const wxPoint& TPaintControl::GetScrollPos() const {
    return m_scrollPos;
}

void TPaintControl::SetScrollPos(const wxPoint& pos) {
    m_scrollPos = pos;
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

const FloatPoint& TPaintControl::GetFloatScrollPos() const {
    return m_floatScrollPos;
}

void TPaintControl::AdjustWindowHorizontal(float /*amount*/) {
}

void TPaintControl::AdjustWindowVertical(float /*amount*/) {
}

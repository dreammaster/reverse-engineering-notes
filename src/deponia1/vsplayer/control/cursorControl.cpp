#include "vsplayer/control/cursorControl.h"

wxPoint TCursorControl::GetPositionNextToCursor() const {
	return wxPoint();
}

void TCursorControl::SetCursor(int /*cursorId*/, bool /*flag*/) {
}

void TCursorControl::SetCursor(bool /*flag1*/, int /*cursorId*/, bool /*flag2*/) {
}

void TCursorControl::Clear() {
}

void TCursorControl::LoadCursor(const TVisObjRef &/*cursor*/) {
}

void TCursorControl::LinkButtonCursor(int /*linkedId*/, int /*objectId*/) {
}

void TCursorControl::SetCursorPosition(int /*x*/, int /*y*/) {
}

bool TCursorControl::IsActiveMoveObject() const {
	return false;
}

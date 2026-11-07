#include "TTimer.h"

#include "WxStub.h"

TTimer::TTimer() {
	_setAt = wxGetLocalTimeMillis();
}

long TTimer::GetTime() const {
	return static_cast<long>(wxGetLocalTimeMillis() - _setAt);
}

void TTimer::SetTime() {
	_setAt = wxGetLocalTimeMillis();
}

void TTimer::AdjustTimer(long delta) {
	_setAt += delta;
}

void TTimer::WaitUntil(long ms) {
	wxMilliSleep(ms);
}

// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed in full (Deponia_Linux.asm lines 559372-559478, all 6
// manifest-listed methods): a millisecond stopwatch backed by
// wxGetLocalTimeMillis(). The constructor writes a confirmed second qword
// (immediately after the timestamp) to 0, but no other method here ever
// reads or writes it - not modeled, since nothing depends on it.
#pragma once

class TTimer {
public:
	TTimer();
	~TTimer() = default;

	// Confirmed (asm lines 559407-559427): elapsed milliseconds since the
	// last SetTime() (or construction), via wxLongLong::ToLong() - hence
	// `long`, not a wider integer type.
	long GetTime() const;
	void SetTime();
	// Confirmed (asm line 559460): shifts the reference point by `delta`
	// milliseconds - a positive delta makes GetTime() report LESS elapsed
	// time (as if SetTime() had been called later than it really was).
	void AdjustTimer(long delta);
	// Confirmed (asm lines 559471-559478): ignores `this` entirely - just a
	// tail call to wxMilliSleep(ms), despite the "Until" name suggesting it
	// might reference the timer's own state.
	void WaitUntil(long ms);

private:
	long long _setAt = 0;
};

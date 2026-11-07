// Confirmed (Deponia_Linux.asm lines 1628599-1628883 and 430170-430273, 456047-456157): the
// tweens (the changes of a number over a time) of the scripts and of the action parts that move
// objects. A Tween goes from `begin` to `end` in `duration` milliseconds, by an easing function
// (Easing.h: a function from the part of the time that is gone, 0 to 1, to the part of the way
// that is gone); it can start again when it is over (`loop`) and then go back (`pingPong`).
// A TVisObjTween changes two numbers - the x and the y of a point - of a field of a data object,
// every frame, until both tweens are over; a field of another type (a number, a float) uses
// only the x tween.
//
// Original layout of a Tween (0x50 bytes): +0x00 the time that is gone, +0x08 the duration (1 if it
// was not above 0), +0x10 begin, +0x18 end, +0x20 the value now, +0x28 loop, +0x29 ping-pong, +0x30 the
// easing (a std::function). A TVisObjTween (0xB0 bytes): +0x00 the tween of x, +0x50 the tween of y,
// +0xA0 the object, +0xA8 the field.
#pragma once

#include <functional>
#include <string>

#include "datastruct/visobjref.h"

class Tween {
public:
	Tween() : Tween(0.0, 0.0, 0.0, nullptr, false, false) {
	}
	/** `duration`: milliseconds (a duration that is not above 0 is taken as 1). */
	Tween(double begin, double end, double duration, std::function<double(double)> easing, bool loop, bool pingPong);

	/** The tween goes on by `deltaMs` milliseconds. */
	void Update(float deltaMs);
	bool IsFinished() const;
	/** The value now (begin + (end - begin) * easing(the part of the time that is gone)). */
	double GetValue() const {
		return _value;
	}

	// The name a script gave a tween (TGameControl::StartTween(const Tween &, const std::string &)).
	std::string name;

private:
	double _elapsed;   // +0x00
	double _duration;  // +0x08
	double _begin;     // +0x10
	double _end;       // +0x18
	double _value;     // +0x20
	bool _loop;        // +0x28
	bool _pingPong;    // +0x29
	std::function<double(double)> _easing;  // +0x30
};

struct TVisObjTween {
	TVisObjTween() : field(0) {
	}
	/** `x` changes the number (or the x of a point) of `field` of `target`, `y` the y of a point. */
	TVisObjTween(const Tween &x, const TVisObjRef &target, int field, const Tween &y)
		: x(x), y(y), target(target), field(field) {
	}

	Tween x;
	Tween y;
	TVisObjRef target;
	int field;

	/** Goes on by `deltaMs` milliseconds and sets the field; whether it goes on (false: both
	 *  tweens are over, or the field is of a type that cannot be tweened). */
	bool update(double deltaMs);
};

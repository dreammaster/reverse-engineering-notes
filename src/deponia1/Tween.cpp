#include "Tween.h"

#include <algorithm>

#include "datastruct/type.h"
#include "datastruct/visionaireobject.h"

// Confirmed (asm lines 1628599-1628795)
Tween::Tween(double begin, double end, double duration, std::function<double(double)> easing, bool loop, bool pingPong)
	: _elapsed(0.0), _duration(duration > 0.0 ? duration : 1.0), _begin(begin), _end(end), _value(0.0), _loop(loop),
	  _pingPong(pingPong), _easing(easing) {
}

// Confirmed (asm lines 1628798-1628846): the time goes on (not past the duration); when the
// tween has reached its end and loops, it starts again (turned round, when it pings and pongs).
void Tween::Update(float deltaMs) {
	_elapsed += std::min((double)deltaMs, _duration - _elapsed);

	// (an empty easing is a bad cast, as std::function throws it)
	double progress = _easing(_elapsed / _duration);

	_value = _begin + (_end - _begin) * progress;

	if (_elapsed >= _duration && _loop) {
		if (_pingPong)
			std::swap(_begin, _end);

		_elapsed = 0.0;
	}
}

// Confirmed (asm lines 1628859-1628864)
bool Tween::IsFinished() const {
	return _elapsed >= _duration;
}

// Confirmed (asm lines 456047-456157): the x tween makes the number (the field's type 1) or the
// float (4) or the x of the point (13); the y of the point is the other tween's.
bool TVisObjTween::update(double deltaMs) {
	x.Update((float)deltaMs);
	y.Update((float)deltaMs);

	switch ((eTypeData)target.GetObjectPointer()->GetTypeField(field)) {
	case eTypeData::kInt:
		target.SetValue(field, (int)x.GetValue(), TSendEventEnum::kForce);
		return !x.IsFinished() || !y.IsFinished();
	case eTypeData::kPoint: {
		wxPoint point;

		point.x = (int)x.GetValue();
		point.y = (int)y.GetValue();
		target.SetValue(field, point, TSendEventEnum::kForce);
		return !x.IsFinished() || !y.IsFinished();
	}
	case eTypeData::kFloat:
		target.SetValue(field, (float)x.GetValue(), TSendEventEnum::kForce);
		return !x.IsFinished() || !y.IsFinished();
	default:
		return false;
	}
}

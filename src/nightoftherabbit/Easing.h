// Confirmed (Deponia_Linux.asm lines 1628883-1630440, `Easing::*(double)`): the easing functions
// of the tweens (see Tween.h): each takes the part of the time that is gone (0 to 1) and gives the
// part of the way that is gone. They are Robert Penner's equations: In starts slowly, Out ends
// slowly, InOut does both; the Back ones overshoot (by 1.70158), the Bounce ones bounce, the
// Elastic ones swing like a spring. The `easing_*_func` ones are the In version of each kind
// (what a script that names only the kind gets).
#pragma once

struct Easing {
	static double BackIn(double t);
	static double BackOut(double t);
	static double BackInOut(double t);
	static double BounceIn(double t);
	static double BounceOut(double t);
	static double BounceInOut(double t);
	static double CircIn(double t);
	static double CircOut(double t);
	static double CircInOut(double t);
	static double CubicIn(double t);
	static double CubicOut(double t);
	static double CubicInOut(double t);
	static double ElasticIn(double t);
	static double ElasticOut(double t);
	static double ElasticInOut(double t);
	/** Confirmed (asm lines 1629616-1629619): the way goes on as the time does. */
	static double LinearIn(double t) {
		return t;
	}
	static double LinearOut(double t);
	static double LinearInOut(double t);
	/** Nothing happens until the time is over (the way is 0, and 1 at the end). */
	static double NoneIn(double t);
	static double NoneOut(double t);
	static double NoneInOut(double t);
	static double QuadIn(double t);
	static double QuadOut(double t);
	static double QuadInOut(double t);
	static double QuartIn(double t);
	static double QuartOut(double t);
	static double QuartInOut(double t);
	static double QuintIn(double t);
	static double QuintOut(double t);
	static double QuintInOut(double t);
	static double SineIn(double t);
	static double SineOut(double t);
	static double SineInOut(double t);

	static double easing_back_func(double t);
	static double easing_bounce_func(double t);
	static double easing_circ_func(double t);
	static double easing_cubic_func(double t);
	static double easing_elastic_func(double t);
	static double easing_expo_func(double t);
	static double easing_linear_func(double t);
	static double easing_quad_func(double t);
	static double easing_quart_func(double t);
	static double easing_quint_func(double t);
	static double easing_sine_func(double t);
};

#include "Easing.h"

#include <cmath>

// The constants of the equations (Deponia_Linux.asm: qword_E40078 ... qword_E400D0).
static const double kPi = 3.141592653589793;
static const double kBackOvershoot = 1.70158;
static const double kBounceScale = 7.5625;
static const double kElasticPeriod = 0.3;
static const double kElasticShift = 0.075;

// The bounce of the Out equation: the parabolas of the four bounces.
static double bounce(double x) {
	double offset;

	if (x < 0.36363636363636365) {
		offset = 0.0;
	} else if (x < 0.7272727272727273) {
		x -= 0.5454545454545454;
		offset = 0.75;
	} else if (x < 0.9090909090909091) {
		x -= 0.8181818181818182;
		offset = 0.9375;
	} else {
		x -= 0.9545454545454546;
		offset = 0.984375;
	}

	return kBounceScale * x * x + offset;
}

// The swing of the Elastic equations at `p` (the time minus 1, or its mirror).
static double elasticSwing(double p) {
	return std::pow(2.0, 10.0 * p) * std::sin((p - kElasticShift) * 2.0 * kPi / kElasticPeriod);
}

// Confirmed (asm lines 1628883-1628958)
double Easing::BackIn(double t) {
	return t * t * ((kBackOvershoot + 1.0) * t - kBackOvershoot);
}

double Easing::BackOut(double t) {
	double u = 1.0 - t;

	return 1.0 - u * u * ((kBackOvershoot + 1.0) * u - kBackOvershoot);
}

double Easing::BackInOut(double t) {
	if (t < 0.5) {
		double w = t + t;

		return w * w * ((kBackOvershoot + 1.0) * w - kBackOvershoot) * 0.5;
	}

	double v = 1.0 - (t + t - 1.0);

	return (1.0 - v * v * ((kBackOvershoot + 1.0) * v - kBackOvershoot)) * 0.5 + 0.5;
}

// Confirmed (asm lines 1628974-1629200)
double Easing::BounceIn(double t) {
	return 1.0 - bounce(1.0 - t);
}

double Easing::BounceOut(double t) {
	return bounce(t);
}

double Easing::BounceInOut(double t) {
	if (t < 0.5)
		return (1.0 - bounce(1.0 - (t + t))) * 0.5;

	return bounce(t + t - 1.0) * 0.5 + 0.5;
}

// Confirmed (asm lines 1629204-1629360)
double Easing::CircIn(double t) {
	return 1.0 - std::sqrt(1.0 - t * t);
}

double Easing::CircOut(double t) {
	double u = 1.0 - t;

	return std::sqrt(1.0 - u * u);
}

double Easing::CircInOut(double t) {
	if (t < 0.5) {
		double w = t + t;

		return (1.0 - std::sqrt(1.0 - w * w)) * 0.5;
	}

	double v = 1.0 - (t + t - 1.0);

	return std::sqrt(1.0 - v * v) * 0.5 + 0.5;
}

// Confirmed (asm lines 1629370-1629440)
double Easing::CubicIn(double t) {
	return t * t * t;
}

double Easing::CubicOut(double t) {
	double u = 1.0 - t;

	return 1.0 - u * u * u;
}

double Easing::CubicInOut(double t) {
	if (t < 0.5) {
		double w = t + t;

		return w * w * w * 0.5;
	}

	double v = 1.0 - (t + t - 1.0);

	return (1.0 - v * v * v) * 0.5 + 0.5;
}

// Confirmed (asm lines 1629453-1629605)
double Easing::ElasticIn(double t) {
	return -elasticSwing(t - 1.0);
}

double Easing::ElasticOut(double t) {
	return elasticSwing((1.0 - t) - 1.0) + 1.0;
}

double Easing::ElasticInOut(double t) {
	if (t < 0.5)
		return -elasticSwing(t + t - 1.0) * 0.5;

	return (elasticSwing(1.0 - (t + t - 1.0) - 1.0) + 1.0) * 0.5 + 0.5;
}

// Confirmed (asm lines 1629631-1629672)
double Easing::LinearOut(double t) {
	return 1.0 - (1.0 - t);
}

double Easing::LinearInOut(double t) {
	if (t < 0.5)
		return (t + t) * 0.5;

	return (1.0 - (1.0 - (t + t - 1.0))) * 0.5 + 0.5;
}

// Confirmed (asm lines 1629684-1629760): nothing until the end.
double Easing::NoneIn(double t) {
	return (t == 1.0) ? 1.0 : 0.0;
}

double Easing::NoneOut(double t) {
	return (t == 1.0) ? 1.0 : 0.0;
}

double Easing::NoneInOut(double t) {
	return (t == 1.0) ? 1.0 : 0.0;
}

// Confirmed (asm lines 1629771-1630015)
double Easing::QuadIn(double t) {
	return t * t;
}

double Easing::QuadOut(double t) {
	double u = 1.0 - t;

	return 1.0 - u * u;
}

double Easing::QuadInOut(double t) {
	if (t < 0.5) {
		double w = t + t;

		return w * w * 0.5;
	}

	double v = 1.0 - (t + t - 1.0);

	return (1.0 - v * v) * 0.5 + 0.5;
}

double Easing::QuartIn(double t) {
	return t * t * t * t;
}

double Easing::QuartOut(double t) {
	double u = 1.0 - t;

	return 1.0 - u * u * u * u;
}

double Easing::QuartInOut(double t) {
	if (t < 0.5) {
		double w = t + t;

		return w * w * w * w * 0.5;
	}

	double v = 1.0 - (t + t - 1.0);

	return (1.0 - v * v * v * v) * 0.5 + 0.5;
}

double Easing::QuintIn(double t) {
	return t * t * t * t * t;
}

double Easing::QuintOut(double t) {
	double u = 1.0 - t;

	return 1.0 - u * u * u * u * u;
}

double Easing::QuintInOut(double t) {
	if (t < 0.5) {
		double w = t + t;

		return w * w * w * w * w * 0.5;
	}

	double v = 1.0 - (t + t - 1.0);

	return (1.0 - v * v * v * v * v) * 0.5 + 0.5;
}

// Confirmed (asm lines 1630021-1630130)
double Easing::SineIn(double t) {
	return 1.0 - std::cos(t * kPi * 0.5);
}

double Easing::SineOut(double t) {
	double u = 1.0 - t;

	return 1.0 - (1.0 - std::cos(kPi * u * 0.5));
}

double Easing::SineInOut(double t) {
	if (t < 0.5)
		return (1.0 - std::cos((t + t) * kPi * 0.5)) * 0.5;

	double v = 1.0 - (t + t - 1.0);

	return (1.0 - (1.0 - std::cos(kPi * v * 0.5))) * 0.5 + 0.5;
}

// Confirmed (asm lines 1630135-1630440): the In version of each kind.
double Easing::easing_back_func(double t) {
	return BackIn(t);
}

double Easing::easing_bounce_func(double t) {
	return BounceIn(t);
}

double Easing::easing_circ_func(double t) {
	return CircIn(t);
}

double Easing::easing_cubic_func(double t) {
	return CubicIn(t);
}

double Easing::easing_elastic_func(double t) {
	return ElasticIn(t);
}

// (the original leaves 0 as it is: the result then is 2^-10, not 0)
double Easing::easing_expo_func(double t) {
	return std::pow(2.0, (t - 1.0) * 10.0);
}

double Easing::easing_linear_func(double t) {
	return t;
}

double Easing::easing_quad_func(double t) {
	return QuadIn(t);
}

double Easing::easing_quart_func(double t) {
	return QuartIn(t);
}

double Easing::easing_quint_func(double t) {
	return QuintIn(t);
}

double Easing::easing_sine_func(double t) {
	return SineIn(t);
}

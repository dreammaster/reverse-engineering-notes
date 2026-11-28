#include <cmath>
#include <cstdio>
#include <vector>

#include "Easing.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

static const double kPi = 3.141592653589793;

typedef double (*EaseFunction)(double);

struct Kind {
	const char *name;
	EaseFunction in, out, inOut;
	bool monotonic;
};

static bool near(double a, double b, double epsilon = 1e-9) {
	return std::fabs(a - b) <= epsilon;
}

// The easing equations of the tweens: each kind goes from 0 to 1, the Out one is the In one mirrored, the InOut one is the two
// halves put together, and the values of some are known.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);

	const Kind kinds[] = {
		{"Back", Easing::BackIn, Easing::BackOut, Easing::BackInOut, false},
		{"Bounce", Easing::BounceIn, Easing::BounceOut, Easing::BounceInOut, false},
		{"Circ", Easing::CircIn, Easing::CircOut, Easing::CircInOut, true},
		{"Cubic", Easing::CubicIn, Easing::CubicOut, Easing::CubicInOut, true},
		{"Elastic", Easing::ElasticIn, Easing::ElasticOut, Easing::ElasticInOut, false},
		{"Linear", Easing::LinearIn, Easing::LinearOut, Easing::LinearInOut, true},
		{"Quad", Easing::QuadIn, Easing::QuadOut, Easing::QuadInOut, true},
		{"Quart", Easing::QuartIn, Easing::QuartOut, Easing::QuartInOut, true},
		{"Quint", Easing::QuintIn, Easing::QuintOut, Easing::QuintInOut, true},
		{"Sine", Easing::SineIn, Easing::SineOut, Easing::SineInOut, true},
	};

	for (const Kind &kind : kinds) {
		// (the elastic ones swing a little around 0 at the start: 2^-10 of the swing)
		double slack = 2e-3;

		CHECK(near(kind.in(0.0), 0.0, slack));
		CHECK(near(kind.in(1.0), 1.0, slack));
		CHECK(near(kind.out(0.0), 0.0, slack));
		CHECK(near(kind.out(1.0), 1.0, slack));
		CHECK(near(kind.inOut(0.0), 0.0, slack));
		CHECK(near(kind.inOut(1.0), 1.0, slack));
		CHECK(near(kind.inOut(0.5), 0.5, 1e-9));

		bool mirrored = true;
		bool continuous = true;
		bool rises = true;
		double previousIn = kind.in(0.0);
		double previousOut = kind.out(0.0);
		double previousInOut = kind.inOut(0.0);

		for (int i = 1; i <= 100; i++) {
			double t = i / 100.0;

			// the Out equation is the In one the other way round
			if (!near(kind.out(t), 1.0 - kind.in(1.0 - t), 1e-9))
				mirrored = false;

			// the halves of the InOut one are the In and the Out ones, in half the size
			double half = (t < 0.5) ? kind.in(t * 2.0) * 0.5 : 1.0 - kind.in(2.0 - t * 2.0) * 0.5;

			if (!near(kind.inOut(t), half, 1e-9))
				continuous = false;

			if (kind.monotonic) {
				if (kind.in(t) < previousIn - 1e-12 || kind.out(t) < previousOut - 1e-12 || kind.inOut(t) < previousInOut - 1e-12)
					rises = false;
				previousIn = kind.in(t);
				previousOut = kind.out(t);
				previousInOut = kind.inOut(t);
			}
		}

		if (!mirrored || !continuous || !rises)
			printf("%s: mirrored %d, halves %d, rises %d\n", kind.name, (int)mirrored, (int)continuous, (int)rises);
		CHECK(mirrored);
		CHECK(continuous);
		CHECK(rises);
	}

	// values that are known
	CHECK(near(Easing::QuadIn(0.5), 0.25));
	CHECK(near(Easing::CubicIn(0.5), 0.125));
	CHECK(near(Easing::QuartIn(0.5), 0.0625));
	CHECK(near(Easing::QuintIn(0.5), 0.03125));
	CHECK(near(Easing::QuadOut(0.5), 0.75));
	CHECK(near(Easing::CubicOut(0.5), 0.875));
	CHECK(near(Easing::SineIn(0.5), 1.0 - std::cos(kPi / 4)));
	CHECK(near(Easing::SineOut(0.5), std::cos(kPi / 4)));
	CHECK(near(Easing::CircIn(0.5), 1.0 - std::sqrt(0.75)));
	CHECK(near(Easing::LinearIn(0.3), 0.3));
	CHECK(near(Easing::LinearOut(0.3), 0.3));
	CHECK(near(Easing::LinearInOut(0.3), 0.3));
	CHECK(near(Easing::LinearInOut(0.8), 0.8));

	// back goes under 0 before it starts, and over 1 before it ends
	CHECK(near(Easing::BackIn(0.5), 0.25 * (2.70158 * 0.5 - 1.70158)));
	CHECK(Easing::BackIn(0.3) < 0.0);
	CHECK(Easing::BackOut(0.7) > 1.0);
	CHECK(Easing::BackInOut(0.2) < 0.0);
	CHECK(Easing::BackInOut(0.8) > 1.0);

	// bounce: the parabolas
	CHECK(near(Easing::BounceOut(0.2), 7.5625 * 0.04));
	CHECK(near(Easing::BounceOut(0.5), 7.5625 * (0.5 - 0.5454545454545454) * (0.5 - 0.5454545454545454) + 0.75));
	CHECK(near(Easing::BounceOut(1.0), 1.0, 1e-6));
	CHECK(Easing::BounceOut(0.9) < 1.0);
	CHECK(Easing::BounceOut(0.55) < Easing::BounceOut(0.45));

	// elastic swings around the end
	CHECK(Easing::ElasticOut(0.3) > 1.0 || Easing::ElasticOut(0.4) > 1.0 || Easing::ElasticOut(0.2) > 1.0);
	CHECK(Easing::ElasticIn(0.8) < 0.0 || Easing::ElasticIn(0.7) < 0.0 || Easing::ElasticIn(0.9) < 0.0);

	// none: nothing until the end
	CHECK(Easing::NoneIn(0.0) == 0.0);
	CHECK(Easing::NoneIn(0.999) == 0.0);
	CHECK(Easing::NoneIn(1.0) == 1.0);
	CHECK(Easing::NoneOut(0.5) == 0.0);
	CHECK(Easing::NoneOut(1.0) == 1.0);
	CHECK(Easing::NoneInOut(0.5) == 0.0);
	CHECK(Easing::NoneInOut(1.0) == 1.0);

	// the functions the tweens are made of by name are the In ones (and the exponential)
	for (int i = 0; i <= 10; i++) {
		double t = i / 10.0;

		CHECK(Easing::easing_back_func(t) == Easing::BackIn(t));
		CHECK(Easing::easing_bounce_func(t) == Easing::BounceIn(t));
		CHECK(Easing::easing_circ_func(t) == Easing::CircIn(t));
		CHECK(Easing::easing_cubic_func(t) == Easing::CubicIn(t));
		CHECK(Easing::easing_elastic_func(t) == Easing::ElasticIn(t));
		CHECK(Easing::easing_linear_func(t) == t);
		CHECK(Easing::easing_quad_func(t) == Easing::QuadIn(t));
		CHECK(Easing::easing_quart_func(t) == Easing::QuartIn(t));
		CHECK(Easing::easing_quint_func(t) == Easing::QuintIn(t));
		CHECK(Easing::easing_sine_func(t) == Easing::SineIn(t));
	}
	CHECK(near(Easing::easing_expo_func(1.0), 1.0));
	CHECK(near(Easing::easing_expo_func(0.0), 1.0 / 1024.0));
	CHECK(near(Easing::easing_expo_func(0.9), 0.5));

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

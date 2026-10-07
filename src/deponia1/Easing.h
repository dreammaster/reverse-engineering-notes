// Confirmed (Deponia_Linux.asm lines 1628883-1630440, `Easing::*(double)`): the easing functions
// of the tweens (see Tween.h): each takes the part of the time that is gone (0 to 1) and gives the
// part of the way that is gone. Only the linear one is reconstructed so far; the others (Back,
// Bounce, Circ, Cubic, Elastic, LinearOut/InOut, None, Quad, Quart, Quint, Sine, each In/Out/InOut)
// are in TODO.md, they are what the scripts' tweens choose from by name.
#pragma once

struct Easing {
	/** Confirmed (asm lines 1629616-1629619): the way goes on as the time does. */
	static double LinearIn(double t) {
		return t;
	}
};

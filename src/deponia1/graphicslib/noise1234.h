// Noise1234 and SimplexNoise1234 (asm 434064-437359 for the noise() functions that the scripts use): Stefan Gustavson's public
// domain noise library, a vendor library. The Perlin noise of 1 to 4 numbers and the simplex noise of 1 and 2 numbers, both from
// about -1 to 1 and 0 at whole numbers. The functions are written from the published algorithm; what is the binary's is the
// table of permutations (Ken Perlin's, twice; the same in both classes) and the constants: 0.188, 0.507, 0.936 and 0.87 for
// the four sizes of Perlin noise, 0.395 and 45.23 and the skew factors 0.366025403 and 0.211324865 for the simplex noise. The
// periodic pnoise() functions are not used by the player and not reconstructed.
#pragma once

class Noise1234 {
public:
	static float noise(float x);
	static float noise(float x, float y);
	static float noise(float x, float y, float z);
	static float noise(float x, float y, float z, float w);
};

class SimplexNoise1234 {
public:
	static float noise(float x);
	static float noise(float x, float y);
};

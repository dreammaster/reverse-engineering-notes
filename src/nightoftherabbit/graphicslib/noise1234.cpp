#include "graphicslib/noise1234.h"

// Confirmed (the data of Noise1234::perm and SimplexNoise1234::perm): Ken Perlin's permutation, 256 numbers, twice.
static const unsigned char perm[512] = {
	151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225,
	140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148,
	247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32,
	57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175,
	74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122,
	60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54,
	65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169,
	200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64,
	52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212,
	207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213,
	119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
	129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104,
	218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,
	81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157,
	184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93,
	222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180,
	151, 160, 137, 91, 90, 15, 131, 13, 201, 95, 96, 53, 194, 233, 7, 225,
	140, 36, 103, 30, 69, 142, 8, 99, 37, 240, 21, 10, 23, 190, 6, 148,
	247, 120, 234, 75, 0, 26, 197, 62, 94, 252, 219, 203, 117, 35, 11, 32,
	57, 177, 33, 88, 237, 149, 56, 87, 174, 20, 125, 136, 171, 168, 68, 175,
	74, 165, 71, 134, 139, 48, 27, 166, 77, 146, 158, 231, 83, 111, 229, 122,
	60, 211, 133, 230, 220, 105, 92, 41, 55, 46, 245, 40, 244, 102, 143, 54,
	65, 25, 63, 161, 1, 216, 80, 73, 209, 76, 132, 187, 208, 89, 18, 169,
	200, 196, 135, 130, 116, 188, 159, 86, 164, 100, 109, 198, 173, 186, 3, 64,
	52, 217, 226, 250, 124, 123, 5, 202, 38, 147, 118, 126, 255, 82, 85, 212,
	207, 206, 59, 227, 47, 16, 58, 17, 182, 189, 28, 42, 223, 183, 170, 213,
	119, 248, 152, 2, 44, 154, 163, 70, 221, 153, 101, 155, 167, 43, 172, 9,
	129, 22, 39, 253, 19, 98, 108, 110, 79, 113, 224, 232, 178, 185, 112, 104,
	218, 246, 97, 228, 251, 34, 242, 193, 238, 210, 144, 12, 191, 179, 162, 241,
	81, 51, 145, 235, 249, 14, 239, 107, 49, 192, 214, 31, 181, 199, 106, 157,
	184, 84, 204, 176, 115, 121, 50, 45, 127, 4, 150, 254, 138, 236, 205, 93,
	222, 114, 67, 29, 24, 72, 243, 141, 128, 195, 78, 66, 215, 61, 156, 180
};

/** The integer part of x, but towards minus infinity only for the numbers below 1 (0 itself gives -1, as the library has it). */
static int fastFloor(float x) {
	return (x > 0.0f) ? static_cast<int>(x) : static_cast<int>(x) - 1;
}

static float fade(float t) {
	return t * t * t * (t * (t * 6.0f - 15.0f) + 10.0f);
}

static float lerp(float t, float a, float b) {
	return a + t * (b - a);
}

// Confirmed (asm lines 434500-434680 for the four of Noise1234; 434064-434130 for the two of SimplexNoise1234)
static float grad(int hash, float x) {
	int h = hash & 15;
	float gradient = static_cast<float>(1.0 + (h & 7));

	if (h & 8)
		gradient = -gradient;

	return gradient * x;
}

static float grad(int hash, float x, float y) {
	int h = hash & 7;
	float u = (h < 4) ? x : y;
	float v = (h < 4) ? y : x;
	double first = (h & 1) ? -static_cast<double>(u) : static_cast<double>(u);
	double second = (h & 2) ? static_cast<double>(v) * -2.0 : static_cast<double>(v) + static_cast<double>(v);

	return static_cast<float>(first + second);
}

static float grad(int hash, float x, float y, float z) {
	int h = hash & 15;
	float u = (h < 8) ? x : y;
	float v = (h < 4) ? y : ((h == 12 || h == 14) ? x : z);

	return ((h & 1) ? -u : u) + ((h & 2) ? -v : v);
}

static float grad(int hash, float x, float y, float z, float t) {
	int h = hash & 31;
	float u = (h < 24) ? x : y;
	float v = (h < 16) ? y : z;
	float w = (h < 8) ? z : t;

	return ((h & 1) ? -u : u) + ((h & 2) ? -v : v) + ((h & 4) ? -w : w);
}

// Confirmed (asm lines 434680-434756)
float Noise1234::noise(float x) {
	int ix0 = fastFloor(x);
	float fx0 = x - static_cast<float>(ix0);
	float fx1 = fx0 - 1.0f;
	int ix1 = (ix0 + 1) & 0xFF;

	ix0 = ix0 & 0xFF;

	float s = fade(fx0);
	float n0 = grad(perm[ix0], fx0);
	float n1 = grad(perm[ix1], fx1);

	return 0.188f * lerp(s, n0, n1);
}

// Confirmed (asm lines 434839-435074)
float Noise1234::noise(float x, float y) {
	int ix0 = fastFloor(x);
	int iy0 = fastFloor(y);
	float fx0 = x - static_cast<float>(ix0);
	float fy0 = y - static_cast<float>(iy0);
	float fx1 = fx0 - 1.0f;
	float fy1 = fy0 - 1.0f;
	int ix1 = (ix0 + 1) & 0xFF;
	int iy1 = (iy0 + 1) & 0xFF;

	ix0 = ix0 & 0xFF;
	iy0 = iy0 & 0xFF;

	float t = fade(fy0);
	float s = fade(fx0);
	float nx0 = grad(perm[ix0 + perm[iy0]], fx0, fy0);
	float nx1 = grad(perm[ix0 + perm[iy1]], fx0, fy1);
	float n0 = lerp(t, nx0, nx1);

	nx0 = grad(perm[ix1 + perm[iy0]], fx1, fy0);
	nx1 = grad(perm[ix1 + perm[iy1]], fx1, fy1);

	float n1 = lerp(t, nx0, nx1);

	return 0.507f * lerp(s, n0, n1);
}

// Confirmed (asm lines 435323-435804)
float Noise1234::noise(float x, float y, float z) {
	int ix0 = fastFloor(x);
	int iy0 = fastFloor(y);
	int iz0 = fastFloor(z);
	float fx0 = x - static_cast<float>(ix0);
	float fy0 = y - static_cast<float>(iy0);
	float fz0 = z - static_cast<float>(iz0);
	float fx1 = fx0 - 1.0f;
	float fy1 = fy0 - 1.0f;
	float fz1 = fz0 - 1.0f;
	int ix1 = (ix0 + 1) & 0xFF;
	int iy1 = (iy0 + 1) & 0xFF;
	int iz1 = (iz0 + 1) & 0xFF;

	ix0 = ix0 & 0xFF;
	iy0 = iy0 & 0xFF;
	iz0 = iz0 & 0xFF;

	float r = fade(fz0);
	float t = fade(fy0);
	float s = fade(fx0);
	float nxy0 = grad(perm[ix0 + perm[iy0 + perm[iz0]]], fx0, fy0, fz0);
	float nxy1 = grad(perm[ix0 + perm[iy0 + perm[iz1]]], fx0, fy0, fz1);
	float nx0 = lerp(r, nxy0, nxy1);

	nxy0 = grad(perm[ix0 + perm[iy1 + perm[iz0]]], fx0, fy1, fz0);
	nxy1 = grad(perm[ix0 + perm[iy1 + perm[iz1]]], fx0, fy1, fz1);

	float nx1 = lerp(r, nxy0, nxy1);
	float n0 = lerp(t, nx0, nx1);

	nxy0 = grad(perm[ix1 + perm[iy0 + perm[iz0]]], fx1, fy0, fz0);
	nxy1 = grad(perm[ix1 + perm[iy0 + perm[iz1]]], fx1, fy0, fz1);
	nx0 = lerp(r, nxy0, nxy1);
	nxy0 = grad(perm[ix1 + perm[iy1 + perm[iz0]]], fx1, fy1, fz0);
	nxy1 = grad(perm[ix1 + perm[iy1 + perm[iz1]]], fx1, fy1, fz1);
	nx1 = lerp(r, nxy0, nxy1);

	float n1 = lerp(t, nx0, nx1);

	return 0.936f * lerp(s, n0, n1);
}

// Confirmed (asm lines 436310-437359)
float Noise1234::noise(float x, float y, float z, float w) {
	int ix0 = fastFloor(x);
	int iy0 = fastFloor(y);
	int iz0 = fastFloor(z);
	int iw0 = fastFloor(w);
	float fx0 = x - static_cast<float>(ix0);
	float fy0 = y - static_cast<float>(iy0);
	float fz0 = z - static_cast<float>(iz0);
	float fw0 = w - static_cast<float>(iw0);
	float fx1 = fx0 - 1.0f;
	float fy1 = fy0 - 1.0f;
	float fz1 = fz0 - 1.0f;
	float fw1 = fw0 - 1.0f;
	int ix1 = (ix0 + 1) & 0xFF;
	int iy1 = (iy0 + 1) & 0xFF;
	int iz1 = (iz0 + 1) & 0xFF;
	int iw1 = (iw0 + 1) & 0xFF;

	ix0 = ix0 & 0xFF;
	iy0 = iy0 & 0xFF;
	iz0 = iz0 & 0xFF;
	iw0 = iw0 & 0xFF;

	float q = fade(fw0);
	float r = fade(fz0);
	float t = fade(fy0);
	float s = fade(fx0);

	// the sixteen corners: x and y and z and w each 0 or 1
	int ix[2] = {ix0, ix1};
	int iy[2] = {iy0, iy1};
	int iz[2] = {iz0, iz1};
	int iw[2] = {iw0, iw1};
	float fx[2] = {fx0, fx1};
	float fy[2] = {fy0, fy1};
	float fz[2] = {fz0, fz1};
	float fw[2] = {fw0, fw1};
	float nx[2];

	for (int a = 0; a < 2; a++) {
		float ny[2];

		for (int b = 0; b < 2; b++) {
			float nz[2];

			for (int c = 0; c < 2; c++) {
				float lower = grad(perm[ix[a] + perm[iy[b] + perm[iz[c] + perm[iw[0]]]]], fx[a], fy[b], fz[c], fw[0]);
				float upper = grad(perm[ix[a] + perm[iy[b] + perm[iz[c] + perm[iw[1]]]]], fx[a], fy[b], fz[c], fw[1]);

				nz[c] = lerp(q, lower, upper);
			}

			ny[b] = lerp(r, nz[0], nz[1]);
		}

		nx[a] = lerp(t, ny[0], ny[1]);
	}

	return 0.87f * lerp(s, nx[0], nx[1]);
}

// Confirmed (asm lines 434130-434207)
float SimplexNoise1234::noise(float x) {
	int i0 = fastFloor(x);
	int i1 = i0 + 1;
	float x0 = x - static_cast<float>(i0);
	float x1 = x0 - 1.0f;
	float t0 = 1.0f - x0 * x0;

	t0 *= t0;

	float n0 = t0 * t0 * grad(perm[i0 & 0xFF], x0);
	float t1 = 1.0f - x1 * x1;

	t1 *= t1;

	float n1 = t1 * t1 * grad(perm[i1 & 0xFF], x1);

	return 0.395f * (n0 + n1);
}

// Confirmed (asm lines 434207-434500). The skew factors are doubles in the binary.
float SimplexNoise1234::noise(float x, float y) {
	const double F2 = 0.366025403;
	const double G2 = 0.211324865;
	float s = static_cast<float>(static_cast<double>(x + y) * F2);
	float xs = x + s;
	float ys = y + s;
	int i = fastFloor(xs);
	int j = fastFloor(ys);
	float t = static_cast<float>(static_cast<double>(static_cast<float>(i + j)) * G2);
	float X0 = static_cast<float>(i) - t;
	float Y0 = static_cast<float>(j) - t;
	float x0 = x - X0;
	float y0 = y - Y0;
	int i1;
	int j1;

	if (x0 > y0) {
		i1 = 1;
		j1 = 0;
	} else {
		i1 = 0;
		j1 = 1;
	}

	float x1 = static_cast<float>(static_cast<double>(x0 - static_cast<float>(i1)) + G2);
	float y1 = static_cast<float>(static_cast<double>(y0 - static_cast<float>(j1)) + G2);
	float x2 = static_cast<float>(static_cast<double>(x0 - 1.0f) + 0.42264973);
	float y2 = static_cast<float>(static_cast<double>(y0 - 1.0f) + 0.42264973);
	int ii = i & 0xFF;
	int jj = j & 0xFF;
	float n0;
	float n1;
	float n2;
	float t0 = 0.5f - x0 * x0 - y0 * y0;

	if (t0 < 0.0f) {
		n0 = 0.0f;
	} else {
		t0 *= t0;
		n0 = t0 * t0 * grad(perm[ii + perm[jj]], x0, y0);
	}

	float t1 = 0.5f - x1 * x1 - y1 * y1;

	if (t1 < 0.0f) {
		n1 = 0.0f;
	} else {
		t1 *= t1;
		n1 = t1 * t1 * grad(perm[ii + i1 + perm[jj + j1]], x1, y1);
	}

	float t2 = 0.5f - x2 * x2 - y2 * y2;

	if (t2 < 0.0f) {
		n2 = 0.0f;
	} else {
		t2 *= t2;
		n2 = t2 * t2 * grad(perm[ii + 1 + perm[jj + 1]], x2, y2);
	}

	return 45.23f * (n0 + n1 + n2);
}

// Comparison of a rendered screen with an oracle screenshot (PPM written by the host's HOST_SHOT): mismatching pixels per region, and a PPM that shows them in red.
#ifndef TESTS_SCREEN_COMPARE_H
#define TESTS_SCREEN_COMPARE_H

#include <cstdio>
#include <string>
#include <vector>

#include "common/types.h"
#include "legend/surface.h"

// Compares our screen with an oracle screenshot (PPM written by the host's HOST_SHOT): mismatching pixels per region, and a PPM that shows them in red.
static inline bool loadPpm(const char *path, std::vector<byte> &rgb) {
	FILE *f = fopen(path, "rb");
	if (!f)
		return false;
	int w = 0, h = 0, m = 0;
	bool ok = fscanf(f, "P6 %d %d %d", &w, &h, &m) == 3 && w == 320 && h == 200 && fgetc(f) >= 0;
	rgb.resize(320 * 200 * 3);
	ok = ok && fread(&rgb[0], 1, rgb.size(), f) == rgb.size();
	fclose(f);
	return ok;
}

static inline int compareScreens(const Legend::Surface &ours, const std::vector<byte> &oracle, const char *diffPath) {
	static const struct { const char *name; int x1, y1, x2, y2; } kRegions[] = {
		{"verbs", 0, 0, 49, 127}, {"picture window", 50, 3, 315, 126}, {"text window", 50, 127, 315, 150}, {"compass", 0, 127, 49, 166}, {"buttons", 0, 167, 49, 199},
		{"slot panel", 50, 151, 84, 199}, {"list panel", 85, 151, 289, 199}, {"scroll panel", 290, 151, 319, 199}, {"everything", 0, 0, 319, 199}};
	std::vector<byte> rgb(320 * 200 * 3);
	ours.toRgb(&rgb[0]);
	std::vector<byte> diff(rgb);
	for (int i = 0; i < 320 * 200; i++) {
		if (memcmp(&rgb[i * 3], &oracle[i * 3], 3) != 0) {
			diff[i * 3] = 255;
			diff[i * 3 + 1] = diff[i * 3 + 2] = 0;
		}
	}
	int hist[256] = {0};
	for (int i = 0; i < 320 * 200; i++)
		if (diff[i * 3] == 255 && diff[i * 3 + 1] == 0 && memcmp(&rgb[i * 3], &oracle[i * 3], 3) != 0)
			hist[ours.pixels()[i]]++;
	printf("  palette indices of differing pixels:");
	for (int i = 0; i < 256; i++)
		if (hist[i] > 20)
			printf(" %d:%d", i, hist[i]);
	printf("\n");
	for (int r = 0; r < ARRAYSIZE(kRegions); r++) {
		int bad = 0, total = 0;
		for (int y = kRegions[r].y1; y <= kRegions[r].y2; y++)
			for (int x = kRegions[r].x1; x <= kRegions[r].x2; x++, total++)
				bad += memcmp(&rgb[(y * 320 + x) * 3], &oracle[(y * 320 + x) * 3], 3) != 0;
		printf("  %-15s %6d of %6d pixels differ\n", kRegions[r].name, bad, total);
	}
	int all = 0;
	for (int i = 0; i < 320 * 200; i++)
		all += memcmp(&rgb[i * 3], &oracle[i * 3], 3) != 0;
	if (diffPath) {
		FILE *f = fopen(diffPath, "wb");
		if (f) {
			fprintf(f, "P6\n320 200\n255\n");
			fwrite(&diff[0], 1, diff.size(), f);
			fclose(f);
		}
	}
	return all;
}


#endif

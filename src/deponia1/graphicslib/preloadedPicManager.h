// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/graphicslib/preloadedPicManager.cpp - see manifest/source_layout.tsv.
#pragma once

#include <vector>

class TPictureIO;

class TPreloadedPicManager {
public:
	void StopPreloading(TPictureIO *picture);
	// Confirmed call shape only (TCAnimation::PreloadSprites): queues a picture
	// for the background loader; not reconstructed (the picture is loaded when it
	// is first drawn).
	void PreloadPicture(TPictureIO *picture);
	// Confirmed call shape only (TGameControl::Update, Deponia_Linux.asm
	// lines 469773-469780) - returned by value via a nested-container
	// iteration (an outer "bucket" list of inner pointer arrays in the
	// original) that this reconstruction flattens into a single vector,
	// since nothing depends on the original's exact bucketing; not reversed
	// beyond that call shape.
	std::vector<TPictureIO *> GetPreloadedPictures();
};

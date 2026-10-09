// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/graphicslib/preloadedPicManager.cpp - see manifest/source_layout.tsv.
#pragma once

#include <vector>

class TPictureIO;

class TPreloadedPicManager {
public:
	void StopPreloading(TPictureIO *picture);
	// asm 779878-780419: the picture is taken out of the manager (it was made a sprite); nothing is queued here.
	void ReleasePicture(TPictureIO *picture);
	// Confirmed call shape only (TCAnimation::PreloadSprites): queues a picture
	// for the background loader; not reconstructed (the picture is loaded when it
	// is first drawn).
	void PreloadPicture(TPictureIO *picture);
	// Confirmed call shapes only (TGAnimation::~TGAnimation(), PreloadAnimation(),
	// Deponia_Linux.asm lines 156633, 157186): the same two operations for all the
	// pictures of an animation at once (the second takes the vector by value);
	// not reconstructed.
	void StopPreloading(std::vector<TPictureIO *> &pictures);
	void PreloadPictures(std::vector<TPictureIO *> pictures);
	// Confirmed call shape only (TGAnimation::LoadAnimations()/StartAnimation(),
	// asm lines 153788-153792, 158340-158343): whether the vector at +0x100 of the
	// manager is not empty, which decides if a new animation's pictures are
	// handed to it; not reconstructed (always false, nothing is queued).
	bool HasQueuedPictures() const;
	// Confirmed call shape only (TGameControl::Update, Deponia_Linux.asm
	// lines 469773-469780) - returned by value via a nested-container
	// iteration (an outer "bucket" list of inner pointer arrays in the
	// original) that this reconstruction flattens into a single vector,
	// since nothing depends on the original's exact bucketing; not reversed
	// beyond that call shape.
	std::vector<TPictureIO *> GetPreloadedPictures();
};

// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/graphicslib/preloadedPicManager.cpp - see manifest/source_layout.tsv.
#pragma once

class TPictureIO;

class TPreloadedPicManager {
public:
    void StopPreloading(TPictureIO* picture);
};

// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vsplayer/control/loadingControl.cpp - see manifest/source_layout.tsv.
#pragma once

#include "TPaintControl.h"

class TLoadingControl : public TPaintControl {
public:
    void UpdateStatus(int current, int total);
};

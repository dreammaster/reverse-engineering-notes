// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vsplayer/control/gameControl.cpp - see manifest/source_layout.tsv.
//
// TGameControl is the concrete class that overrides TMasterControl's 8
// pure virtuals (confirmed: TMasterControl's constructor/destructor CODE
// XREFs all point into TGameControl's). It has 98 of its own methods
// (manifest/proprietary_classes.tsv) - a full reconstruction is its own
// dedicated pass. This stub exists only so TMasterControl is instantiable
// (AppGlobals.cpp needs a concrete object).
#pragma once

#include "vsplayer/control/masterControl.h"

class TGameControl : public TMasterControl {
public:
    void OnGameSignal(const TSignalData& signal, TSignalData& result) override;
    bool ShouldSkipActionText() const override;
    bool ShouldSkipNormalDraw() const override;
    void UnknownVirtualSlot30() override;
    void OnAfterMatrixReset() override;
    void UnknownVirtualSlot40() override;
    void UnknownVirtualSlot48() override;
    void UnknownVirtualSlot50() override;
};

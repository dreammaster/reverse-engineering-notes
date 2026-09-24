// Reconstructed from Deponia_Linux.asm, TGameController methods at asm
// lines 450097-453674 (address range 0x60BA70-0x60E5F8). See NOTES.md.
//
// Original path confirmed via the x_assert() call in RemoveGameController():
// src/vsplayer/control/gameController.cpp - see manifest/source_layout.tsv.
#pragma once

#include <utility>
#include <vector>

#include "SdlStub.h"
#include "WxStub.h"

// Ordinal groups recovered from ConvertHapticEffectType's lookup table
// range checks in each HapticNewEffect* method: value 0 is accepted only by
// HapticNewEffectConstant, 1-5 only by HapticNewEffectPeriodic, 6 only by
// HapticNewEffectRamp, 7-10 only by HapticNewEffectCondition. That grouping
// matches real SDL2's own effect-type families (one constant type, five
// periodic waveforms, one ramp type, four condition types), which is also
// the basis for guessing these enumerator names - the actual identifiers
// Visionnaire used are not recoverable from the binary.
// Note the overlap: HapticNewEffectLeftRight requires effectType == 2
// exactly (TCET_LeftRight), while HapticNewEffectPeriodic's own check
// accepts the contiguous range [1,5] (a single "(type-1) <= 4" comparison,
// cheaper than an exact match per waveform) - which technically also lets 2
// through. That's harmless as long as calling code never actually passes
// TCET_LeftRight to HapticNewEffectPeriodic, which - per their distinct
// call sites (CmdCreateHapticEffectLeftRight vs. ...Periodic) - it doesn't.
enum TControllerEffectType {
    TCET_Constant = 0,
    TCET_Sine = 1,
    TCET_LeftRight = 2,
    TCET_Triangle = 3,
    TCET_SawtoothUp = 4,
    TCET_SawtoothDown = 5,
    TCET_Ramp = 6,
    TCET_Spring = 7,
    TCET_Damper = 8,
    TCET_Inertia = 9,
    TCET_Friction = 10,
};

// Recovered the same way from ConvertHapticEffectDirection: value 1 uses one
// direction component, 2 uses three, 3 uses two - matching SDL_HAPTIC_POLAR
// (1 component), SDL_HAPTIC_CARTESIAN (3) and SDL_HAPTIC_SPHERICAL (2)
// respectively.
enum TControllerEffectDirection {
    TCED_Polar = 1,
    TCED_Cartesian = 2,
    TCED_Spherical = 3,
};

int ConvertHapticEffectType(TControllerEffectType type);
int ConvertHapticEffectDirection(TControllerEffectDirection direction);

class TGameController {
public:
    TGameController();
    virtual ~TGameController();

    // Called with the raw controller-axis position and the game's mouse
    // deadzone/scale settings; writes the resulting cursor delta into the
    // movex/movey globals.
    static void ControllerAxisMouseMove(wxPoint pos, int threshold, int scaleAdjust);

    // Same idea for character (analog-walk) movement, but stateful: it also
    // detects the stick returning to center and flags stopped_char so
    // walk-cycle animation can stop. See TStandardPaths-style globals in
    // AppGlobals.
    static void ControllerAxisCharacterMove(wxPoint pos, int threshold);

    bool HapticStartRumble(float strength, unsigned int lengthMs);
    bool HapticStopRumble();
    bool HapticStartEffect(int effectHandle);
    bool HapticStopEffect(int effectHandle);
    bool HapticControllerStartEffect(int localEffectIndex);
    bool HapticControllerStopEffect(int localEffectIndex);
    bool HapticStopAll();

    // Returns the joystick instance ID on success (existing entry's ID if
    // this joystick was already added), or -1 on failure.
    int AddGameController(int joystickIndex);
    // Returns the removed controller's instance ID.
    int RemoveGameController(int instanceId);

    // Uploads `effect` to every haptic-capable controller that reports
    // support for `effectType`'s SDL effect-type bit, assigns it a new
    // shared handle, and returns that handle (or -1 if no controller
    // accepted it).
    int HapticControllerEffectUpload(TControllerEffectType effectType, SDL_HapticEffect effect);

    // Each of these builds an SDL_HapticEffect from its parameters (a
    // parameter value of -1 means "leave this field at its zero default")
    // and uploads it via HapticControllerEffectUpload. Each only accepts
    // the TControllerEffectType subrange matching its SDL effect family
    // (see the enum comment above) and returns -1 without building
    // anything otherwise.
    int HapticNewEffectLeftRight(TControllerEffectType effectType, int length, int largeMagnitude,
                                  int smallMagnitude);
    int HapticNewEffectRamp(TControllerEffectType effectType, TControllerEffectDirection direction,
                             int* dirComponents, int length, int delay, int button, int interval, int start, int end,
                             int attackLength, int fadeLength);
    int HapticNewEffectPeriodic(TControllerEffectType effectType, TControllerEffectDirection direction,
                                 int* dirComponents, int length, int delay, int button, int interval, int period,
                                 int magnitude, int offset, int phase, int attackLength, int fadeLength);
    int HapticNewEffectCondition(TControllerEffectType effectType, int length, int delay, int rightSat, int leftSat,
                                  int rightCoeff, int leftCoeff, int deadband, int center);
    int HapticNewEffectConstant(TControllerEffectType effectType, TControllerEffectDirection direction,
                                 int* dirComponents, int length, int delay, int level, int attackLength,
                                 int attackLevel, int fadeLength, int fadeLevel);

private:
    static std::vector<std::pair<SDL_GameController*, SDL_Haptic*>> m_gamecontrollers;
    // {handle, per-controller SDL effect index} - handle is a sequentially
    // assigned id (current size + 1 at upload time) shared by every
    // controller a given HapticNewEffect* call succeeded on; see
    // HapticControllerEffectUpload.
    static std::vector<std::pair<int, int>> m_gamecontrollerHapticEffects;
};

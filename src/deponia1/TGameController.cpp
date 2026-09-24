#include "TGameController.h"

#include <cmath>
#include <cstring>

#include "AppGlobals.h"
#include "Diagnostics.h"

std::vector<std::pair<SDL_GameController*, SDL_Haptic*>> TGameController::m_gamecontrollers;
std::vector<std::pair<int, int>> TGameController::m_gamecontrollerHapticEffects;

// CSWTCH_185 or/CSWTCH_188's exact values were not read from the binary
// (their content depends on SDL's SDL_HapticEffectType/SDL_HapticDirectionType
// enumerators, which are already just re-exposing SDL2's own bit values) -
// forwarding the enum straight through is the actual intended behavior:
// TControllerEffectType/TControllerEffectDirection's ordinals were chosen
// (see TGameController.h) to already match what SDL expects.
int ConvertHapticEffectType(TControllerEffectType type) {
    if (static_cast<unsigned>(type) > 10)
        return -1;
    switch (type) {
    case TCET_Constant: return 1 << 0;      // SDL_HAPTIC_CONSTANT
    case TCET_Sine: return 1 << 3;          // SDL_HAPTIC_SINE
    case TCET_LeftRight: return 1 << 10;    // SDL_HAPTIC_LEFTRIGHT
    case TCET_Triangle: return 1 << 4;      // SDL_HAPTIC_TRIANGLE
    case TCET_SawtoothUp: return 1 << 5;    // SDL_HAPTIC_SAWTOOTHUP
    case TCET_SawtoothDown: return 1 << 6;  // SDL_HAPTIC_SAWTOOTHDOWN
    case TCET_Ramp: return 1 << 2;          // SDL_HAPTIC_RAMP
    case TCET_Spring: return 1 << 16;       // SDL_HAPTIC_SPRING
    case TCET_Damper: return 1 << 17;       // SDL_HAPTIC_DAMPER
    case TCET_Inertia: return 1 << 18;      // SDL_HAPTIC_INERTIA
    case TCET_Friction: return 1 << 19;     // SDL_HAPTIC_FRICTION
    }
    return -1;
}

int ConvertHapticEffectDirection(TControllerEffectDirection direction) {
    switch (direction) {
    case TCED_Polar: return 0;       // SDL_HAPTIC_POLAR
    case TCED_Cartesian: return 1;   // SDL_HAPTIC_CARTESIAN
    case TCED_Spherical: return 2;   // SDL_HAPTIC_SPHERICAL
    }
    return -1;
}

TGameController::TGameController() = default;
TGameController::~TGameController() = default;

void TGameController::ControllerAxisMouseMove(wxPoint pos, int threshold, int scaleAdjust) {
    int divisor = threshold - scaleAdjust;
    movex = (std::abs(pos.x) > threshold) ? (pos.x / divisor) : 0;
    movey = (std::abs(pos.y) > threshold) ? (pos.y / divisor) : 0;
}

void TGameController::ControllerAxisCharacterMove(wxPoint pos, int threshold) {
    // Contribution carried into the shared Y-axis check below: abs(pos.x)
    // and pos.x itself when X is beyond the threshold, or 0/0 when it's
    // centered (matches the eax/ecx values the disassembly carries between
    // its X-handling and Y-handling halves).
    int xMagnitude;
    int xRaw;

    if (std::abs(pos.x) > threshold) {
        charmovex = pos.x;
        xMagnitude = std::abs(pos.x);
        xRaw = pos.x;
    } else {
        int oldX = charmovex;
        int oldY = charmovey;
        charmovex = 0;
        if (oldY == 0 && oldX != 0) {
            stopped_char = 1;
        }
        xMagnitude = 0;
        xRaw = 0;
    }

    if (std::abs(pos.y) > threshold) {
        charmovey = pos.y;
        return;
    }

    int oldCharmovey = charmovey;
    charmovey = 0;
    if (xMagnitude + std::abs(oldCharmovey) == 0)
        return;
    if (xRaw != 0)
        return;
    stopped_char = 1;
}

bool TGameController::HapticStartRumble(float strength, unsigned int lengthMs) {
    bool result = false;
    for (auto& [controller, haptic] : m_gamecontrollers) {
        (void)controller;
        if (!haptic)
            continue;
        SDL_HapticRumblePlay(haptic, strength, lengthMs);
        result = true;
    }
    return result;
}

bool TGameController::HapticStopRumble() {
    bool result = false;
    for (auto& [controller, haptic] : m_gamecontrollers) {
        (void)controller;
        if (!haptic)
            continue;
        SDL_HapticRumbleStop(haptic);
        result = true;
    }
    return result;
}

bool TGameController::HapticStartEffect(int effectHandle) {
    bool result = false;
    for (auto& [handle, localIndex] : m_gamecontrollerHapticEffects) {
        if (handle != effectHandle)
            continue;
        bool ranHere = false;
        for (auto& [controller, haptic] : m_gamecontrollers) {
            (void)controller;
            if (!haptic)
                continue;
            if (SDL_HapticRunEffect(haptic, localIndex, 1) == 0)
                ranHere = true;
        }
        if (ranHere)
            result = true;
    }
    return result;
}

bool TGameController::HapticStopEffect(int effectHandle) {
    bool result = false;
    for (auto& [handle, localIndex] : m_gamecontrollerHapticEffects) {
        if (handle != effectHandle)
            continue;
        bool stoppedHere = false;
        for (auto& [controller, haptic] : m_gamecontrollers) {
            (void)controller;
            if (!haptic)
                continue;
            if (SDL_HapticStopEffect(haptic, localIndex) == 0)
                stoppedHere = true;
        }
        if (stoppedHere)
            result = true;
    }
    return result;
}

bool TGameController::HapticControllerStartEffect(int localEffectIndex) {
    bool result = false;
    for (auto& [controller, haptic] : m_gamecontrollers) {
        (void)controller;
        if (!haptic)
            continue;
        if (SDL_HapticRunEffect(haptic, localEffectIndex, 1) == 0)
            result = true;
    }
    return result;
}

bool TGameController::HapticControllerStopEffect(int localEffectIndex) {
    bool result = false;
    for (auto& [controller, haptic] : m_gamecontrollers) {
        (void)controller;
        if (!haptic)
            continue;
        if (SDL_HapticStopEffect(haptic, localEffectIndex) == 0)
            result = true;
    }
    return result;
}

bool TGameController::HapticStopAll() {
    bool result = false;
    for (auto& [controller, haptic] : m_gamecontrollers) {
        (void)controller;
        if (!haptic)
            continue;
        SDL_HapticStopAll(haptic);
        result = true;
    }
    return result;
}

int TGameController::AddGameController(int joystickIndex) {
    if (!SDL_IsGameController(joystickIndex))
        return -1;

    SDL_GameController* controller = SDL_GameControllerOpen(joystickIndex);
    int newInstanceId = SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(controller));

    for (auto& [existingController, existingHaptic] : m_gamecontrollers) {
        (void)existingHaptic;
        if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(existingController)) == newInstanceId) {
            // Already tracked (e.g. a duplicate add event) - close the
            // newly-opened handle and report the existing entry's id.
            SDL_GameControllerClose(controller);
            return SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(existingController));
        }
    }

    if (!controller)
        return -1;

    if (wxLog::loglevel > 1) {
        wxString name;
        toUTF(&name, SDL_GameControllerName(controller));
        wxLog::logexpanded(L"Added controller: %s", name.c_str());
    }

    SDL_Joystick* joystick = SDL_GameControllerGetJoystick(controller);
    SDL_Haptic* haptic = nullptr;

    if (SDL_JoystickIsHaptic(joystick)) {
        haptic = SDL_HapticOpenFromJoystick(joystick);
        if (wxLog::loglevel > 1) {
            wxLog::logexpanded(L"Haptic effects available: %d", SDL_HapticNumEffects(haptic));
            wxLog::logexpanded(L"Haptic features: %d", SDL_HapticQuery(haptic));
        }
        if (SDL_HapticRumbleSupported(haptic)) {
            if (SDL_HapticRumbleInit(haptic) != 0) {
                if (wxLog::loglevel > 1) {
                    wxString error;
                    toUTF(&error, SDL_GetError());
                    wxLog::logexpanded(L"Haptic rumble init failed: %s", error.c_str());
                }
                SDL_HapticClose(haptic);
                haptic = nullptr;
            }
        } else {
            SDL_HapticClose(haptic);
            if (wxLog::loglevel > 1)
                wxLog::logexpanded(L"Haptic rumble not supported");
            haptic = nullptr;
        }
    }

    m_gamecontrollers.push_back({controller, haptic});
    return SDL_JoystickInstanceID(joystick);
}

int TGameController::RemoveGameController(int instanceId) {
    auto it = m_gamecontrollers.begin();
    for (; it != m_gamecontrollers.end(); ++it) {
        if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(it->first)) == instanceId)
            break;
    }
    x_assert(it != m_gamecontrollers.end(), "gamecontroller_it != m_gamecontrollers.end()",
              "/home/simon/Documents/jenkins/branchPillars/src/vsplayer/control/gameController.cpp", 0xC6);

    if (it->second) {
        SDL_HapticClose(it->second);
        it->second = nullptr;
    }
    if (it->first) {
        if (wxLog::loglevel > 1) {
            wxString name;
            toUTF(&name, SDL_GameControllerName(it->first));
            // The reversed call site passes this converted name as a
            // logexpanded vararg against a format string that's just a
            // single space with no "%s" in it, so the name is computed but
            // never actually shown - reproduced as-is rather than "fixed".
            wxLog::logexpanded(L" ");
        }
        SDL_GameControllerClose(it->first);
        it->first = nullptr;
    }
    m_gamecontrollers.erase(it);
    return instanceId;
}

int TGameController::HapticControllerEffectUpload(TControllerEffectType effectType, SDL_HapticEffect effect) {
    int effectTypeBit = ConvertHapticEffectType(effectType);
    int newHandle = static_cast<int>(m_gamecontrollerHapticEffects.size()) + 1;
    int result = -1;

    for (auto& [controller, haptic] : m_gamecontrollers) {
        if (!haptic)
            continue;
        if (SDL_HapticQuery(haptic) & effectTypeBit) {
            int localEffectIndex = SDL_HapticNewEffect(haptic, &effect);
            if (localEffectIndex != -1) {
                m_gamecontrollerHapticEffects.push_back({newHandle, localEffectIndex});
                result = newHandle;
            }
        } else if (wxLog::loglevel > 0) {
            wxString name;
            toUTF(&name, SDL_GameControllerName(controller));
            wxLog::logexpanded(L"Controller %s does not support this haptic effect type", name.c_str());
        }
    }
    return result;
}

int TGameController::HapticNewEffectLeftRight(TControllerEffectType effectType, int length, int largeMagnitude,
                                               int smallMagnitude) {
    if (effectType != TCET_LeftRight) {
        if (wxLog::loglevel > 0)
            wxLog::logexpanded(L"Unsupported haptic effect type");
        return -1;
    }

    SDL_HapticEffect effect{};
    int typeBit = ConvertHapticEffectType(effectType);
    effect.leftright.type = static_cast<Uint16>(typeBit);
    if (length != -1)
        effect.leftright.length = static_cast<Uint32>(length);
    if (largeMagnitude != -1)
        effect.leftright.large_magnitude = static_cast<Uint16>(largeMagnitude);
    if (smallMagnitude != -1)
        effect.leftright.small_magnitude = static_cast<Uint16>(smallMagnitude);

    return HapticControllerEffectUpload(effectType, effect);
}

static void ApplyDirection(SDL_HapticDirection& outDirection, TControllerEffectDirection direction,
                            const int* components) {
    outDirection.type = static_cast<Uint8>(ConvertHapticEffectDirection(direction));
    switch (direction) {
    case TCED_Polar:
        if (components[0])
            outDirection.dir[0] = components[0];
        break;
    case TCED_Cartesian:
        if (components[0])
            outDirection.dir[0] = components[0];
        if (components[1])
            outDirection.dir[1] = components[1];
        if (components[2])
            outDirection.dir[2] = components[2];
        break;
    case TCED_Spherical:
        if (components[0])
            outDirection.dir[0] = components[0];
        if (components[1])
            outDirection.dir[1] = components[1];
        break;
    }
}

int TGameController::HapticNewEffectRamp(TControllerEffectType effectType, TControllerEffectDirection direction,
                                          int* dirComponents, int length, int delay, int button, int interval,
                                          int start, int end, int attackLength, int fadeLength) {
    int typeBit = ConvertHapticEffectType(effectType);
    (void)typeBit;

    if (effectType != TCET_Ramp) {
        if (wxLog::loglevel > 0)
            wxLog::logexpanded(L"Unsupported haptic effect type");
        return -1;
    }

    SDL_HapticEffect effect{};
    effect.ramp.type = static_cast<Uint16>(ConvertHapticEffectType(effectType));
    ApplyDirection(effect.ramp.direction, direction, dirComponents);
    if (length != -1)
        effect.ramp.length = static_cast<Uint32>(length);
    if (delay != -1)
        effect.ramp.delay = static_cast<Uint16>(delay);
    if (button != -1)
        effect.ramp.button = static_cast<Uint16>(button);
    if (interval != -1)
        effect.ramp.interval = static_cast<Uint16>(interval);
    if (start != -1)
        effect.ramp.start = static_cast<Sint16>(start);
    if (end != -1)
        effect.ramp.end = static_cast<Sint16>(end);
    if (attackLength != -1)
        effect.ramp.attack_length = static_cast<Uint16>(attackLength);
    if (fadeLength != -1)
        effect.ramp.fade_length = static_cast<Uint16>(fadeLength);

    return HapticControllerEffectUpload(effectType, effect);
}

int TGameController::HapticNewEffectPeriodic(TControllerEffectType effectType, TControllerEffectDirection direction,
                                              int* dirComponents, int length, int delay, int button, int interval,
                                              int period, int magnitude, int offset, int phase, int attackLength,
                                              int fadeLength) {
    if (effectType < TCET_Sine || effectType > TCET_SawtoothDown) {
        if (wxLog::loglevel > 0)
            wxLog::logexpanded(L"Unsupported haptic effect type");
        return -1;
    }

    SDL_HapticEffect effect{};
    effect.periodic.type = static_cast<Uint16>(ConvertHapticEffectType(effectType));
    ApplyDirection(effect.periodic.direction, direction, dirComponents);
    if (length != -1)
        effect.periodic.length = static_cast<Uint32>(length);
    if (delay != -1)
        effect.periodic.delay = static_cast<Uint16>(delay);
    if (button != -1)
        effect.periodic.button = static_cast<Uint16>(button);
    if (interval != -1)
        effect.periodic.interval = static_cast<Uint16>(interval);
    if (period != -1)
        effect.periodic.period = static_cast<Uint16>(period);
    if (magnitude != -1)
        effect.periodic.magnitude = static_cast<Sint16>(magnitude);
    if (offset != -1)
        effect.periodic.offset = static_cast<Sint16>(offset);
    if (phase != -1)
        effect.periodic.phase = static_cast<Uint16>(phase);
    if (attackLength != -1)
        effect.periodic.attack_length = static_cast<Uint16>(attackLength);
    if (fadeLength != -1)
        effect.periodic.fade_length = static_cast<Uint16>(fadeLength);

    return HapticControllerEffectUpload(effectType, effect);
}

int TGameController::HapticNewEffectCondition(TControllerEffectType effectType, int length, int delay, int rightSat,
                                               int leftSat, int rightCoeff, int leftCoeff, int deadband, int center) {
    if (effectType < TCET_Spring || effectType > TCET_Friction) {
        if (wxLog::loglevel > 0)
            wxLog::logexpanded(L"Unsupported haptic effect type");
        return -1;
    }

    SDL_HapticEffect effect{};
    effect.condition.type = static_cast<Uint16>(ConvertHapticEffectType(effectType));
    if (length != -1)
        effect.condition.length = static_cast<Uint32>(length);
    if (delay != -1)
        effect.condition.delay = static_cast<Uint16>(delay);

    // The disassembly generates one specialized loop per combination of
    // which of these six fields is present (a compiler artifact of hoisting
    // six loop-invariant "!= -1" checks out of a single per-axis loop, not
    // six original separate loops - see NOTES.md); this is the equivalent
    // un-hoisted source form.
    for (auto& [controller, haptic] : m_gamecontrollers) {
        (void)controller;
        if (!haptic)
            continue;
        for (int axis = 0; axis < SDL_HapticNumAxes(haptic); ++axis) {
            if (rightSat != -1)
                effect.condition.right_sat[axis] = static_cast<Uint16>(rightSat);
            if (leftSat != -1)
                effect.condition.left_sat[axis] = static_cast<Uint16>(leftSat);
            if (rightCoeff != -1)
                effect.condition.right_coeff[axis] = static_cast<Sint16>(rightCoeff);
            if (leftCoeff != -1)
                effect.condition.left_coeff[axis] = static_cast<Sint16>(leftCoeff);
            if (deadband != -1)
                effect.condition.deadband[axis] = static_cast<Uint16>(deadband);
            if (center != -1)
                effect.condition.center[axis] = static_cast<Sint16>(center);
        }
    }

    return HapticControllerEffectUpload(effectType, effect);
}

int TGameController::HapticNewEffectConstant(TControllerEffectType effectType, TControllerEffectDirection direction,
                                              int* dirComponents, int length, int delay, int level, int attackLength,
                                              int attackLevel, int fadeLength, int fadeLevel) {
    if (effectType != TCET_Constant) {
        if (wxLog::loglevel > 0)
            wxLog::logexpanded(L"Unsupported haptic effect type");
        return -1;
    }

    SDL_HapticEffect effect{};
    effect.constant.type = static_cast<Uint16>(ConvertHapticEffectType(effectType));
    ApplyDirection(effect.constant.direction, direction, dirComponents);
    if (length != -1)
        effect.constant.length = static_cast<Uint32>(length);
    if (delay != -1)
        effect.constant.delay = static_cast<Uint16>(delay);
    if (level != -1)
        effect.constant.level = static_cast<Sint16>(level);
    if (attackLength != -1)
        effect.constant.attack_length = static_cast<Uint16>(attackLength);
    if (attackLevel != -1)
        effect.constant.attack_level = static_cast<Uint16>(attackLevel);
    if (fadeLength != -1)
        effect.constant.fade_length = static_cast<Uint16>(fadeLength);
    if (fadeLevel != -1)
        effect.constant.fade_level = static_cast<Uint16>(fadeLevel);

    return HapticControllerEffectUpload(effectType, effect);
}

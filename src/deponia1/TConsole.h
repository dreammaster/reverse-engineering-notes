// Not yet assert-confirmed to a specific file; stays at the top level.
// Embedded by value in TGameControl (confirmed: TGameControl's ctor calls
// TConsole::TConsole() directly on an interior pointer). The in-game/dev
// console, matching TGameControl::GetConsole()/DisplayConsole()/
// DisplayInSceneConsole().
#pragma once

class TConsole {
public:
    TConsole() = default;
};

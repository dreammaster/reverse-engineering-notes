// Not yet assert-confirmed to a specific file; stays at the top level.
// Embedded by value in TGameControl (confirmed: TGameControl's ctor calls
// TConsole::TConsole() directly on an interior pointer). The in-game/dev
// console, matching TGameControl::GetConsole()/DisplayConsole()/
// DisplayInSceneConsole().
//
// Draw()/DrawInScene() are confirmed as tail-called directly from
// TGameControl::DisplayConsole()/DisplayInSceneConsole() (Deponia_Linux.asm
// lines 455750-455779) - only the fact that they exist and are called with
// no arguments is confirmed, not their internal drawing logic.
#pragma once

class TConsole {
public:
    TConsole() = default;

    bool Draw();
    void DrawInScene();
};

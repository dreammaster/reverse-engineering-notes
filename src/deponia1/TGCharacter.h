// Not yet assert-confirmed to a specific file; stays at the top level.
// An in-game character (position, animation, walking state); referenced
// throughout TGameControl (GetCurrentCharacter, GetCharacter,
// ChangeCharacter, etc.) but not itself reversed yet.
#pragma once

class TGCharacter {
public:
    TGCharacter() = default;

    // Confirmed called for every character (TGameControl::
    // SetCharacterInterfaces, Deponia_Linux.asm lines 458260-458285) - not
    // reversed beyond that call shape.
    void SetInterfaces();

    // Confirmed called per-character in TGameControl::HandleCharacters
    // (asm lines 460829-460866), in this order, every frame the scene isn't
    // a menu.
    void WalkWay();
    void UpdateCharacter();

    // Confirmed called for every character (TGameControl::
    // SetAllCharactersOnDestination, asm lines 460874-460902).
    void SetOnDestination();
};

// Not yet assert-confirmed to a specific file; stays at the top level.
// An in-game character (position, animation, walking state); referenced
// throughout TGameControl (GetCurrentCharacter, GetCharacter,
// ChangeCharacter, etc.) but not itself reversed yet.
#pragma once

class TGCharacter {
public:
    TGCharacter() = default;
};

// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed only by call shape (TGameControl::LoadAndInitGame,
// Deponia_Linux.asm lines 467773-467774, 467802): TGameControl's own `this`
// pointer is passed as a TSignalSlot* when isEditor is true, nullptr
// otherwise - implying TGameControl derives from (or otherwise converts to)
// TSignalSlot in the original. That relationship isn't modeled here (like
// THGameControl's own not-yet-integrated relationship - see NOTES.md); this
// is an empty placeholder purely so the call compiles.
#pragma once

class TSignalSlot {
};

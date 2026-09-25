// Not yet assert-confirmed to a specific file; stays at the top level.
// Generic message struct passed through TMasterControl::Signal (and
// presumably other Signal() overrides across the engine). Only the fields
// TMasterControl::Signal actually reads are represented; real TSignalData
// is almost certainly a tagged union with many more type-specific fields.
#pragma once

// Signal type codes TMasterControl::Signal recognizes (int at offset 0):
// see NOTES.md for how these were read directly off the switch/cmp chain.
constexpr int kSignalGameEvent = 0x1011;     // dispatched to a pure-virtual TGameControl override
constexpr int kSignalDrawInterfaces = 0x2001;
constexpr int kSignalDraw = 0x2002;
constexpr int kSignalLoadingProgress = 0x100;

struct TSignalData {
    int type = 0;
    int unknown1 = 0;
    int unknown2 = 0;
    int unknown3 = 0;
    int unknown4 = 0;
    // kSignalLoadingProgress reads two ints at offsets the disassembly
    // shows as +0x18/+0x1C from the struct start; approximated here as the
    // 7th/8th int (there isn't enough evidence to know what, if anything,
    // occupies bytes 0x04-0x18).
    int loadingCurrent = 0;
    int loadingTotal = 0;
};

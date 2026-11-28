// Not yet assert-confirmed to a specific file; stays at the top level.
// Generic message struct passed through TMasterControl::Signal (and
// presumably other Signal() overrides across the engine). Only the fields
// TMasterControl::Signal actually reads are represented; real TSignalData
// is almost certainly a tagged union with many more type-specific fields.
#pragma once

#include "WxStub.h"

// Signal type codes TMasterControl::Signal recognizes (int at offset 0):
// see NOTES.md for how these were read directly off the switch/cmp chain.
constexpr int kSignalGameEvent = 0x1011;     // dispatched to TMasterControl::Update() (pure virtual, TGameControl overrides it)
constexpr int kSignalDrawInterfaces = 0x2001;
constexpr int kSignalDraw = 0x2002;
constexpr int kSignalLoadingProgress = 0x100;
// TFontManager::Signal's two recognized types (Deponia_Linux.asm lines
// 1389694-1389697) - print a single line of `text` at `point`, or split
// `text` on '\n' and print each resulting line via PrintTextLines(),
// respectively. Both also read a font id at a THIRD offset that overlaps
// kSignalLoadingProgress's own loadingCurrent/loadingTotal fields in the
// real binary (a tagged union, per this struct's own top comment) - given
// separate, independently-named fields here instead, since nothing needs
// the two interpretations to share the same bytes in this reconstruction.
constexpr int kSignalPrintText = 0x2003;
constexpr int kSignalPrintTextLines = 0x2004;
// The signals TSText sends to the sound manager (TSignalSlot) for the speech of a text
// (TSText.cpp). Named by what the text does with the answer; the sound manager itself is
// not reconstructed. They take the speech file in `speechFile`; the answer has the same type
// as the question and its result in `value` (the fields start at -1, as in the original).
constexpr int kSignalSoundIsPlaying = 0x1000;    // value != 0: the file is still playing
constexpr int kSignalSoundFlag = 0x1001;         // value: a flag of the sound system (not used by the texts)
constexpr int kSignalSoundMute = 0x1002;         // value != 0: all sounds are muted (no answer)
constexpr int kSignalSoundsDisabled = 0x1003;    // value != 0: the player has sounds (and speech) turned off
constexpr int kSignalMusicVolume = 0x1004;       // value: the volume of the music (percent)
constexpr int kSignalSpeechVolume = 0x1005;      // value: the volume of the speech (what to play it with)
constexpr int kSignalMovieVolume = 0x1006;       // value: the volume of the movies
constexpr int kSignalGlobalVolume = 0x1007;      // value: the volume of everything
constexpr int kSignalSoundPlay = 0x1008;         // file; value: the volume, value2: the balance,
// value3: != 0 to loop, value4: the kind of sound (TSoundTypeEnum; 2 for the speech)
constexpr int kSignalSoundStop = 0x100F;         // the file stops
constexpr int kSignalSoundFade = 0x1010;         // value != 0: the fades go on (answers as kSignalSoundSystemReady)
constexpr int kSignalSoundSystemReady = 0x1012;  // value != 0: the sound system works (speech can be played)

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
	// kSignalPrintText/kSignalPrintTextLines fields (asm lines 1389717-
	// 1389742, 1389871-1389872): the text to print (single-line for
	// kSignalPrintText, '\n'-delimited for kSignalPrintTextLines), the
	// screen position to print it at, and (kSignalPrintTextLines only) a
	// raw packed font id (see PackVisId()) to select the font by first.
	wxString text;
	wxPoint point;
	int fontId = 0;
	// The speech signals (above): the file (+0x10 in the original), and a result or arguments
	// (+0x18, +0x1C, +0x20 and +0x24; the first two are also what kSignalLoadingProgress reads).
	wxString speechFile;
	int value = -1;
	int value2 = -1;
	int value3 = -1;
	int value4 = -1;
};

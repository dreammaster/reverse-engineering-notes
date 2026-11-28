// Original path confirmed via x_assert() calls (not yet reconstructed):
// src/vscommon/fontManager.cpp - see manifest/source_layout.tsv.
//
// Confirmed layout (ctor/dtor/GetFont/GetCurrentFont/SetCurrentFont x2/
// Initialize, Deponia_Linux.asm lines 1388826-1390289): owns every loaded
// TCFont in a plain vector (positionally indexable - SetCurrentFont(int&)
// needs that), plus a hash table keyed by the same packed-id scheme as
// TGameControl's own _charactersByHash for fast lookup by TVisObjRef -
// modeled the same way, as a plain std::unordered_map rather than
// replicating the original's open-hashing bucket/node layout. A "current
// font" is tracked directly rather than via the original's pointer-into-
// the-values-array/sentinel trick, since a plain nullable TCFont* is
// behaviorally identical for every confirmed caller.
//
// The constructor also writes a vtable pointer (`off_E03A80`), so the real
// TFontManager is polymorphic - Signal()'s signature exactly matches
// TMasterControl::Signal()'s (also virtual), hinting at a shared, not-yet-
// identified signal-handler base interface connecting the two. Not modeled
// (TFontManager is declared non-polymorphic here) since nothing currently
// depends on dispatching through it virtually.
#pragma once

#include <list>
#include <unordered_map>
#include <vector>

#include "WxStub.h"
#include "datastruct/visobjref.h"
#include "datastruct/vlist.h"

/** Confirmed values (TCFont::GetTextStartPos(), asm 1392196; the data and the actions use the same numbers): 0 left, 1 right,
 *  2 centre; 3 to 5 are for a text in a box of a given width (names invented): 3 the box is centred on the point and the
 *  text starts at its left edge, 4 the box is centred on the point and the text ends at its right edge, 5 the text is
 *  centred in a box that starts at the point. */
enum class TextAlignmentEnum { kLeft = 0, kRight = 1, kCenter = 2, kBoxLeft = 3, kBoxRight = 4, kBoxCenter = 5 };
class GLCharBuffer;
class TCFont;
struct TSignalData;

class TFontManager {
public:
	~TFontManager();

	// Confirmed called with a list of font objects fetched from
	// TVisionaire::GetList (TGameControl::InitFonts, asm lines 458293-458322)
	// - not reversed beyond that call shape.
	void Initialize(TVList &fonts);

	// Confirmed (asm lines 1389069-1389122): looks a font up by its
	// TVisObjRef's packed id, returning null on a miss.
	TCFont *GetFont(const TVisObjRef &font) const;
	// Confirmed (asm lines 1389130-1389148): null when no font is selected.
	TCFont *GetCurrentFont() const {
		return _currentFont;
	}
	void SetCurrentFont(const TVisObjRef &font);
	// Confirmed (asm lines 1389221-1389266): a second overload that selects
	// by position instead, clamping/wrapping the given index into
	// [0, GetFontCount()) and writing the clamped value back through the
	// reference - used for cycling through fonts (e.g. a debug console).
	void SetCurrentFont(int &index);

	// All four confirmed (asm lines 1389274-1389459) to no-op (or return a
	// zeroed/default out-param) when no font is selected, otherwise delegate
	// directly to the current TCFont.
	void PrintText(const wxString &text, TextAlignmentEnum alignment, const wxPoint &pos, float scale,
	               GLCharBuffer *buffer = nullptr);
	void PrintTextLines(const std::list<wxString> &lines, const std::vector<int> &lineWidths,
	                    TextAlignmentEnum alignment, const wxPoint &pos, float scale, bool wrap,
	                    std::vector<GLCharBuffer *> *buffers);
	void GetTextDimension(const wxString &text, wxPoint &outSize) const;
	void GetTextDimension(const std::list<wxString> &lines, wxPoint &outSize) const;
	int GetLineHeight() const;
	void PerformAutoLineBreak(const wxString &text, std::list<wxString> &outLines);

	/** All the fonts (ShowFrame asks each one every frame whether its Freetype font has to be made again). */
	const std::vector<TCFont *> &GetFonts() const {
		return _fonts;
	}

	// Confirmed (asm lines 1389422-1389433).
	int GetFontCount() const {
		return static_cast<int>(_fonts.size());
	}

	// Confirmed (asm lines 1389467-1389654): re-splits every string in
	// `texts` that doesn't already fit within `maxWidth`, replacing a single
	// entry with however many lines TCFont::SplitIntoLines() produces for it
	// (an entry that already fits - 0 or 1 resulting lines - is left alone).
	void SplitTexts(std::list<wxString> &texts, int maxWidth);

	// Confirmed (asm lines 1389662-1390023): handles kSignalPrintText/
	// kSignalPrintTextLines (see TSignalData.h) - not reversed beyond that
	// (the x_assert() default case for any other signal type is skipped
	// rather than reproduced, matching this project's usual treatment of
	// "impossible per the caller" assertions).
	void Signal(const TSignalData &signal, TSignalData &result);

private:
	std::vector<TCFont *> _fonts;
	std::unordered_map<int, TCFont *> _fontsById;
	TCFont *_currentFont = nullptr;
};

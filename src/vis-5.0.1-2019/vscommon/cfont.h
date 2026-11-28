// Reconstructed from Deponia_Linux.asm, TCFont (asm 1391446-1398227); the path of the original, from its x_assert():
// src/vscommon/cfont.cpp. A font of the game, owned by TFontManager. It is one of three things:
//  - a font that stands for another (the field kFontFont links to it): everything is passed on to that font (`_base`);
//  - a TrueType font (kFontTrueTypeFont): Freetype makes the glyphs (`_freetype`); it makes the font again when the font
//    object changes (OnEvent);
//  - a font of a picture: kFontLetters are the places of the letters in the picture kFontSprite, kFontAlphabet the
//    characters they are, kFontKerning pairs of characters with a closer or wider spacing.
//
// PrintText() and PrintTextLines() draw: the letters of a picture font one by one through TPictureIO::DrawWithSrcRect (between
// BeginBatch() and EndBatch() of the graphics), a TrueType font through TFreetypeFont::RenderString and GLCharBuffer - both
// with the text matrix and the scroll of the paint control. EnsureSpriteLoaded() makes the picture of a font into an atlas
// of the letters with their transparent edges taken off. Not reconstructed: TFreetypeFont (see graphicslib/freetypeFont.h).
#pragma once

#include <list>
#include <map>
#include <vector>

#include "WxStub.h"
#include "datastruct/eventhandler.h"
#include "datastruct/visobjref.h"
#include "graphicslib/freetypeFont.h"
#include "graphicslib/picture.h"
#include "vscommon/fontManager.h"

class TFontManager;

// Confirmed call shape only (TGText, Deponia_Linux.asm lines 213821-213827): the glyph buffer
// of a printed text, kept by the text that printed it and deleted by it. Not reversed.
class GLCharBuffer {
public:
	~GLCharBuffer();
};

/** One letter of a font of a picture (0x18 bytes in the map of the font). */
struct TLetter {
	wchar_t _char = 0;          // +0x00
	wxRect _rect;               // +0x04, where the letter is in the picture of the font (after EnsureSpriteLoaded(): in the atlas)
	signed char _advance = 0;   // +0x14, how far the next letter is (the width of the letter)
	signed char _offsetX = 0;   // +0x15, how many columns EnsureSpriteLoaded() took off at the left of the letter
	signed char _offsetY = 0;   // +0x16, how many lines it took off at the top
};

class TCFont : public TEventHandlerInterface {
public:
	/** Set while a text is printed that is not tied to a character (TGText::Draw()): the text
	 *  is zoomed with the scene. */
	static bool ZoomText;

	TCFont(TVisObjRef &font, TFontManager *manager);
	virtual ~TCFont();

	/** Slot 0x00: the font object changed: the font that this one stands for is read again, and a TrueType font is made again. */
	void OnEvent(TEventEnum event, int field, TVisionaireObject *object) override;

	/** The font object that this font was made from. */
	const TVisObjRef &GetFontObject() const {
		return _font;
	}

	/** Called every frame by ShowFrame (asm 497798): makes the Freetype font again when it has to be. */
	void CheckFreetypeFont();

	/** How far the letter `c` moves the next one (the first letter of the font when there is none for `c`). */
	int GetCharWidth(wchar_t c) const;
	/** The spacing between the letter `previous` and `current`: the font's letter spacing, or minus the value of the kerning
	 *  of the pair; a kerning that has `previous` alone takes the place of both. */
	int GetCharSpacing(wchar_t current, wchar_t previous) const;
	/** The height of a line with the spacing between lines. */
	int GetLineHeight() const;
	TLetter GetLetter(wchar_t c) const;
	/** The x where a text of the width `textWidth` starts if `x` is the point it is aligned to (see TextAlignmentEnum;
	 *  `boxWidth` is the width of the box for the alignments 3 to 5). */
	int GetTextStartPos(TextAlignmentEnum alignment, int x, int textWidth, int boxWidth);

	void GetTextDimension(const wxString &text, wxPoint &outSize) const;
	void GetTextDimension(const std::list<wxString> &lines, wxPoint &outSize) const;
	/** Cuts `text` into lines of at most `maxWidth` (-1: only where the text has a new line), at the last space before. */
	void SplitIntoLines(const wxString &text, std::list<wxString> &outLines, int maxWidth);
	/** SplitIntoLines() with the width of the font's line (kFontLineWidth) when the font breaks lines (kFontAutoLineBreak). */
	void PerformAutoLineBreak(const wxString &text, std::list<wxString> &outLines);
	/** Draws `text` with `pos` as the point it is aligned to; `alpha` how opaque; `color` -1: the colour the font has. A font
	 *  that stands for another passes it on, with its own colour. */
	void PrintText(const wxString &text, TextAlignmentEnum alignment, const wxPoint &pos, float alpha, int color,
	               GLCharBuffer *buffer);
	/** Draws the lines (`lineWidths` are their widths, from GetTextDimension()), one below the other. With `wrap` (and the
	 *  text matrix on) the text is moved inside the screen. */
	void PrintTextLines(const std::list<wxString> &lines, const std::vector<int> &lineWidths,
	                    TextAlignmentEnum alignment, const wxPoint &pos, float alpha, int color, bool wrap,
	                    std::vector<GLCharBuffer *> *buffers);
	/** Loads the picture of a font of a picture and puts its letters, with the transparent edges taken off, into one
	 *  picture 512 pixels wide (an atlas); the letters get their place in it. Done once. */
	void EnsureSpriteLoaded();

private:
	/** The font object has TrueType font data: makes `_freetype` from it. */
	void createFreetypeFont(const TVisObjRef &font);
	/** Reads the letters, their spacing and the kerning of a font of a picture. */
	void loadPictureFont(const TVisObjRef &font);
	void loadKerning(const TVisObjRef &font);
	/** The spacing between two letters of a font of a picture (as SplitIntoLines() and GetTextDimension() have it: the
	 *  pair first, then the previous letter alone). */
	int spacingBetween(wchar_t current, wchar_t previous) const;
	/** The width of `c` in a font of a picture. */
	int letterAdvance(wchar_t c) const;

	TVisObjRef _font;                    // +0x08, the font (a copy of the one that the constructor was given)
	TFreetypeFont *_freetype;            // +0x10, the TrueType font (null for a font of a picture)
	TVisObjRef _base;                    // +0x18, the font that this one stands for (empty: it is its own)
	TVisObjRef _fontCopy;                // +0x20, the same object as _font (what OnEvent reads)
	TPictureIO _picture;                 // +0x28, the picture of the font
	std::map<wchar_t, TLetter> _letters; // +0x110
	std::vector<wxRect> _letterRects;    // +0x140, the rectangles of the letters in the picture
	std::vector<wxRect> _atlasRects;     // +0x158, the same as the font object has them (EnsureSpriteLoaded() puts the place
	//         of each letter in the texture here)
	mutable int _letterSpacing;          // +0x174 (read again from the font object whenever a text is measured)
	int _lineHeight;                     // +0x178, the height of the first letter
	int _atlasHeight = 0;                // +0x170
	std::map<int, int> _kerning;         // +0x180, (current << 16) + previous, or the previous letter alone, to the value
	TFontManager *_manager;              // +0x1B0
};

// Confirmed call shapes only (TCFont, Deponia_Linux.asm lines 1391446-1398227; the class itself is asm 1582405-1589320,
// 19 methods, none reconstructed): a TrueType font made with Freetype (and drawn through GL glyph buffers). TCFont needs
// of it what is declared here: to make it from the font object, the width of a character and of a string, the height
// of a line, and the two numbers TCFont sets in it. Nothing of the Freetype and GL parts is reconstructed, so
// CreateFont() never succeeds and every measure is 0: a TrueType font of the game has no size yet. (A backend that
// draws TrueType - ScummVM's - gives the metrics here.)
#pragma once

#include <string>

#include "WxStub.h"

class TFreetypeFont {
public:
	/** One glyph of the font (the original keeps them in a HashMap<int, Character>). */
	struct Character {
		int _advance = 0;  // +0x1C, the width of the glyph in pixels
	};

	TFreetypeFont();
	virtual ~TFreetypeFont();

	/** Makes the font from a file: the size in pixels, the colour, an outline (its size and colour) and a shadow (its offset).
	 *  False when the file cannot be made a font. */
	bool CreateFont(const wxString &path, float size, int color, bool border, float borderSize, int borderColor,
	                bool shadow, const wxPoint &shadowOffset);
	/** The glyph for a code point (made when it is not there yet). */
	Character *GetChar(unsigned int code);
	/** The width in pixels of a UTF-8 string. */
	int StringLength(const std::string &utf8);
	/** Called every frame: starts a new glyph texture when the one in use is full. */
	void CheckFillRate();

	int _lineHeight = 0;       // +0xC4
	int _letterSpacing = 0;    // +0xF0
	bool _spacingFlag = false; // +0xF7, set by TCFont together with _letterSpacing

private:
	Character _unknown;
};

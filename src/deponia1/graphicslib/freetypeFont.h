// TFreetypeFont (Deponia_Linux.asm lines 1582405-1589320, 19 methods): a TrueType or OpenType font made with FreeType
// (third_party/freetype, 2.8.0), as TCFont uses it for the fonts of the game that have a font file (Chinese, Korean and
// Arabic). What is as the original has it: the font file is read from the data (and kept for every font that names the
// same file), the face is set to the size in pixels (FT_Set_Char_Size with 64 times the size), `CreateFont()` and its
// fields, the width of a character (`GetChar()`: the advance of the glyph; a space is the size divided by 4.2 plus
// the letter spacing, a tab is two spaces and the spacing, a line break nothing), `StringLength()` (the longest line,
// each character with the letter spacing), the line height (the size) and the border (FT_Stroker, the outside of the
// glyph) and shadow of the glyphs.
// What is not: the original puts the glyphs into textures of the GL backend and draws quads in the batches of the
// shaders (TSpritePartTexture, TGraphicsOGL::BindIndizesBuffer, region, GLCharBuffer); here each glyph is a sprite of the
// graphics backend and is drawn by TGraphicsInterface::Draw(), so every backend can show the text. The original also
// shapes the text with HarfBuzz (itemize(), hb_shape(): the ligatures and the shapes of the letters of Arabic, the order of
// the letters of a right-to-left run); here a glyph is the one the character maps to, in the order of the string
// (TODO: HarfBuzz, third_party/harfbuzz). The font that the original keeps for characters this one has not (+0x110) is
// not done either.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "WxStub.h"

class GLCharBuffer;
class TSpriteHandle;

struct FT_FaceRec_;

class TFreetypeFont {
public:
	/** One glyph of the font (the original keeps them in a HashMap<int, Character>, by the index of the glyph). */
	struct Character {
		int _advance = 0;       // the width of the glyph in pixels (+0x1C in the original)
		int _bearingX = 0;      // how far the bitmap is from the pen, to the right
		int _bearingY = 0;      // how far the top of the bitmap is above the baseline
		int _width = 0;         // the bitmap of the glyph
		int _height = 0;
		TSpriteHandle *_sprite = nullptr;  // the glyph as white with the coverage as alpha (null: nothing to draw)
		int _borderLeft = 0;    // the bitmap of the outline: its place relative to the glyph
		int _borderTop = 0;
		int _borderWidth = 0;
		int _borderHeight = 0;
		TSpriteHandle *_borderSprite = nullptr;
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
	/** Called every frame: starts a new glyph texture when the one in use is full (the sprites of the glyphs have no such limit). */
	void CheckFillRate();

	/** Draws `utf8` at (`x`, `y`), `scale` times as big (the top of the line is `y`). `firstLetter` and `lastLetter` (the
	 *  game passes -1 for both) are the first and the last character to draw. `buffer` and `flag` are for the GL batches. */
	void RenderString(float x, float y, const std::string &utf8, float scale, int firstLetter, int lastLetter,
	                  GLCharBuffer *buffer, bool flag);

	int _lineHeight = 30;      // +0xC4 (the constructor makes it 30; CreateFont() reads the real one)
	float _alpha = 1.0f;       // +0xD0, how opaque the glyphs are drawn (TCFont::PrintText() sets it)
	int _color = 0xFFFFFF;     // +0xE0, the colour of the glyphs
	int _previousColor = 0;    // +0x120, the colour that was used before
	int _letterSpacing = 0;    // +0xF0
	bool _spacingFlag = false; // +0xF7, set by TCFont together with _letterSpacing

	/** The bytes of a font file (kept for every font that names it). */
	struct FontFile;

private:
	/** The glyph of an index of the face, made and kept. */
	Character *glyph(unsigned int index);
	/** The width of a character as the strings count it (the space and the tab are the font's own). */
	int advanceOf(unsigned int code);
	void clear();

	FontFile *_file = nullptr;
	FT_FaceRec_ *_face = nullptr;
	bool _valid = false;       // +0xF4
	int _size = 0;             // +0x108
	bool _border = false;      // +0xD8
	bool _shadow = false;      // +0xD9
	float _borderSize = 0.0f;  // +0xD4
	int _borderColor = 0;      // +0xDC
	int _shadowX = 0;          // +0xE8
	int _shadowY = 0;          // +0xEC
	int _maxBearingY = 0;      // +0xCC, the highest a glyph has been above the baseline: the top of the line is this far above it
	Character _space;          // the space and the tab have no glyph: only an advance
	Character _tab;
	Character _unknown;
	std::map<unsigned int, Character> _glyphs;
};

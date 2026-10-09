#include "vscommon/cfont.h"

#include <algorithm>
#include <cstring>

#include "AppGlobals.h"
#include "Diagnostics.h"
#include "TPaintControl.h"
#include "graphicslib/graphics.h"
#include "graphicslib/maxRectsBinPack.h"
#include "graphicslib/vector3d.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/vscommon/cfont.cpp";

bool TCFont::ZoomText = false;

GLCharBuffer::~GLCharBuffer() {
}

/** The packed id that the log messages of the fonts have. */
static int fontId(const TVisObjRef &font) {
	return PackVisId(font.GetId());
}

/** UTF-8 of a string (the TrueType fonts measure UTF-8, the wide string of wxString is Unicode here). */
static std::string toUtf8(const wxString &text) {
	std::string result;

	for (wchar_t c : text.ToStdWstring()) {
		unsigned int code = static_cast<unsigned int>(c);

		if (code < 0x80) {
			result += static_cast<char>(code);
		} else if (code < 0x800) {
			result += static_cast<char>(0xC0 | (code >> 6));
			result += static_cast<char>(0x80 | (code & 0x3F));
		} else if (code < 0x10000) {
			result += static_cast<char>(0xE0 | (code >> 12));
			result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
			result += static_cast<char>(0x80 | (code & 0x3F));
		} else {
			result += static_cast<char>(0xF0 | (code >> 18));
			result += static_cast<char>(0x80 | ((code >> 12) & 0x3F));
			result += static_cast<char>(0x80 | ((code >> 6) & 0x3F));
			result += static_cast<char>(0x80 | (code & 0x3F));
		}
	}

	return result;
}

/** The string of `length` bytes of UTF-8 at `bytes`. */
static wxString fromUtf8(const char *bytes, int length) {
	std::wstring result;
	const char *end = bytes + (length > 0 ? length : 0);

	while (bytes < end) {
		unsigned char first = static_cast<unsigned char>(*bytes++);
		unsigned int code = first;
		int more = 0;

		if (first >= 0xF0) {
			code = first & 0x07;
			more = 3;
		} else if (first >= 0xE0) {
			code = first & 0x0F;
			more = 2;
		} else if (first >= 0xC0) {
			code = first & 0x1F;
			more = 1;
		}

		for (; more > 0 && bytes < end; more--)
			code = (code << 6) | (static_cast<unsigned char>(*bytes++) & 0x3F);

		result += static_cast<wchar_t>(code);
	}

	return wxString(result);
}

/** The code point at `*position`, which then is the byte after it (the function of the original with this name; the
 *  broken sequences are not looked at). */
static unsigned int decode_utf8_next_char(const char **position, const char *end) {
	const char *p = *position;
	unsigned char first = static_cast<unsigned char>(*p++);
	unsigned int code = first;
	int more = 0;

	if (first >= 0xF0) {
		code = first & 0x07;
		more = 3;
	} else if (first >= 0xE0) {
		code = first & 0x0F;
		more = 2;
	} else if (first >= 0xC0) {
		code = first & 0x1F;
		more = 1;
	}

	for (; more > 0 && p < end; more--)
		code = (code << 6) | (static_cast<unsigned char>(*p++) & 0x3F);

	*position = p;
	return code;
}

// Confirmed (asm lines 1393750-1395566). Which of the three kinds of fonts it is is decided here.
TCFont::TCFont(TVisObjRef &font, TFontManager *manager) : _font(font), _freetype(nullptr), _manager(manager) {
	_fontCopy = font;
	_letterSpacing = _font.GetInt(kFontLetterSpacing);
	_lineHeight = 0;

	TVisObjRef link = font.GetLink(kFontFont);
	bool standsForAnother = !link.IsEmpty();

	if (font.GetBool(kFontTrueTypeFont)) {
		if (standsForAnother) {
			_base = font.GetLink(kFontFont);
		} else {
			font.RegisterEventHandler(this, TEventEnum::kChanged);
			_freetype = new TFreetypeFont();
			createFreetypeFont(font);
		}
	} else {
		if (standsForAnother)
			_base = font.GetLink(kFontFont);
		else
			loadPictureFont(font);
	}
}

// Confirmed (asm lines 1392266-1392320). The handler registered with the font object is not taken off again.
TCFont::~TCFont() {
	delete _freetype;
}

// Confirmed (asm lines 1394322-1394460, 1391446-1391700): the TrueType font is made from the data of the font object
// (the same in the constructor and in OnEvent); when Freetype cannot make it that is logged.
void TCFont::createFreetypeFont(const TVisObjRef &font) {
	const wxPoint *shadowOffset = font.GetPoint(kFontShadowOffset);
	bool shadow = font.GetBool(kFontShadow);
	int borderColor = font.GetInt(kFontBorderColor);
	float borderSize = font.GetFloat(kFontBorderSize);
	bool border = font.GetBool(kFontBorder);
	int color = font.GetInt(kFontColor);
	float size = font.GetFloat(kFontSize);
	wxFileName path = font.GetPath(kFontTrueTypeFontPath);

	if (_freetype->CreateFont(path.GetFullPath(), size, color, border, borderSize, borderColor, shadow, *shadowOffset)) {
		// the glyphs of a few letters are made at once
		_freetype->StringLength("abc");
		_freetype->_spacingFlag = true;
		_freetype->_letterSpacing = font.GetInt(kFontLetterSpacing);
	} else if (wxLog::loglevel >= 0) {
		wxLog::logexpanded(L"Failed to create Freetype font '%s' (id: %d) with font file '%s'",
		                   font.GetName().c_str().wc_str(), fontId(font), path.GetFullPath().wc_str());
	}
}

// Confirmed (asm lines 1391446-1391900): the font object changed: the font that this one stands for is read again; a
// font that stands for none is made again if it is a TrueType one.
void TCFont::OnEvent(TEventEnum /*event*/, int /*field*/, TVisionaireObject */*object*/) {
	_base = _fontCopy.GetLink(kFontFont);

	TVisObjRef link = _fontCopy.GetLink(kFontFont);

	if (!link.IsEmpty())
		return;

	if (!_freetype)
		_freetype = new TFreetypeFont();

	createFreetypeFont(_fontCopy);
}

// Confirmed (asm lines 1391900-1391911)
void TCFont::CheckFreetypeFont() {
	if (_freetype)
		_freetype->CheckFillRate();
}

// Confirmed (asm lines 1393938-1394640 and 1394088-1394105): a font of a picture. The letters are the rectangles of
// kFontLetters, the n-th for the n-th character of kFontAlphabet; each is a TLetter whose advance is the width of its
// rectangle. The space has no rectangle, its advance is kFontSpaceWidth.
void TCFont::loadPictureFont(const TVisObjRef &font) {
	font.GetRects(kFontLetters, _atlasRects);

	wxString alphabet = font.GetStr(kFontAlphabet);

	_picture.Set(font.GetSprite(kFontSprite));
	_letterRects = _atlasRects;

	size_t used = 0;

	for (; used < alphabet.Length() && used < _atlasRects.size(); used++) {
		const wxRect &rect = _atlasRects[used];
		wchar_t c = alphabet.GetChar(used);
		TLetter &letter = _letters[c];

		letter._char = c;
		letter._rect = rect;
		letter._advance = static_cast<signed char>(rect.GetWidth());
		letter._offsetX = 0;
		letter._offsetY = 0;
	}

	if (used < _atlasRects.size() && wxLog::loglevel > 1) {
		wxLog::logexpanded(L"Font \"%s\" (Id: %d): Number of font characters does not match with alphabet. Alphabet has %d "
		                   L"characters, but there are %d characters in the font picture.",
		                   font.GetName().c_str().wc_str(), fontId(font), static_cast<int>(alphabet.Length()),
		                   static_cast<int>(_atlasRects.size()));
	}

	if (_atlasRects.empty()) {
		if (wxLog::loglevel > 1) {
			wxLog::logexpanded(L"Font '%s' (id: %d) does not have a single character.", font.GetName().c_str().wc_str(),
			                   fontId(font));
		}

		_lineHeight = 0;
	} else {
		_lineHeight = _atlasRects[0].GetHeight();
	}

	int spaceWidth = font.GetInt(kFontSpaceWidth);
	TLetter &space = _letters[L' '];

	space._char = L' ';
	space._rect = wxRect();
	space._advance = static_cast<signed char>(spaceWidth);
	space._offsetX = 0;
	space._offsetY = 0;

	loadKerning(font);
}

// Confirmed (asm lines 1394103-1394880): kFontKerning is pairs separated by '|', each `letters=value`: the letters are the
// previous letter and then the current one (the key of the pair is (current << 16) + previous), or the previous letter
// alone. The value is how much closer to put them. One pair that is not like that and all of the kerning is let go.
void TCFont::loadKerning(const TVisObjRef &font) {
	wxString kerning = font.GetStr(kFontKerning);

	if (kerning.IsEmpty())
		return;

	wxStringTokenizer pairs(kerning, L'|');

	while (pairs.HasMoreTokens()) {
		wxString pair = pairs.GetNextToken();
		wxStringTokenizer parts(pair, L'=');

		if (parts.CountTokens() != 2) {
			if (wxLog::loglevel > 0) {
				wxLog::logexpanded(L"Invalid kerning for font '%s' (id: %d). Kerning is ignored.",
				                   font.GetName().c_str().wc_str(), fontId(font));
			}

			_kerning.clear();
			return;
		}

		wxString letters = parts.GetNextToken();
		wxString valueText = parts.GetNextToken();
		long value = -1;
		bool valid = (letters.Len() == 1 || letters.Len() == 2) && valueText.ToLong(&value, 10);

		if (!valid) {
			if (wxLog::loglevel > 0) {
				wxLog::logexpanded(L"Invalid kerning value in font '%s' (id: %d) for '%s'.", font.GetName().c_str().wc_str(),
				                   fontId(font), pair.wc_str());
			}

			continue;
		}

		int key = letters.GetChar(0);

		if (letters.Len() == 2)
			key += letters.GetChar(1) << 16;

		_kerning[key] = static_cast<int>(value);
	}
}

// The width of the letter in a font of a picture: that of the letter, else that of the first one (the lowest code).
int TCFont::letterAdvance(wchar_t c) const {
	auto it = _letters.find(c);

	if (it != _letters.end())
		return it->second._advance;

	return _letters.empty() ? 0 : _letters.begin()->second._advance;
}

// Confirmed (asm lines 1391924-1391975)
int TCFont::GetCharWidth(wchar_t c) const {
	return letterAdvance(c);
}

// Confirmed (asm lines 1391975-1392071): the spacing of the font, or minus the kerning of the pair; and the kerning of the
// previous letter alone is looked up after it and takes its place.
int TCFont::GetCharSpacing(wchar_t current, wchar_t previous) const {
	int spacing = _letterSpacing;

	if (_kerning.empty())
		return spacing;

	auto pair = _kerning.find((static_cast<int>(current) << 16) + static_cast<int>(previous));

	if (pair != _kerning.end())
		spacing = -pair->second;

	auto single = _kerning.find(static_cast<int>(previous));

	if (single != _kerning.end())
		spacing = -single->second;

	return spacing;
}

// The spacing in SplitIntoLines() and GetTextDimension(): the pair, and only when there is none the previous letter alone.
int TCFont::spacingBetween(wchar_t current, wchar_t previous) const {
	int spacing = _letterSpacing;

	if (_kerning.empty())
		return spacing;

	auto pair = _kerning.find((static_cast<int>(current) << 16) + static_cast<int>(previous));

	if (pair != _kerning.end())
		return -pair->second;

	auto single = _kerning.find(static_cast<int>(previous));

	if (single != _kerning.end())
		return -single->second;

	return spacing;
}

// Confirmed (asm lines 1392071-1392134): the height of the font that is the end of the chain of fonts that stand for
// another, and the spacing between lines of the font object.
int TCFont::GetLineHeight() const {
	const TCFont *font = this;

	while (!font->_base.IsEmpty()) {
		font = font->_manager->GetFont(font->_base);

		if (!font)
			return 0;
	}

	int height = font->_freetype ? font->_freetype->_lineHeight : font->_lineHeight;

	return height + font->_font.GetInt(kVerticalLetterSpacing);
}

// Confirmed (asm lines 1392134-1392196)
TLetter TCFont::GetLetter(wchar_t c) const {
	auto it = _letters.find(c);

	if (it != _letters.end())
		return it->second;

	return _letters.empty() ? TLetter() : _letters.begin()->second;
}

// Confirmed (asm lines 1392196-1392266)
int TCFont::GetTextStartPos(TextAlignmentEnum alignment, int x, int textWidth, int boxWidth) {
	switch (alignment) {
	case TextAlignmentEnum::kLeft:
		return x;
	case TextAlignmentEnum::kRight:
		return x - textWidth;
	case TextAlignmentEnum::kCenter:
		return x - (textWidth >> 1);
	case TextAlignmentEnum::kBoxLeft:
		return x - (boxWidth >> 1);
	case TextAlignmentEnum::kBoxRight:
		return x + (boxWidth >> 1) - textWidth;
	case TextAlignmentEnum::kBoxCenter:
		return x + ((boxWidth - textWidth) >> 1);
	default:
		x_assert(false, "false", kSourceFile, 0x2AB);
		return 0;
	}
}

// Confirmed (asm lines 1392370-1393270). A font that stands for another passes the text on; a TrueType font measures the
// UTF-8 of the text by what Freetype says (its quirks kept: after a break at a space, the letters that follow the space
// have no width, and a line after a new line begins with the new line character); a font of a picture adds up the width of
// its letters and the spacing between them. A line is over at a new line (a CR or LF, together with one more of them) or
// when it is `maxWidth` wide or more, at the last space - if there was one.
void TCFont::SplitIntoLines(const wxString &text, std::list<wxString> &outLines, int maxWidth) {
	if (!_base.IsEmpty()) {
		TCFont *font = _manager->GetFont(_base);

		if (font)
			font->SplitIntoLines(text, outLines, maxWidth);

		return;
	}

	if (_freetype) {
		std::string utf8 = toUtf8(text);
		const char *start = utf8.data();
		const char *end = start + utf8.size();
		const char *position = start;
		int lineStart = 0;
		int lastSpace = -1;
		int width = 0;

		while (position != end) {
			unsigned int code = decode_utf8_next_char(&position, end);

			width += _freetype->GetChar(code)->_advance + _freetype->_letterSpacing;

			if (maxWidth != -1 && width >= maxWidth && lastSpace != -1) {
				outLines.push_back(fromUtf8(start + lineStart, lastSpace - lineStart));
				lineStart = lastSpace + 1;
				width = 0;
				lastSpace = -1;
				continue;
			}

			int index = static_cast<int>(position - start) - 1;

			if (code == 0x0D || code == 0x0A) {
				outLines.push_back(fromUtf8(start + lineStart, index - lineStart));

				if (position != end && (*position == 0x0D || *position == 0x0A))
					position++;

				lineStart = index;
				width = 0;
				lastSpace = -1;
			} else if (code == 0x20) {
				lastSpace = index;
			}
		}

		outLines.push_back(fromUtf8(start + lineStart, static_cast<int>(end - start) - lineStart));
		return;
	}

	_letterSpacing = _font.GetInt(kFontLetterSpacing);

	int length = static_cast<int>(text.Length());

	if (length <= 0) {
		outLines.push_back(text.Mid(0, -1));
		return;
	}

	bool wrap = (maxWidth != -1);
	int lineStart = 0;
	int lastSpace = -1;
	int index = 0;
	int width = 0;
	wchar_t previous = 0;
	wchar_t current = text.GetChar(0);

	for (;;) {
		bool checkEnd = true;

		width += letterAdvance(current);

		if (width >= maxWidth && wrap && lastSpace != -1) {
			outLines.push_back(text.Mid(lineStart, lastSpace - lineStart));
			previous = 0;
			width = 0;
			lineStart = lastSpace + 1;
			index = lineStart;
			lastSpace = -1;
		} else if (current == 0x0D || current == 0x0A) {
			outLines.push_back(text.Mid(lineStart, index - lineStart));

			int after = index + 1;

			if (length <= after) {
				outLines.push_back(text.Mid(after, -1));
				return;
			}

			wchar_t next = text.GetChar(after);

			width = 0;
			lastSpace = -1;

			if (next == 0x0D || next == 0x0A) {
				previous = 0;
				index = after + 1;
				lineStart = index;
			} else {
				index = after;
				lineStart = after;
				current = next;
				checkEnd = false;
			}
		} else if (current == 0x20) {
			lastSpace = index;
			previous = 0x20;
			index++;
		} else {
			previous = current;
			index++;
		}

		if (!checkEnd)
			continue;

		if (length <= index) {
			outLines.push_back(text.Mid(lineStart, -1));
			return;
		}

		current = text.GetChar(index);

		if (previous != 0)
			width += spacingBetween(current, previous);
	}
}

// Confirmed (asm lines 1393270-1393319)
void TCFont::PerformAutoLineBreak(const wxString &text, std::list<wxString> &outLines) {
	int maxWidth = -1;

	if (_font.GetBool(kFontAutoLineBreak))
		maxWidth = _font.GetInt(kFontLineWidth);

	SplitIntoLines(text, outLines, maxWidth);
}

// Confirmed (asm lines 1393319-1393677): the size of a text: its widest line, and a line height for each line; a CR LF
// or LF CR is one new line (but not two LFs).
void TCFont::GetTextDimension(const wxString &text, wxPoint &outSize) const {
	if (!_base.IsEmpty()) {
		TCFont *font = _manager->GetFont(_base);

		if (font)
			font->GetTextDimension(text, outSize);

		return;
	}

	if (_freetype) {
		outSize.x = _freetype->StringLength(toUtf8(text));
		outSize.y = _freetype->_lineHeight + _font.GetInt(kVerticalLetterSpacing);
		return;
	}

	_letterSpacing = _font.GetInt(kFontLetterSpacing);
	outSize.y = _lineHeight;

	if (text.IsEmpty()) {
		outSize.x = 0;
		return;
	}

	int widest = 0;
	int width = 0;
	wchar_t previous = 0;

	for (size_t i = 0; i < text.Length(); i++) {
		wchar_t c = text.GetChar(i);

		if (c == 0x0A || c == 0x0D) {
			wchar_t other = (c == 0x0A) ? 0x0D : 0x0A;

			if (text.GetChar(i + 1) == other)
				i++;

			if (width > widest)
				widest = width;

			previous = 0;
			outSize.y += _lineHeight + _font.GetInt(kVerticalLetterSpacing);
			width = 0;
			continue;
		}

		if (previous != 0)
			width += spacingBetween(c, previous);

		width += letterAdvance(c);
		previous = c;
	}

	outSize.x = (width < widest) ? widest : width;
}

// Confirmed (asm lines 1393677-1393750)
void TCFont::GetTextDimension(const std::list<wxString> &lines, wxPoint &outSize) const {
	outSize.x = 0;
	outSize.y = 0;

	for (auto it = lines.begin(); it != lines.end(); ++it) {
		if (it != lines.begin())
			outSize.y += _font.GetInt(kVerticalLetterSpacing);

		wxPoint size;

		GetTextDimension(*it, size);

		if (size.x > outSize.x)
			outSize.x = size.x;

		outSize.y += size.y;
	}
}

// Confirmed (asm lines 1396036-1397049). A font that stands for another prints with its own colour. A TrueType font
// hands the whole text to Freetype. A font of a picture draws the letters one by one: the position of the next letter is
// moved by the width of the letter (a signed byte) and the spacing (of the pair of letters, else the letter spacing of the
// font), a letter is drawn `offset` further in because its transparent edges were taken off, a new line (CR, LF, or the two
// of them in any order) begins at the position for the next line of the text (see GetTextStartPos()) and `lineHeight +
// vertical letter spacing` lower. With the text matrix on (and the matrices active) the position goes through it first.
// NOTE: the widths of the lines are worked out before for the alignments other than left; a new line that is only a CR or
// only a LF is not seen there but is one when the text is drawn (the original); and the cursor in the widths moves by two at
// every new line (also the original), so the third line of a right or centred text is put at the left (x_assert).
void TCFont::PrintText(const wxString &text, TextAlignmentEnum alignment, const wxPoint &pos, float alpha, int color,
                       GLCharBuffer *buffer) {
	if (!_base.IsEmpty()) {
		TCFont *font = _manager->GetFont(_base);

		if (font)
			font->PrintText(text, alignment, pos, alpha, _font.GetInt(kFontColor), buffer);

		return;
	}

	float posX = static_cast<float>(pos.x);
	float posY = static_cast<float>(pos.y);

	if (textMatrix.size() == 9 && matricesActive) {
		const wxPoint &scroll = TPictureIO::s_pPaintControl->GetScrollPos();
		idMat3 matrix;
		idVec3 vector;

		for (int i = 0; i < 9; i++)
			matrix._m[i] = textMatrix[i];

		vector.x = static_cast<float>(pos.x - scroll.x);
		vector.y = static_cast<float>(pos.y - scroll.y);
		vector.z = 1.0f;

		idVec3 moved = matrix * vector;

		posX = static_cast<float>(scroll.x) + moved.x;
		posY = static_cast<float>(scroll.y) + moved.y;
	}

	if (_freetype) {
		if (color != -1) {
			_freetype->_previousColor = _freetype->_color;
			_freetype->_color = color;
		}

		_freetype->_alpha = alpha;

		const wxPoint &scroll = TPictureIO::s_pPaintControl->GetScrollPos();

		posX -= static_cast<float>(scroll.x);
		posY -= static_cast<float>(scroll.y);
		_freetype->RenderString(posX, posY, toUtf8(text), 1.0f, -1, -1, buffer, false);

		// (the colour of the font is the one to use next, and the one of this text the one before)
		_freetype->_previousColor = _freetype->_color;
		_freetype->_color = _font.GetInt(kFontColor);
		return;
	}

	EnsureSpriteLoaded();
	_letterSpacing = _font.GetInt(kFontLetterSpacing);

	std::vector<int> lineWidths;
	wxPoint dimension;
	int startX;

	if (alignment == TextAlignmentEnum::kLeft) {
		startX = static_cast<int>(posX);
		posX = static_cast<float>(startX);
	} else {
		GetTextDimension(text, dimension);

		// the width of each line
		size_t lineStart = 0;
		size_t i = 0;

		for (; i < text.Length(); i++) {
			wchar_t c = text.GetChar(i);
			size_t lineEnd = i;
			bool pair = false;

			if (c == 0x0D)
				pair = (text.GetChar(i + 1) == 0x0A);
			else if (c == 0x0A)
				pair = (text.GetChar(i + 1) == 0x0D);

			if (!pair)
				continue;

			wxPoint size;

			GetTextDimension(text.Mid(static_cast<int>(lineStart), static_cast<int>(lineEnd - lineStart)), size);
			lineWidths.push_back(size.x);
			lineStart = i + 2;
			i++;
		}

		if (lineStart < i) {
			wxPoint size;

			GetTextDimension(text.Mid(static_cast<int>(lineStart), -1), size);
			lineWidths.push_back(size.x);
		}

		startX = static_cast<int>(posX);
		posX = static_cast<float>(GetTextStartPos(alignment, startX, lineWidths.empty() ? 0 : lineWidths[0], dimension.x));
	}

	size_t cursor = 0;
	wchar_t previous = 0;

	graphics->BeginBatch();

	for (size_t i = 0; i < text.Length(); i++) {
		wchar_t c = text.GetChar(i);

		if (c == 0x0A || c == 0x0D) {
			// a new line; the two characters of a pair are one
			wchar_t other = (c == 0x0A) ? 0x0D : 0x0A;

			if (text.GetChar(i + 1) == other)
				i++;

			int width = 0;

			if (cursor == lineWidths.size()) {
				x_assert(alignment == TextAlignmentEnum::kLeft, "eAlignment == eAlignLeft", kSourceFile, 0x159);
			} else if (cursor + 1 == lineWidths.size()) {
				cursor = lineWidths.size();
				x_assert(alignment == TextAlignmentEnum::kLeft, "eAlignment == eAlignLeft", kSourceFile, 0x159);
			} else {
				width = lineWidths[cursor + 1];
				cursor += 2;
			}

			posY += static_cast<float>(_lineHeight + _font.GetInt(kVerticalLetterSpacing));
			posX = static_cast<float>(GetTextStartPos(alignment, startX, width, dimension.x));
			previous = 0;
			continue;
		}

		TLetter letter = GetLetter(c);

		if (previous != 0)
			posX += static_cast<float>(spacingBetween(c, previous));

		if (letter._char != L' ') {
			wxPoint where;

			where.x = static_cast<int>(posX + static_cast<float>(letter._offsetX));
			where.y = static_cast<int>(posY + static_cast<float>(letter._offsetY));
			_picture.SetPosition(where, -1.0f);
			_picture.DrawWithSrcRect(letter._rect, alpha, static_cast<unsigned int>(color));
		}

		posX += static_cast<float>(letter._advance);
		previous = c;
	}

	graphics->EndBatch();
}

// Confirmed (asm lines 1397049-1398227). As PrintText() for the lines of a text (their widths are given). With the text
// matrix on, the position goes through it and, if `wrap`, is moved so that the text stays inside the screen; with ZoomText
// the sizes are the first number of the matrix times as big (for a TrueType font; a picture font does not zoom). The
// letters of a picture font are put at whole pixels. A TrueType font gets each line from Freetype, aligned by the widths
// times the zoom.
// NOTE: the moving inside the screen of a right-aligned text puts its right edge at the left of the screen
// (scroll - width) when it would stick out there, where the other alignments use scroll + width (the original).
void TCFont::PrintTextLines(const std::list<wxString> &lines, const std::vector<int> &lineWidths,
                            TextAlignmentEnum alignment, const wxPoint &pos, float alpha, int color, bool wrap,
                            std::vector<GLCharBuffer *> *buffers) {
	if (!_base.IsEmpty()) {
		TCFont *font = _manager->GetFont(_base);

		if (font)
			font->PrintTextLines(lines, lineWidths, alignment, pos, alpha, _font.GetInt(kFontColor), wrap, nullptr);

		return;
	}

	float posX = static_cast<float>(pos.x);
	float posY = static_cast<float>(pos.y);
	float zoom = 1.0f;

	if (textMatrix.size() == 9 && matricesActive) {
		const wxPoint scroll = TPictureIO::s_pPaintControl->GetScrollPos();
		idMat3 matrix;
		idVec3 vector;

		for (int i = 0; i < 9; i++)
			matrix._m[i] = textMatrix[i];

		vector.x = static_cast<float>(pos.x - scroll.x);
		vector.y = static_cast<float>(pos.y - scroll.y);
		vector.z = zoom;

		idVec3 moved = matrix * vector;
		float scrollX = static_cast<float>(scroll.x);
		float scrollY = static_cast<float>(scroll.y);

		posX = scrollX + moved.x;
		posY = scrollY + moved.y;

		if (ZoomText)
			zoom = textMatrix[0];

		if (wrap) {
			for (size_t i = 0; i < lineWidths.size(); i++) {
				float width = static_cast<float>(lineWidths[i]);
				float right = static_cast<float>(graphics->GetWidth() + scroll.x);

				switch (alignment) {
				case TextAlignmentEnum::kLeft: {
					float scaled = width * zoom;

					if (posX + scaled > right)
						posX = right - scaled;

					posX = std::max(scrollX, posX);
					break;
				}
				case TextAlignmentEnum::kCenter: {
					float half = width * zoom * 0.5f;

					if (posX + half > right)
						posX = right - half;

					if (scrollX > posX - half)
						posX = half + scrollX;

					break;
				}
				case TextAlignmentEnum::kRight: {
					float scaled = width * zoom;

					if (posX > right)
						posX = right;

					if (scrollX > posX - scaled)
						posX = scrollX - scaled;

					break;
				}
				default:
					break;
				}

				// (not above the screen, and not so low that a line would be under it)
				posY = std::max(scrollY, posY);

				float bottom = static_cast<float>(graphics->GetHeight() + scroll.y);

				if (posY > bottom) {
					TCFont *font = this;
					int lineHeight = 0;

					while (font) {
						if (font->_base.IsEmpty()) {
							int height = font->_freetype ? font->_freetype->_lineHeight : font->_lineHeight;

							lineHeight = height + font->_font.GetInt(kVerticalLetterSpacing);
							break;
						}

						font = font->_manager->GetFont(font->_base);
					}

					posY = static_cast<float>(graphics->GetHeight() + scroll.y - lineHeight);
				}
			}
		}
	}

	if (_freetype) {
		const wxPoint &origin = TPictureIO::s_pPaintControl->GetOrigin();
		float x = static_cast<float>(origin.x) + posX;
		float y = static_cast<float>(origin.y) + posY;

		if (color != -1) {
			_freetype->_previousColor = _freetype->_color;
			_freetype->_color = color;
		}

		_freetype->_alpha = alpha;

		// the widest line (times the zoom)
		int widest = 0;

		for (size_t i = 0; i < lineWidths.size(); i++) {
			float scaled = static_cast<float>(lineWidths[i]) * zoom;

			if (scaled > static_cast<float>(widest))
				widest = static_cast<int>(scaled);
		}

		const wxPoint scroll = TPictureIO::s_pPaintControl->GetScrollPos();

		y -= static_cast<float>(scroll.y);

		if (!lines.empty()) {
			int startX = static_cast<int>(x);
			size_t index = 0;

			for (auto line = lines.begin(); line != lines.end(); ++line, index++) {
				GLCharBuffer *buffer = nullptr;

				if (buffers) {
					if (buffers->size() <= index)
						buffers->push_back(new GLCharBuffer());

					buffer = (*buffers)[index];
				}

				int width = static_cast<int>(static_cast<float>(lineWidths[index]) * zoom);

				if (alignment == TextAlignmentEnum::kLeft)
					width = lineWidths[index];

				int lineX = GetTextStartPos(alignment, startX, width, widest);

				_freetype->RenderString(static_cast<float>(lineX - scroll.x), y, toUtf8(*line), zoom, -1, -1, buffer, true);
				y += static_cast<float>(_freetype->_lineHeight + _font.GetInt(kVerticalLetterSpacing));
			}
		}

		_freetype->_previousColor = _freetype->_color;
		_freetype->_color = _font.GetInt(kFontColor);
		return;
	}

	EnsureSpriteLoaded();
	_letterSpacing = _font.GetInt(kFontLetterSpacing);

	int widest = 0;

	for (int width : lineWidths)
		widest = std::max(widest, width);

	graphics->BeginBatch();

	if (!lineWidths.empty()) {
		int startX = static_cast<int>(posX);
		int y = static_cast<int>(posY);
		auto line = lines.begin();

		for (size_t index = 0; index < lineWidths.size() && line != lines.end(); index++, ++line) {
			int x = GetTextStartPos(alignment, startX, lineWidths[index], widest);
			wchar_t previous = 0;

			for (size_t i = 0; i < line->Length(); i++) {
				wchar_t c = line->GetChar(i);
				TLetter letter = GetLetter(c);

				if (previous != 0)
					x += spacingBetween(c, previous);

				if (letter._char != L' ') {
					wxPoint where;

					where.x = x + letter._offsetX;
					where.y = y + letter._offsetY;
					_picture.SetPosition(where, -1.0f);
					_picture.DrawWithSrcRect(letter._rect, alpha, static_cast<unsigned int>(color));
				}

				x += letter._advance;
				previous = c;
			}

			y += _lineHeight + _font.GetInt(kVerticalLetterSpacing);
		}
	}

	graphics->EndBatch();
}

// Confirmed (asm lines 1395566-1396036). The picture of the font is loaded and each letter that the alphabet names is
// narrowed to what has something in it (RemoveTransparentEdges: the letter's rectangle in the font object's list is
// replaced by the narrowed one, its `_offsetX/Y` are what was taken off at the left and the top); the letters are put
// into an atlas 512 pixels wide with the bottom-left rule and the picture of the font becomes the atlas. A letter's
// `_rect` is then where it is in the atlas. Done once, while the picture has no sprite.
// NOTE: the original reads the pixels of the picture without looking; nothing is done here if there are none (the
// decoders of the pictures are not reconstructed). The picture owns the atlas afterwards (the original frees it itself when
// the picture has a memory block, else the picture does).
void TCFont::EnsureSpriteLoaded() {
	if (_picture.GetSpriteHandle())
		return;

	if (!_picture.LoadPicture(_picture.GetPath(), static_cast<TPictureIO::eLoadSetting>(2)))
		return;

	const char *data = _picture.GetMemoryData();

	if (!data || _picture.GetBytesPerPixel() != 4)
		return;

	wxString alphabet = _font.GetStr(kFontAlphabet);
	int bytesPerPixel = _picture.GetBytesPerPixel();
	MaxRectsBinPack packer;

	packer.Init(512, 4096, false);
	_atlasRects.clear();
	_atlasHeight = 0;

	size_t index = 0;

	for (wxRect &rect : _letterRects) {
		if (alphabet.Length() <= index)
			continue;

		int left = 0;
		int top = 0;
		int width = rect.width;
		int height = rect.height;

		graphics->RemoveTransparentEdges(left, top, width, height, _picture.GetWidth(), rect.y, rect.x, data);
		rect.x += left;
		rect.y += top;
		rect.width = width;
		rect.height = height;

		int bestY = 0;
		int bestX = 0;
		wxRect node = packer.FindPositionForNewNodeBottomLeft(rect.GetWidth(), rect.GetHeight(), bestY, bestX);

		packer.PlaceRect(node);
		_atlasHeight = std::max(_atlasHeight, node.y + node.height);
		_atlasRects.push_back(node);

		wchar_t c = alphabet.GetChar(index);
		auto it = _letters.find(c);

		if (it == _letters.end()) {
			TLetter fresh;

			fresh._char = c;
			it = _letters.insert(std::make_pair(c, fresh)).first;
		}

		it->second._offsetX = static_cast<signed char>(left);
		it->second._offsetY = static_cast<signed char>(top);
		it->second._rect = node;
		index++;
	}

	char *atlas = new char[static_cast<size_t>(_atlasHeight) * 512 * bytesPerPixel]();

	for (size_t i = 0; i < _atlasRects.size(); i++) {
		const wxRect &source = _letterRects[i];
		const wxRect &target = _atlasRects[i];

		for (int row = 0; row < source.height; row++) {
			std::memcpy(atlas + (static_cast<size_t>(target.y + row) * 512 + target.x) * bytesPerPixel,
			            data + (static_cast<size_t>(source.y + row) * _picture.GetWidth() + source.x) * bytesPerPixel,
			            static_cast<size_t>(target.width) * bytesPerPixel);
		}
	}

	bool rgbOrder = _picture.IsRgbOrder();

	_picture.ClearMemData();
	_picture.SetMemoryData(atlas, 512, _atlasHeight, true, rgbOrder, true);
	_picture.CreateSprite(false);
}

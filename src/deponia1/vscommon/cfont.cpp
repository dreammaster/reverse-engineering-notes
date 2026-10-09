#include "vscommon/cfont.h"

#include "Diagnostics.h"
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
		letter._flag1 = false;
		letter._flag2 = false;
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
	space._flag1 = false;
	space._flag2 = false;

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

// Confirmed (asm lines 1396036-1396230 for the font that stands for another). A font that stands for another prints
// with its own colour; the rest is the drawing (the letters of a picture through TPictureIO::DrawWithSrcRect, a TrueType
// font through Freetype and GLCharBuffer, both with the text matrix and the scroll of the paint control) and is not
// reconstructed.
void TCFont::PrintText(const wxString &text, TextAlignmentEnum alignment, const wxPoint &pos, float scale, int /*color*/,
                       GLCharBuffer *buffer) {
	if (!_base.IsEmpty()) {
		TCFont *font = _manager->GetFont(_base);

		if (font)
			font->PrintText(text, alignment, pos, scale, _font.GetInt(kFontColor), buffer);

		return;
	}

	// TODO: asm 1396230-1397049
}

// Confirmed (asm lines 1397049-1397440 for the font that stands for another): as PrintText(); the buffers are not
// passed on.
void TCFont::PrintTextLines(const std::list<wxString> &lines, const std::vector<int> &lineWidths,
                            TextAlignmentEnum alignment, const wxPoint &pos, float scale, int /*color*/, bool wrap,
                            std::vector<GLCharBuffer *> */*buffers*/) {
	if (!_base.IsEmpty()) {
		TCFont *font = _manager->GetFont(_base);

		if (font)
			font->PrintTextLines(lines, lineWidths, alignment, pos, scale, _font.GetInt(kFontColor), wrap, nullptr);

		return;
	}

	// TODO: asm 1397440-1398227
}

// TODO: loads the picture of the font and puts the letters, with their transparent edges taken off, into a texture
// (MaxRectsBinPack, asm 1395566-1396036).
void TCFont::EnsureSpriteLoaded() {
}

#include "graphicslib/freetypeFont.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>
#include <map>

#include "TComposedFileManager.h"
#include "TMemoryBuffer.h"
#include "TSpriteHandle.h"
#include "baselib/file.h"
#include "graphicslib/graphics.h"

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_STROKER_H

// The font files that are in use, by path (the original's font_cache: a file is read once for all the fonts that name it).
struct TFreetypeFont::FontFile {
	std::vector<unsigned char> data;
	int references = 0;
};

static std::map<std::string, TFreetypeFont::FontFile *> s_fontCache;
static FT_Library s_library = nullptr;  // the original's `freetype`

/** The code point at `pos` of a UTF-8 string, and `pos` moved after it (a byte that does not start a sequence is itself). */
static unsigned decodeUtf8(const std::string &text, size_t &pos) {
	unsigned char c = static_cast<unsigned char>(text[pos++]);

	if (c < 0x80)
		return c;

	int more = (c >= 0xF0) ? 3 : (c >= 0xE0) ? 2 : (c >= 0xC0) ? 1 : 0;
	unsigned code = (more == 0) ? c : (c & (0x3F >> more));

	for (int i = 0; i < more; i++) {
		if (pos >= text.size() || (static_cast<unsigned char>(text[pos]) & 0xC0) != 0x80)
			return c;

		code = (code << 6) | (static_cast<unsigned char>(text[pos++]) & 0x3F);
	}

	return code;
}

TFreetypeFont::TFreetypeFont() {
}

TFreetypeFont::~TFreetypeFont() {
	clear();

	if (_face)
		FT_Done_Face(_face);

	if (_file && --_file->references == 0) {
		for (auto it = s_fontCache.begin(); it != s_fontCache.end(); ++it) {
			if (it->second == _file) {
				s_fontCache.erase(it);
				break;
			}
		}

		delete _file;
	}
}

static void releaseSprite(TSpriteHandle *&sprite) {
	if (!sprite)
		return;

	sprite->Release();
	if (sprite->GetRefCount() == 0 && graphics)
		graphics->OnSpriteHandleReleased(sprite);

	sprite = nullptr;
}

// Confirmed (asm lines 1585541-1585671): the glyphs are let go.
void TFreetypeFont::clear() {
	for (auto &entry : _glyphs) {
		releaseSprite(entry.second._sprite);
		releaseSprite(entry.second._borderSprite);
	}

	_glyphs.clear();
	_space = Character();
	_tab = Character();
}

// Confirmed (asm lines 1588913-1589320). The file is read through the composed files of the game (once for every path in use),
// the face is made from the bytes and set to `size` pixels (64 times that is what FT_Set_Char_Size takes); the line height
// is the size and the glyphs made before are let go.
bool TFreetypeFont::CreateFont(const wxString &path, float size, int color, bool border, float borderSize, int borderColor,
                               bool shadow, const wxPoint &shadowOffset) {
	std::string key(static_cast<const char *>(path.mb_str()));

	if (_file && --_file->references == 0) {
		for (auto it = s_fontCache.begin(); it != s_fontCache.end(); ++it) {
			if (it->second == _file) {
				s_fontCache.erase(it);
				break;
			}
		}

		delete _file;
	}

	_file = nullptr;

	auto cached = s_fontCache.find(key);

	if (cached != s_fontCache.end()) {
		_file = cached->second;
		_file->references++;
	} else {
		TFile input;
		TMemoryBuffer buffer;

		if (!TComposedFileManager::Open(input, wxFileName(path.ToStdWstring())))
			return false;

		input.ReadToBuf(buffer, 0);

		if (buffer.GetLen() == 0)
			return false;

		_file = new FontFile();
		_file->references = 1;
		_file->data.assign(buffer.GetData(), buffer.GetData() + buffer.GetLen());
		s_fontCache[key] = _file;
	}

	_maxBearingY = 0;

	if (_face) {
		FT_Done_Face(_face);
		_face = nullptr;
	}

	if (!s_library && FT_Init_FreeType(&s_library) != 0) {
		s_library = nullptr;
		_valid = false;
		return false;
	}

	if (FT_New_Memory_Face(s_library, _file->data.data(), static_cast<FT_Long>(_file->data.size()), 0, &_face) != 0) {
		_face = nullptr;
		_valid = false;
		return false;
	}

	_valid = true;
	clear();

	_borderSize = borderSize;
	_color = color;
	_borderColor = borderColor;
	_border = border;
	_shadow = shadow;
	_shadowX = shadowOffset.x;
	_shadowY = shadowOffset.y;
	_letterSpacing = 0;
	_spacingFlag = false;
	_size = static_cast<int>(size);

	FT_Set_Char_Size(_face, 0, static_cast<FT_F26Dot6>(size * 64.0f), 0, 0);
	_lineHeight = _size;
	return true;
}

/** The pixels of a bitmap of FreeType as a sprite: white, the coverage as the alpha (premultiplied, so the colour is the alpha too). */
static TSpriteHandle *makeSprite(const FT_Bitmap &bitmap) {
	if (!graphics || bitmap.width == 0 || bitmap.rows == 0)
		return nullptr;

	int width = static_cast<int>(bitmap.width);
	int height = static_cast<int>(bitmap.rows);
	std::vector<unsigned char> pixels(static_cast<size_t>(width) * height * 4);

	for (int y = 0; y < height; y++) {
		const unsigned char *row = bitmap.buffer + static_cast<long>(y) * bitmap.pitch;

		for (int x = 0; x < width; x++) {
			unsigned char alpha;

			if (bitmap.pixel_mode == FT_PIXEL_MODE_MONO)
				alpha = (row[x >> 3] & (0x80 >> (x & 7))) ? 255 : 0;
			else
				alpha = row[x];

			unsigned char *out = &pixels[(static_cast<size_t>(y) * width + x) * 4];

			out[0] = out[1] = out[2] = out[3] = alpha;
		}
	}

	TSpriteHandle *sprite = nullptr;

	if (!graphics->CreateSprite(&sprite, reinterpret_cast<const char *>(pixels.data()), width, height, 4, width * 4, true))
		return nullptr;

	return sprite;
}

// Confirmed in the structure (asm lines 1583131-1584092, CreateCharIndex): the glyph is loaded (without a bitmap when there
// is an outline: that is then stroked and drawn as a second bitmap), rendered, its metrics noted (the advance in whole
// pixels, how far the bitmap is from the pen) and the highest it reaches above the baseline kept.
TFreetypeFont::Character *TFreetypeFont::glyph(unsigned int index) {
	auto found = _glyphs.find(index);

	if (found != _glyphs.end())
		return &found->second;

	Character &made = _glyphs[index];

	if (FT_Load_Glyph(_face, index, _border ? FT_LOAD_NO_BITMAP : FT_LOAD_RENDER) != 0)
		return &made;

	FT_GlyphSlot slot = _face->glyph;

	if (_border && slot->format == FT_GLYPH_FORMAT_OUTLINE) {
		FT_Glyph outline = nullptr;
		FT_Stroker stroker = nullptr;

		if (FT_Get_Glyph(slot, &outline) == 0) {
			if (FT_Stroker_New(s_library, &stroker) == 0) {
				FT_Stroker_Set(stroker, static_cast<FT_Fixed>(_borderSize * 64.0f), FT_STROKER_LINECAP_ROUND,
				               FT_STROKER_LINEJOIN_ROUND, 0);

				if (FT_Glyph_Stroke(&outline, stroker, 1) == 0 &&
				        FT_Glyph_To_Bitmap(&outline, FT_RENDER_MODE_NORMAL, nullptr, 1) == 0) {
					FT_BitmapGlyph bitmapGlyph = reinterpret_cast<FT_BitmapGlyph>(outline);

					made._borderLeft = bitmapGlyph->left;
					made._borderTop = bitmapGlyph->top;
					made._borderWidth = static_cast<int>(bitmapGlyph->bitmap.width);
					made._borderHeight = static_cast<int>(bitmapGlyph->bitmap.rows);
					made._borderSprite = makeSprite(bitmapGlyph->bitmap);
				}

				FT_Stroker_Done(stroker);
			}

			FT_Done_Glyph(outline);
		}
	}

	if (_border)
		FT_Render_Glyph(slot, FT_RENDER_MODE_NORMAL);

	made._advance = static_cast<int>(slot->metrics.horiAdvance >> 6);
	made._bearingX = slot->bitmap_left;
	made._bearingY = slot->bitmap_top;
	made._width = static_cast<int>(slot->bitmap.width);
	made._height = static_cast<int>(slot->bitmap.rows);
	made._sprite = makeSprite(slot->bitmap);

	if (made._bearingY > _maxBearingY)
		_maxBearingY = made._bearingY;

	return &made;
}

// Confirmed (asm lines 1584524-1584830). A font that could not be made, and a line break, have no width. The space is the
// size divided by 4.2 and the letter spacing (the first time it is asked for: the spacing is set after the first
// measure), the tab two spaces and the spacing; every other character is the glyph the face has for it.
TFreetypeFont::Character *TFreetypeFont::GetChar(unsigned int code) {
	static Character empty;

	if (!_valid)
		return &empty;

	if (code == 0x0D || code == 0x0A)
		return &empty;

	if (code == 0x20) {
		if (_space._advance == 0)
			_space._advance = static_cast<int>(static_cast<double>(_size) / 4.2 + static_cast<double>(_letterSpacing));

		return &_space;
	}

	if (code == 0x09) {
		if (_tab._advance == 0)
			_tab._advance = GetChar(0x20)->_advance * 2 + _letterSpacing;

		return &_tab;
	}

	return glyph(FT_Get_Char_Index(_face, code));
}

// Confirmed (asm lines 1584830-1584976): the width of the longest line; each character is its width and the letter spacing;
// a line break ends the line (and the spacing is the first thing of the next one, as in the original); a space is counted
// from the size and not from its width.
int TFreetypeFont::StringLength(const std::string &utf8) {
	if (!_valid)
		return 0;

	int widest = 0;
	int line = 0;
	size_t pos = 0;

	while (pos < utf8.size()) {
		unsigned code = decodeUtf8(utf8, pos);
		Character *character = GetChar(code);

		if (code == 0x0A) {
			widest = std::max(widest, line);
			line = 0;
			line += character->_advance + _letterSpacing;
		} else if (code == 0x20) {
			line = static_cast<int>(static_cast<double>(line) + (static_cast<double>(_size) / 4.2 + _letterSpacing));
		} else {
			line += character->_advance + _letterSpacing;
		}
	}

	return std::max(widest, line);
}

// (The glyphs are sprites of their own, there is no texture that fills up.)
void TFreetypeFont::CheckFillRate() {
}

// Confirmed in the structure (asm lines 1586955-1588876, RenderString_i): the characters are placed one after the other from
// the pen, the top of the line is `y` and the baseline `_maxBearingY` below it; a shadow (the glyphs moved by the shadow
// offset, times the scale) is drawn first, then the outline in its colour and then the glyphs. The original draws quads of
// the glyph textures in a batch; here each glyph is drawn by the backend. A line break goes down by the line height.
void TFreetypeFont::RenderString(float x, float y, const std::string &utf8, float scale, int firstLetter, int lastLetter,
                                 GLCharBuffer * /*buffer*/, bool /*flag*/) {
	if (!_valid || !graphics || utf8.empty())
		return;

	struct Placed {
		Character *character;
		float x;
		float y;
	};

	std::vector<Placed> placed;
	float left = std::floor(x);
	float penX = left;
	float penY = std::floor(y);
	size_t pos = 0;
	int index = 0;

	while (pos < utf8.size()) {
		unsigned code = decodeUtf8(utf8, pos);
		Character *character = GetChar(code);

		if (code == 0x0A) {
			penX = left;
			penY += static_cast<float>(_lineHeight) * scale;
			index++;
			continue;
		}

		bool shown = (firstLetter < 0 || index >= firstLetter) && (lastLetter < 0 || index <= lastLetter);

		if (shown && code != 0x20 && code != 0x09 && code != 0x0D)
			placed.push_back({character, penX, penY});

		if (code == 0x20)
			penX += static_cast<float>(static_cast<double>(_size) / 4.2 + _letterSpacing) * scale;
		else
			penX += static_cast<float>(character->_advance + _letterSpacing) * scale;

		index++;
	}

	// (the baseline: `_maxBearingY` can grow while the glyphs above are made, so it is read after the loop)
	auto draw = [&](TSpriteHandle *sprite, int width, int height, float at, float top, unsigned int color) {
		if (!sprite)
			return;

		wxRect source;
		FloatRect dest;
		wxPoint center;

		source.x = 0;
		source.y = 0;
		source.width = width;
		source.height = height;
		dest.x = at;
		dest.y = top;
		dest.width = static_cast<float>(width) * scale;
		dest.height = static_cast<float>(height) * scale;
		center.x = -1;
		center.y = -1;
		graphics->Draw(sprite, source, dest, _alpha, false, color, -1, 0.0f, center, 1.0f, 1.0f, 0);
	};

	const float shadowX = static_cast<float>(_shadowX) * scale;
	const float shadowY = static_cast<float>(_shadowY) * scale;
	const unsigned int fillColor = static_cast<unsigned int>(_color);
	const unsigned int borderColor = static_cast<unsigned int>(_borderColor);

	if (_shadow) {
		for (const Placed &glyphPlaced : placed) {
			const Character *c = glyphPlaced.character;
			float top = glyphPlaced.y + static_cast<float>(_maxBearingY) * scale;

			if (_border && c->_borderSprite) {
				draw(c->_borderSprite, c->_borderWidth, c->_borderHeight, glyphPlaced.x + static_cast<float>(c->_borderLeft) * scale + shadowX,
				     top - static_cast<float>(c->_borderTop) * scale + shadowY, 0xFF000000u);
			} else {
				draw(c->_sprite, c->_width, c->_height, glyphPlaced.x + static_cast<float>(c->_bearingX) * scale + shadowX,
				     top - static_cast<float>(c->_bearingY) * scale + shadowY, 0xFF000000u);
			}
		}
	}

	if (_border) {
		for (const Placed &glyphPlaced : placed) {
			const Character *c = glyphPlaced.character;
			float top = glyphPlaced.y + static_cast<float>(_maxBearingY) * scale;

			draw(c->_borderSprite, c->_borderWidth, c->_borderHeight, glyphPlaced.x + static_cast<float>(c->_borderLeft) * scale,
			     top - static_cast<float>(c->_borderTop) * scale, borderColor);
		}
	}

	for (const Placed &glyphPlaced : placed) {
		const Character *c = glyphPlaced.character;
		float top = glyphPlaced.y + static_cast<float>(_maxBearingY) * scale;

		draw(c->_sprite, c->_width, c->_height, glyphPlaced.x + static_cast<float>(c->_bearingX) * scale,
		     top - static_cast<float>(c->_bearingY) * scale, fillColor);
	}
}

#include "TPictureMEM.h"

#include <algorithm>
#include <cmath>
#include <cstring>

#include "graphicslib/graphics.h"

// Confirmed (asm lines 756895-756925). The sprite part is TSprite's default (no image size).
TPictureMEM::TPictureMEM() {
}

// Confirmed (asm lines 756925-756953): the same, with the sprite copied (not its image size, see TSprite::Set()).
TPictureMEM::TPictureMEM(const TSprite &sprite) : TSprite(sprite) {
	_field70 = false;
}

// Confirmed (asm lines 756823-756850)
TPictureMEM::~TPictureMEM() {
	ClearMemData();
}

// Confirmed (asm lines 756953-756986): a buffer of the picture's own is deleted; one in a memory block stays there.
void TPictureMEM::ClearMemData() {
	if (!_memBlock)
		delete[] _data;

	_data = nullptr;
	_bytesPerPixel = 0;
	_pitch = 0;
}

// Confirmed (asm lines 756986-757003)
void TPictureMEM::SetMemoryBlock(TPictureMemBlock *block) {
	_memBlock = block;
}

// Confirmed (asm lines 757003-757450). The picture is made from TSprite's image size; with a memory block the pixels
// are the first bytes of it, if it is big enough (nothing is reserved in it: the next picture begins at the same place,
// the block is used for one picture at a time).
bool TPictureMEM::InitMemory(bool hasAlpha) {
	ClearMemData();

	int width = GetWidth();
	int height = GetHeight();

	if (height < 0 || width < 0 || height > 100000 || width > 100000) {
		if (wxLog::loglevel > 0)
			wxLog::logexpanded(L"Image size of %s is invalid", GetPath().GetFullPath().wc_str());

		return false;
	}

	_width = width;
	_height = height;
	_bytesPerPixel = hasAlpha ? 4 : 3;
	_pitch = _bytesPerPixel * width;

	int64_t size = static_cast<int64_t>(_pitch) * height;

	if (_memBlock) {
		if (size <= _memBlock->size) {
			_data = _memBlock->data + _memBlock->used;
		} else if (wxLog::loglevel >= 0) {
			wxLog::logexpanded(L"Could not reserve memory (%d KB) for image '%s'. Largest available memory block is %d KB. "
			                   L"Please increase the memory size in the project settings (fields "
			                   L"GamePreloadedPicBufferSize/GamePicBufferSize for max. image size of a preloaded animation or "
			                   L"any other image respectively).",
			                   static_cast<int>(size >> 10), GetPath().GetFullPath().wc_str(),
			                   static_cast<int>(_memBlock->size >> 10));
		}
	} else {
		_data = new uint8_t[static_cast<size_t>(size)];
	}

	graphics->GetPicsMemSettings(_rgbOrder, _flipped);
	return _data != nullptr;
}

// Confirmed (asm lines 757462-757503). The 6th argument of the original (the caller passes 1) is not used.
void TPictureMEM::SetMemoryData(char *data, int width, int height, bool hasAlpha, bool rgbOrder, bool /*flag*/) {
	SetImageSize(width, height);
	_width = width;
	_height = height;
	_data = reinterpret_cast<uint8_t *>(data);
	_rgbOrder = rgbOrder;
	_bytesPerPixel = hasAlpha ? 4 : 3;
	SetTransparency(hasAlpha ? eTransparencyMode::kAlpha : eTransparencyMode::kNone, 0);
	_flipped = hasAlpha;
	_pitch = width * _bytesPerPixel;
}

// Confirmed (asm lines 757520-757546)
int TPictureMEM::GetMemorySize() const {
	return _data ? _pitch * _height : 0;
}

// Confirmed (asm lines 757546-757591)
char *TPictureMEM::GetLine(int y) const {
	if (!_data || y < 0 || y >= _height)
		return nullptr;

	if (_flipped)
		return reinterpret_cast<char *>(_data + (_height - y - 1) * _pitch);

	return reinterpret_cast<char *>(_data + y * _pitch);
}

// Confirmed (asm lines 757591-757692)
unsigned int TPictureMEM::GetPixel(const wxPoint &pos, float brightness) const {
	if (!_data || pos.x < 0 || pos.y < 0 || pos.x >= _width || pos.y >= _height)
		return 0xFFFFFF;

	const uint8_t *row = _flipped ? _data + (_height - pos.y - 1) * _pitch : _data + pos.y * _pitch;
	const uint8_t *pixel = row + pos.x * _bytesPerPixel;
	int red;
	int green = pixel[1];
	int blue;

	if (_rgbOrder) {
		red = pixel[0];
		blue = pixel[2];
	} else {
		blue = pixel[0];
		red = pixel[2];
	}

	if (1.0f > brightness) {
		red = static_cast<int>(static_cast<float>(red) * brightness);
		green = static_cast<int>(static_cast<float>(green) * brightness);
		blue = static_cast<int>(static_cast<float>(blue) * brightness);
	}

	return (red & 0xFF) | ((green & 0xFF) << 8) | ((blue & 0xFF) << 16);
}

// Confirmed (asm lines 757692-757740)
void TPictureMEM::GetPixel(const char *pixel, unsigned char &red, unsigned char &green, unsigned char &blue,
                           unsigned char &alpha) const {
	const unsigned char *bytes = reinterpret_cast<const unsigned char *>(pixel);

	if (_rgbOrder) {
		red = bytes[0];
		green = bytes[1];
		blue = bytes[2];
	} else {
		blue = bytes[0];
		green = bytes[1];
		red = bytes[2];
	}

	if (_bytesPerPixel == 4)
		alpha = bytes[3];
}

// Confirmed (asm lines 757740-757785)
int TPictureMEM::GetAlphaAt(const wxPoint &pos) const {
	if (!_data || _bytesPerPixel != 4)
		return 0;

	if (pos.x < 0 || pos.y < 0 || pos.x >= _width || pos.y >= _height)
		return 0;

	return _data[pos.y * _pitch + pos.x * 4 + 3];
}

// Confirmed (asm lines 757785-757833)
char *TPictureMEM::InsertPixel(char *pixel, unsigned char red, unsigned char green, unsigned char blue,
                               unsigned char alpha) const {
	if (!pixel)
		return nullptr;

	unsigned char *bytes = reinterpret_cast<unsigned char *>(pixel);

	if (_rgbOrder) {
		bytes[0] = red;
		bytes[1] = green;
		bytes[2] = blue;
	} else {
		bytes[0] = blue;
		bytes[1] = green;
		bytes[2] = red;
	}

	bytes += 3;

	if (_bytesPerPixel == 4)
		*bytes++ = alpha;

	return reinterpret_cast<char *>(bytes);
}

// Confirmed (asm lines 757833-758000)
void TPictureMEM::Mirror() {
	if (_pitch <= 0 || _width <= 0 || !_data)
		return;

	uint8_t *line = new uint8_t[_pitch];
	int rowBytes = _bytesPerPixel * _width;

	for (int y = 0; y < _height; y++) {
		char *source = GetLine(y);
		const uint8_t *from = line + rowBytes;

		if (source)
			std::memcpy(line, source, rowBytes);

		uint8_t *to = reinterpret_cast<uint8_t *>(GetLine(y));

		for (int x = 0; x < _width; x++) {
			from -= _bytesPerPixel;
			to[0] = from[0];
			to[1] = from[1];
			to[2] = from[2];

			if (_bytesPerPixel == 4) {
				to[3] = from[3];
				to += 4;
			} else {
				to += 3;
			}
		}
	}

	delete[] line;
}

// Confirmed (asm lines 758000-758240)
bool TPictureMEM::CreateTransparencyBitmap(unsigned int color, eTransparencyMode mode, unsigned char **bitmap,
                                           int &rowBytes) const {
	// (the original does not look: it reads from a null line)
	if (!_data)
		return false;

	rowBytes = (((_width + 3) >> 2) + 7) >> 3;
	int rows = (_height + 3) >> 2;
	int step = _bytesPerPixel << 2;
	unsigned char keyFirst = static_cast<unsigned char>(color);
	unsigned char keySecond = static_cast<unsigned char>(color >> 16);
	unsigned char keyThird = static_cast<unsigned char>(color >> 8);

	// (the alpha mode needs an alpha)
	if (mode == eTransparencyMode::kAlpha && _bytesPerPixel != 4)
		return false;

	size_t size = static_cast<size_t>(rowBytes) * rows;
	unsigned char *result = new unsigned char[size];

	for (int row = 0; row < rows; row++) {
		int y = row * 4;
		const uint8_t *line = _flipped ? _data + (_height - y - 1) * _pitch : _data + y * _pitch;

		unsigned char *out = result + rowBytes * row;
		int bit = 0;

		for (int x = 0; x < _width; x += 4) {
			bool set;

			const uint8_t *pixel = line + (x / 4) * step;

			if (mode == eTransparencyMode::kAlpha)
				set = (~pixel[3] & 0x80) != 0;
			else
				set = pixel[0] == keyFirst && pixel[1] == keySecond && pixel[2] == keyThird;

			if (bit == 0)
				*out = set ? 1 : 0;
			else if (set)
				*out |= static_cast<unsigned char>(1 << bit);

			bit++;

			if ((bit & 7) == 0) {
				out++;
				bit = 0;
			}
		}
	}

	*bitmap = result;
	return true;
}

// Confirmed (asm lines 758253-758785). A rectangle of all zeros (or with x and y 0 or -1 and nothing else) is the
// whole of `source`. The rectangle is kept inside of the source and of this picture. The lines of a flipped picture
// are walked from the other end; when the colour key of this picture is on, a pixel that is the colour key gets alpha 0
// and the rest 255 (and the alpha of the source is not looked at). NOTE: with a flipped picture and a rectangle that is
// not as wide as the picture the original steps from the end of a line to the next one with -2 * (the width of the
// rectangle in bytes), which is right only when the rectangle is as wide as the picture; this is kept.
bool TPictureMEM::Paste(const TPictureMEM &source, const wxRect &srcRect, const wxPoint &destPos) {
	wxRect rect = srcRect;

	if ((rect.x == -1 || rect.x == 0) && (rect.y == -1 || rect.y == 0) && rect.width == 0 && rect.height == 0) {
		rect.x = 0;
		rect.y = 0;
		rect.width = source.GetWidth();
		rect.height = source.GetHeight();
	}

	if (rect.GetLeft() < 0) {
		int right = rect.GetRight();

		rect.SetLeft(0);
		rect.SetRight(right);
	}

	if (rect.GetTop() < 0) {
		int bottom = rect.GetBottom();

		rect.SetTop(0);
		rect.SetBottom(bottom);
	}

	if (rect.GetRight() >= source._width)
		rect.SetRight(source._width - 1);

	if (rect.GetBottom() >= source._height)
		rect.SetBottom(source._height - 1);

	if (!_data || !source._data)
		return false;

	if (rect.GetWidth() + destPos.x > _width)
		rect.SetWidth(_width - destPos.x);

	if (rect.GetHeight() + destPos.y > _height)
		rect.SetHeight(_height - destPos.y);

	int width = rect.GetWidth();
	int height = rect.GetHeight();
	char *sourceLine = source.GetLine(rect.GetTop());
	char *sourcePtr = sourceLine + rect.GetLeft() * source._bytesPerPixel;
	char *destLine = GetLine(destPos.y);
	char *destPtr = destLine + destPos.x * _bytesPerPixel;
	bool colorKey = (GetTransparency() == eTransparencyMode::kColorKey);
	unsigned int key = colorKey ? GetTransparentColor() : 0;
	unsigned char keyRed = static_cast<unsigned char>(key);
	unsigned char keyGreen = static_cast<unsigned char>(key >> 8);
	unsigned char keyBlue = static_cast<unsigned char>(key >> 16);

	if (source._rgbOrder == _rgbOrder && source._bytesPerPixel == _bytesPerPixel && !colorKey) {
		// the lines are copied as they are
		int rowBytes = width * _bytesPerPixel;

		for (int row = 0; row < height; row++) {
			std::memcpy(destPtr, sourcePtr, rowBytes);
			sourcePtr += source._flipped ? -source._pitch : source._pitch;
			destPtr += _flipped ? -_pitch : _pitch;
		}

		return true;
	}

	int sourceSkip = source._pitch - width * source._bytesPerPixel;
	int destSkip = _pitch - width * _bytesPerPixel;

	if (source._flipped)
		sourceSkip -= width * source._bytesPerPixel + source._pitch;

	if (_flipped)
		destSkip -= width * _bytesPerPixel + _pitch;

	for (int row = 0; row < height; row++) {
		for (int x = 0; x < width; x++) {
			unsigned char red;
			unsigned char green;
			unsigned char blue;
			unsigned char alpha = 0xFF;
			const unsigned char *in = reinterpret_cast<const unsigned char *>(sourcePtr);

			if (source._rgbOrder) {
				red = in[0];
				green = in[1];
				blue = in[2];
			} else {
				blue = in[0];
				green = in[1];
				red = in[2];
			}

			sourcePtr += 3;

			if (colorKey) {
				alpha = (red == keyRed && green == keyGreen && blue == keyBlue) ? 0 : 0xFF;

				if (source._bytesPerPixel == 4)
					sourcePtr++;
			} else if (source._bytesPerPixel == 4) {
				alpha = *reinterpret_cast<const unsigned char *>(sourcePtr);
				sourcePtr++;
			}

			unsigned char *out = reinterpret_cast<unsigned char *>(destPtr);

			if (_rgbOrder) {
				out[0] = red;
				out[1] = green;
				out[2] = blue;
			} else {
				out[0] = blue;
				out[1] = green;
				out[2] = red;
			}

			destPtr += 3;

			if (_bytesPerPixel == 4)
				*destPtr++ = static_cast<char>(alpha);
		}

		sourcePtr += sourceSkip;
		destPtr += destSkip;
	}

	return true;
}

// Confirmed (asm lines 758785-758947)
bool TPictureMEM::CopyFrom(const TPictureMEM &source, const wxRect &srcRect, const wxSize &size, const wxPoint &destPos,
                           const unsigned int &color) {
	SetImageSize(size.width, size.height);

	bool hasAlpha = source.IsMemoryImage() ? true : (source.GetTransparency() == eTransparencyMode::kColorKey);

	if (!InitMemory(hasAlpha))
		return false;

	SetMemoryImage();

	unsigned char red = static_cast<unsigned char>(color);
	unsigned char green = static_cast<unsigned char>(color >> 8);
	unsigned char blue = static_cast<unsigned char>(color >> 16);

	for (int y = 0; y < _height; y++) {
		char *pixel = GetLine(y);

		for (int x = 0; x < _width; x++) {
			if (!pixel)
				continue;

			pixel = InsertPixel(pixel, red, green, blue, 0);
		}
	}

	return Paste(source, srcRect, destPos);
}

// Confirmed (asm lines 758947-759017)
bool TPictureMEM::CopyFrom(const TPictureMEM &source, const wxRect &srcRect) {
	bool hasAlpha;

	if (source.IsMemoryImage() && source.GetTransparency() == eTransparencyMode::kAlpha)
		hasAlpha = true;
	else
		hasAlpha = (GetTransparency() == eTransparencyMode::kColorKey);

	if (!InitMemory(hasAlpha))
		return false;

	SetMemoryImage();
	return Paste(source, srcRect, wxPoint{0, 0});
}

// Confirmed (asm lines 759017-759239), except for the scaling (see the top of the file). The pixels are scaled as they are
// in memory, the picture gets the alpha and the transparency of `source`, and `flipped` (the 4th argument) as its setting;
// the colour order is `source`'s.
bool TPictureMEM::ResizeImage(const TPictureMEM &source, const wxPoint &size, bool /*bgr*/, bool flipped) {
	SetImageSize(size.x, size.y);

	if (!InitMemory(source._bytesPerPixel == 4))
		return false;

	_flipped = flipped;
	SetMemoryImage();
	_bytesPerPixel = source._bytesPerPixel;
	_pitch = _bytesPerPixel * size.x;
	_rgbOrder = source._rgbOrder;
	SetTransparency(source.GetTransparency(), source.GetTransparentColor());

	if (!source._data || source._width <= 0 || source._height <= 0 || size.x <= 0 || size.y <= 0)
		return true;

	float xRatio = static_cast<float>(source.GetWidth()) / static_cast<float>(size.x);
	float yRatio = static_cast<float>(source.GetHeight()) / static_cast<float>(size.y);

	for (int y = 0; y < size.y; y++) {
		float sy = (static_cast<float>(y) + 0.5f) * yRatio - 0.5f;
		int y0 = std::max(0, std::min(source.GetHeight() - 1, static_cast<int>(std::floor(sy))));
		int y1 = std::min(source.GetHeight() - 1, y0 + 1);
		float fy = std::max(0.0f, sy - static_cast<float>(y0));
		uint8_t *out = _data + y * _pitch;

		for (int x = 0; x < size.x; x++) {
			float sx = (static_cast<float>(x) + 0.5f) * xRatio - 0.5f;
			int x0 = std::max(0, std::min(source.GetWidth() - 1, static_cast<int>(std::floor(sx))));
			int x1 = std::min(source.GetWidth() - 1, x0 + 1);
			float fx = std::max(0.0f, sx - static_cast<float>(x0));

			for (int c = 0; c < _bytesPerPixel; c++) {
				float a = source._data[y0 * source._pitch + x0 * source._bytesPerPixel + c];
				float b = source._data[y0 * source._pitch + x1 * source._bytesPerPixel + c];
				float d = source._data[y1 * source._pitch + x0 * source._bytesPerPixel + c];
				float e = source._data[y1 * source._pitch + x1 * source._bytesPerPixel + c];
				float top = a + (b - a) * fx;
				float bottom = d + (e - d) * fx;

				out[x * _bytesPerPixel + c] = static_cast<uint8_t>(top + (bottom - top) * fy + 0.5f);
			}
		}
	}

	return true;
}

// Confirmed (asm lines 759239-759314): the cubic B-spline, (P(x+2)^3 - 4 P(x+1)^3 + 6 P(x)^3 - 4 P(x-1)^3) / 6 with
// P(v) = v if it is above 0, else 0. Nothing calls it.
double TPictureMEM::WeightingFunction(double x) {
	auto positive = [](double v) {
		return (v > 0.0) ? std::pow(v, 3.0) : 0.0;
	};

	if (!(x + 2.0 > 0.0))
		return 0.0;

	return (positive(x + 2.0) - 4.0 * positive(x + 1.0) + 6.0 * positive(x) - 4.0 * positive(x - 1.0)) / 6.0;
}

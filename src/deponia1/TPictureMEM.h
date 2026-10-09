// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Reconstructed from Deponia_Linux.asm lines 756823-759239 (22 methods). TPictureIO's base class (confirmed:
// TPictureIO's ctor/dtor call TPictureMEM::TPictureMEM()/~TPictureMEM() directly on `this`, no offset
// adjustment), derived from TSprite. It owns the pixels of a picture: a buffer of `_height` lines of `_pitch`
// bytes, 3 (RGB) or 4 (RGBA) bytes to a pixel. The buffer is a new[] of the picture, or - when the picture was
// given a memory block (SetMemoryBlock(): the engine keeps two big ones, GetMainMemBlock() and
// GetLightMapMemBlock(), so that pictures that are loaded often do not allocate) - the next bytes of the block.
//
// Two settings that the graphics backend gives (TGraphicsInterface::GetPicsMemSettings()) tell how the pixels
// are laid out: `_rgbOrder` (the bytes of a pixel are red, green, blue; else blue, green, red) and `_flipped`
// (the lines are stored from the bottom: GetLine(0) is the last line).
//
// ResizeImage() is not exact: the original scales with libswscale (Lanczos, bilinear when that cannot be made);
// here the picture is scaled bilinear.
#pragma once

#include <stdint.h>

#include "TSprite.h"

/** A block of memory that pictures are put into one after the other (the engine's picture buffers; see
 *  TGraphicsInterface::GetMainMemBlock()). 0x18 bytes in the original. */
struct TPictureMemBlock {
	uint8_t *data = nullptr;  // +0x00
	int64_t size = 0;         // +0x08, how many bytes the block has
	int64_t used = 0;         // +0x10, where the next picture begins
};

class TPictureMEM : public TSprite {
public:
	TPictureMEM();
	explicit TPictureMEM(const TSprite &sprite);
	virtual ~TPictureMEM();

	/** Lets the pixels go (a buffer of the picture's own is deleted). */
	void ClearMemData();
	/** The picture puts its pixels into this block (or into a buffer of its own: null). */
	void SetMemoryBlock(TPictureMemBlock *block);

	/** Makes the buffer for a picture of GetWidth() x GetHeight() (TSprite's image size): 4 bytes to a pixel with alpha,
	 *  else 3. False when the size is not valid (0 to 100000) or there is no room. */
	bool InitMemory(bool hasAlpha);
	/** The picture is `width` x `height` pixels at `data` (that it does not own... it deletes the buffer when it lets
	 *  go: the buffer is new[] of the caller's). `hasAlpha`: 4 bytes to a pixel; `rgbOrder`; the last flag of the
	 *  original is not used. A picture with alpha is flipped. */
	void SetMemoryData(char *data, int width, int height, bool hasAlpha, bool rgbOrder, bool flag);
	/** The pixels (null when there are none). */
	char *GetMemoryData() const {
		return reinterpret_cast<char *>(_data);
	}
	/** How many bytes the pixels take (0 when there are none). */
	int GetMemorySize() const;
	/** The pixels of line `y` (the lines are counted from the top; null when the line is not in the picture). */
	char *GetLine(int y) const;

	/** The colour (0x00BBGGRR) of the pixel at `pos`, with each part of it multiplied by `brightness` when that is below
	 *  1.0; white (0xFFFFFF) outside of the picture. */
	unsigned int GetPixel(const wxPoint &pos, float brightness) const;
	/** The parts of the pixel at `pixel` (alpha only if the pixels have it). */
	void GetPixel(const char *pixel, unsigned char &red, unsigned char &green, unsigned char &blue,
	              unsigned char &alpha) const;
	/** The alpha of the pixel at `pos` (0 if the pixels have none or `pos` is outside; the lines are NOT turned for a
	 *  flipped picture). */
	int GetAlphaAt(const wxPoint &pos) const;
	/** Puts a pixel at `pixel`; returns where the next one is (null if `pixel` was null). */
	char *InsertPixel(char *pixel, unsigned char red, unsigned char green, unsigned char blue, unsigned char alpha) const;

	/** Turns every line round. */
	void Mirror();

	/** A 1-bit-a-pixel picture of every 4th pixel of every 4th line: `mode` kColorKey: a bit is set where the pixel is
	 *  `color` (the parts compared in the order red, blue, green - as the original does), kAlpha: where the alpha is
	 *  below 128. `rowBytes` gets how many bytes a line of the bitmap has. The bitmap is new[]. */
	bool CreateTransparencyBitmap(unsigned int color, eTransparencyMode mode, unsigned char **bitmap,
	                              int &rowBytes) const;

	/** Copies the part `srcRect` of `source` (an empty rectangle: all of it) to `destPos` of this picture, the colour
	 *  key of this picture becoming alpha 0. False if one of the pictures has no pixels. */
	bool Paste(const TPictureMEM &source, const wxRect &srcRect, const wxPoint &destPos);
	/** This picture is `size` big, filled with `color` (alpha 0), and the part `srcRect` of `source` is pasted at
	 *  `destPos`. */
	bool CopyFrom(const TPictureMEM &source, const wxRect &srcRect, const wxSize &size, const wxPoint &destPos,
	              const unsigned int &color);
	/** This picture is made from the part `srcRect` of `source`. */
	bool CopyFrom(const TPictureMEM &source, const wxRect &srcRect);
	/** This picture is `source` scaled to `size` (see above: bilinear). `flipped` is the setting of the new picture; `bgr`
	 *  (the colour order) is replaced by the source's by the original. */
	bool ResizeImage(const TPictureMEM &source, const wxPoint &size, bool bgr, bool flipped);

	/** The weight of the B-spline of a bicubic scaling (not used by the player). */
	static double WeightingFunction(double x);

	/** TPictureIO uses these. */
	int GetMemWidth() const {
		return _width;
	}
	int GetMemHeight() const {
		return _height;
	}
	int GetBytesPerPixel() const {
		return _bytesPerPixel;
	}
	bool IsRgbOrder() const {
		return _rgbOrder;
	}

protected:
	int _width = 0;                       // +0x60
	int _height = 0;                      // +0x64

private:
	TPictureMemBlock *_memBlock = nullptr;  // +0x50
	uint8_t *_data = nullptr;               // +0x58
	signed char _bytesPerPixel = 0;         // +0x68
	int _pitch = 0;                         // +0x6C, how many bytes a line has
	bool _field70 = true;                   // +0x70 (set by the constructor, not read)
	bool _flipped = false;                  // +0x71, the lines are stored from the bottom (SetMemoryData() stores 2)
	bool _rgbOrder = true;                  // +0x72
};

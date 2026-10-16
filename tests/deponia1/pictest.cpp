#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "AppGlobals.h"
#include "TPictureMEM.h"
#include "graphicslib/graphics.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

// The pictures made of pixels in memory: reading the pixels, turning the lines round, taking a part of a picture, pasting it
// into a bigger one, scaling, and the bitmap of the transparent parts.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	// 4 x 2 pixels with alpha, red, green, blue, alpha: the red is 10 * x, the green 20 * y, the blue 5, the alpha 255 (100 for x == 3)
	const int width = 4, height = 2;
	std::vector<char> pixels(width * height * 4);

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			char *pixel = &pixels[(y * width + x) * 4];

			pixel[0] = (char)(10 * x);
			pixel[1] = (char)(20 * y);
			pixel[2] = 5;
			pixel[3] = (char)(x == 3 ? 100 : 255);
		}
	}

	TPictureMEM source;

	source.TakePixels(pixels.data(), width, height, 4);
	// (the size of the image is TSprite's, which the loading of a picture sets)
	source.SetImageSize(width, height);
	source.SetLoadedImage();
	CHECK(source.GetPitch() == 16);
	CHECK(source.GetMemWidth() == 4 && source.GetMemHeight() == 2);
	CHECK(source.GetMemorySize() == 32);

	// the pixels
	CHECK(source.GetPixel(wxPoint{2, 1}, 1.0f) == (20u | (20u << 8) | (5u << 16)));
	CHECK(source.GetPixel(wxPoint{9, 1}, 1.0f) == 0xFFFFFFu);
	CHECK(source.GetPixel(wxPoint{2, 1}, 0.5f) == (10u | (10u << 8) | (2u << 16)));
	CHECK(source.GetAlphaAt(wxPoint{3, 0}) == 100);
	CHECK(source.GetAlphaAt(wxPoint{0, 0}) == 255);
	CHECK(source.GetAlphaAt(wxPoint{-1, 0}) == 0);
	CHECK(source.GetLine(1) == pixels.data() + 16);
	CHECK(source.GetLine(2) == nullptr);

	unsigned char red, green, blue, alpha = 0;

	source.GetPixel(&pixels[(1 * width + 3) * 4], red, green, blue, alpha);
	CHECK(red == 30 && green == 20 && blue == 5 && alpha == 100);

	// a pixel put in
	char pixel[8];

	CHECK(source.InsertPixel(pixel, 1, 2, 3, 4) == pixel + 4);
	CHECK(pixel[0] == 1 && pixel[1] == 2 && pixel[2] == 3 && pixel[3] == 4);
	CHECK(source.InsertPixel(nullptr, 1, 2, 3, 4) == nullptr);

	// the bitmap of the transparent: where the alpha is below 128 (every 4th pixel of every 4th line: here the one at (0, 0)
	// and the one at (4, 0) if there was one)
	{
		std::vector<char> clear(8 * 8 * 4, (char)255);

		for (int i = 0; i < 8 * 8; i++)
			clear[i * 4 + 3] = (char)255;
		clear[3] = 0;  // the alpha of (0, 0)

		TPictureMEM picture;

		picture.TakePixels(clear.data(), 8, 8, 4);

		unsigned char *bitmap = nullptr;
		int rowBytes = 0;

		CHECK(picture.CreateTransparencyBitmap(0, eTransparencyMode::kAlpha, &bitmap, rowBytes));
		CHECK(rowBytes == 1);
		CHECK(bitmap != nullptr);
		if (bitmap) {
			printf("bitmap bytes %02X %02X\n", bitmap[0], bitmap[1]);
			CHECK((bitmap[0] & 0x80) != 0 || (bitmap[0] & 0x01) != 0);
			delete[] bitmap;
		}
		picture.DropMemData();
	}

	// turned round: every line backwards
	{
		std::vector<char> copy = pixels;
		TPictureMEM picture;

		picture.TakePixels(copy.data(), width, height, 4);
		picture.Mirror();
		CHECK(picture.GetPixel(wxPoint{0, 0}, 1.0f) == (30u | (0u << 8) | (5u << 16)));
		CHECK(picture.GetPixel(wxPoint{3, 1}, 1.0f) == (0u | (20u << 8) | (5u << 16)));
		CHECK(picture.GetAlphaAt(wxPoint{0, 0}) == 100);
		picture.Mirror();
		CHECK(copy == pixels);
		picture.DropMemData();
	}

	// a part of the picture
	{
		TPictureMEM part;
		wxRect area;

		area.x = 1;
		area.y = 0;
		area.width = 2;
		area.height = 2;
		part.SetImageSize(2, 2);
		part.SetTransparency(eTransparencyMode::kAlpha, 0);
		CHECK(part.CopyFrom(source, area));
		CHECK(part.GetMemWidth() == 2 && part.GetMemHeight() == 2);
		printf("part pixel (0,1): %06X, source (1,1): %06X\n", part.GetPixel(wxPoint{0, 1}, 1.0f), source.GetPixel(wxPoint{1, 1}, 1.0f));
		CHECK(part.GetPixel(wxPoint{0, 1}, 1.0f) == source.GetPixel(wxPoint{1, 1}, 1.0f));
		CHECK(part.GetPixel(wxPoint{1, 0}, 1.0f) == source.GetPixel(wxPoint{2, 0}, 1.0f));
	}

	// pasted into a bigger picture that is filled with a colour first
	{
		TPictureMEM big;
		wxRect all;

		all.x = 0;
		all.y = 0;
		all.width = 0;
		all.height = 0;
		CHECK(big.CopyFrom(source, all, wxSize{6, 4}, wxPoint{1, 1}, 0x00804020u));
		CHECK(big.GetMemWidth() == 6 && big.GetMemHeight() == 4);
		// the corner is the colour (red 0x20, green 0x40, blue 0x80) with no alpha, the pasted area is the picture
		CHECK(big.GetPixel(wxPoint{0, 0}, 1.0f) == 0x00804020u);
		CHECK(big.GetAlphaAt(wxPoint{0, 0}) == 0);
		CHECK(big.GetPixel(wxPoint{3, 2}, 1.0f) == source.GetPixel(wxPoint{2, 1}, 1.0f));
		CHECK(big.GetAlphaAt(wxPoint{4, 1}) == 100);
	}

	// scaled: the size is the new one, and a picture of one colour stays so
	{
		std::vector<char> flat(4 * 4 * 4);

		for (int i = 0; i < 16; i++) {
			flat[i * 4] = 40;
			flat[i * 4 + 1] = 80;
			flat[i * 4 + 2] = 120;
			flat[i * 4 + 3] = (char)255;
		}

		TPictureMEM small;
		TPictureMEM scaled;

		small.TakePixels(flat.data(), 4, 4, 4);
		CHECK(scaled.ResizeImage(small, wxPoint{9, 6}, false, false));
		CHECK(scaled.GetMemWidth() == 9 && scaled.GetMemHeight() == 6);
		CHECK(scaled.GetBytesPerPixel() == 4);
		CHECK(scaled.GetPixel(wxPoint{0, 0}, 1.0f) == (40u | (80u << 8) | (120u << 16)));
		CHECK(scaled.GetPixel(wxPoint{8, 5}, 1.0f) == (40u | (80u << 8) | (120u << 16)));
		CHECK(scaled.GetPixel(wxPoint{4, 3}, 1.0f) == (40u | (80u << 8) | (120u << 16)));
		small.DropMemData();
	}

	// the transparent edges of a picture are taken off: the rectangle that has something in it
	{
		std::vector<char> image(8 * 8 * 4, 0);

		for (int y = 3; y <= 4; y++) {
			for (int x = 2; x <= 5; x++)
				image[(y * 8 + x) * 4 + 3] = (char)200;
		}

		int left = 0, top = 0, w = 8, h = 8;

		graphics->RemoveTransparentEdges(left, top, w, h, image.data());
		printf("opaque part: left %d top %d size %dx%d\n", left, top, w, h);
		CHECK(left == 2);
		CHECK(top == 3);
		CHECK(w == 4);
		CHECK(h == 2);

		// nothing in it at all: nothing is left
		std::vector<char> empty(8 * 8 * 4, 0);

		left = 0;
		top = 0;
		w = 8;
		h = 8;
		graphics->RemoveTransparentEdges(left, top, w, h, empty.data());
		CHECK(h <= 0 || w <= 0);
	}

	source.DropMemData();

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

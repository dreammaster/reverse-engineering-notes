#include "graphicslib/softwareGraphics.h"

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "zlibShim.h"

TSoftwareGraphics::TSoftwareGraphics(int width, int height) : _frameWidth(width), _frameHeight(height) {
	_frame.assign((size_t)width * height * 4, 0);
	_shown = _frame;
	SetSize(width, height);
	SetSurfaceSize(width, height);
}

bool TSoftwareGraphics::CreateSprite(TSpriteHandle **sprite, const char *data, int width, int height, int bytesPerPixel, int pitch,
                                     bool flag) {
	if (!TGraphicsInterface::CreateSprite(sprite, data, width, height, bytesPerPixel, pitch, flag))
		return false;

	Pixels &pixels = _sprites[*sprite];

	pixels.width = width;
	pixels.height = height;
	pixels.rgba.assign((size_t)width * height * 4, 0);

	if (data) {
		for (int y = 0; y < height; y++) {
			const uint8_t *line = reinterpret_cast<const uint8_t *>(data) + (size_t)y * pitch;
			uint8_t *out = &pixels.rgba[(size_t)y * width * 4];

			for (int x = 0; x < width; x++) {
				if (bytesPerPixel >= 4) {
					std::memcpy(out + x * 4, line + x * bytesPerPixel, 4);
				} else if (bytesPerPixel == 3) {
					std::memcpy(out + x * 4, line + x * 3, 3);
					out[x * 4 + 3] = 255;
				}
			}
		}
	}
	return true;
}

void TSoftwareGraphics::OnSpriteHandleReleased(TSpriteHandle *handle) {
	_sprites.erase(handle);
	TGraphicsInterface::OnSpriteHandleReleased(handle);
}

void TSoftwareGraphics::SetOffset(float x, float y) {
	_offsetX = x;
	_offsetY = y;
}

// A colour with its alpha over the frame (the colour is premultiplied: dst = src + dst * (1 - alpha)).
void TSoftwareGraphics::blend(int x, int y, float r, float g, float b, float a) {
	if (x < 0 || y < 0 || x >= _frameWidth || y >= _frameHeight)
		return;

	uint8_t *dst = &_frame[((size_t)y * _frameWidth + x) * 4];
	float keep = 1.0f - a;

	dst[0] = (uint8_t)std::min(255.0f, r * 255.0f + dst[0] * keep + 0.5f);
	dst[1] = (uint8_t)std::min(255.0f, g * 255.0f + dst[1] * keep + 0.5f);
	dst[2] = (uint8_t)std::min(255.0f, b * 255.0f + dst[2] * keep + 0.5f);
	dst[3] = (uint8_t)std::min(255.0f, a * 255.0f + dst[3] * keep + 0.5f);
}

void TSoftwareGraphics::Draw(TSpriteHandle *sprite, const wxRect &sourceRect, const FloatRect &destRect, float alpha, bool mirrored,
                             const unsigned int &color, int /*shader*/, float rotation, const wxPoint &rotationCenter, float scaleX,
                             float scaleY, int /*matrixId*/) {
	auto found = _sprites.find(sprite);

	if (std::getenv("RENDER_TRACE") && _drawn < 80)
		std::fprintf(stderr, "draw %s %dx%d src %d,%d %dx%d dest %.0f,%.0f %.0fx%.0f alpha %.2f color %08x rot %.2f scale %.2f,%.2f"
		                     "%c", found == _sprites.end() ? "(unknown sprite)" : "", sprite ? sprite->width : 0, sprite ? sprite->height : 0,
		             sourceRect.x, sourceRect.y, sourceRect.width, sourceRect.height, destRect.x, destRect.y, destRect.width,
		             destRect.height, alpha, color, rotation, scaleX, scaleY, 10);

	if (found == _sprites.end() || destRect.width <= 0.0f || destRect.height <= 0.0f)
		return;

	const Pixels &pixels = found->second;
	int sx = std::max(0, sourceRect.x);
	int sy = std::max(0, sourceRect.y);
	int sw = std::min(sourceRect.width, pixels.width - sx);
	int sh = std::min(sourceRect.height, pixels.height - sy);

	if (sw <= 0 || sh <= 0)
		return;

	// the quad on the screen: the rectangle, scaled and turned about the centre
	float dx = destRect.x + _offsetX;
	float dy = destRect.y + _offsetY;
	float centerX = rotationCenter.x < 0 ? dx + destRect.width / 2.0f : (float)rotationCenter.x + _offsetX;
	float centerY = rotationCenter.y < 0 ? dy + destRect.height / 2.0f : (float)rotationCenter.y + _offsetY;
	float sinus = std::sin(rotation);
	float cosinus = std::cos(rotation);
	float scaleXs = scaleX == 0.0f ? 1.0f : scaleX;
	float scaleYs = scaleY == 0.0f ? 1.0f : scaleY;
	bool plain = rotation == 0.0f && scaleXs == 1.0f && scaleYs == 1.0f;

	// the bounding box
	float cornersX[4] = {dx, dx + destRect.width, dx, dx + destRect.width};
	float cornersY[4] = {dy, dy, dy + destRect.height, dy + destRect.height};
	float minX = 1e9f, minY = 1e9f, maxX = -1e9f, maxY = -1e9f;

	for (int i = 0; i < 4; i++) {
		float px = (cornersX[i] - centerX) * scaleXs;
		float py = (cornersY[i] - centerY) * scaleYs;
		float rx = centerX + px * cosinus - py * sinus;
		float ry = centerY + px * sinus + py * cosinus;

		minX = std::min(minX, rx);
		maxX = std::max(maxX, rx);
		minY = std::min(minY, ry);
		maxY = std::max(maxY, ry);
	}

	int x0 = std::max(0, (int)std::floor(minX));
	int y0 = std::max(0, (int)std::floor(minY));
	int x1 = std::min(_frameWidth, (int)std::ceil(maxX));
	int y1 = std::min(_frameHeight, (int)std::ceil(maxY));

	float tintR = (float)(color & 0xFF) / 255.0f;
	float tintG = (float)((color >> 8) & 0xFF) / 255.0f;
	float tintB = (float)((color >> 16) & 0xFF) / 255.0f;
	float tintA = (float)((color >> 24) & 0xFF) / 255.0f * alpha;

	bool smooth = !plain || std::fabs(destRect.width - sw) > 0.5f || std::fabs(destRect.height - sh) > 0.5f;

	_drawn++;

	for (int y = y0; y < y1; y++) {
		for (int x = x0; x < x1; x++) {
			// the position in the rectangle that is not scaled or turned
			float px = (float)x + 0.5f - centerX;
			float py = (float)y + 0.5f - centerY;
			float ux = centerX + (px * cosinus + py * sinus) / scaleXs;
			float uy = centerY + (-px * sinus + py * cosinus) / scaleYs;

			float fx = (ux - dx) / destRect.width;
			float fy = (uy - dy) / destRect.height;

			if (fx < 0.0f || fy < 0.0f || fx >= 1.0f || fy >= 1.0f)
				continue;
			if (mirrored)
				fx = 1.0f - fx;

			float u = sx + fx * sw;
			float v = sy + fy * sh;
			float rgba[4];

			if (!smooth) {
				const uint8_t *p = &pixels.rgba[((size_t)std::min(pixels.height - 1, (int)v) * pixels.width + std::min(pixels.width - 1, (int)u)) * 4];

				for (int i = 0; i < 4; i++)
					rgba[i] = p[i] / 255.0f;
			} else {
				float bu = std::max((float)sx, u - 0.5f);
				float bv = std::max((float)sy, v - 0.5f);
				int iu = (int)bu;
				int iv = (int)bv;
				float tu = bu - iu;
				float tv = bv - iv;
				int iu2 = std::min(sx + sw - 1, iu + 1);
				int iv2 = std::min(sy + sh - 1, iv + 1);
				const uint8_t *p00 = &pixels.rgba[((size_t)iv * pixels.width + iu) * 4];
				const uint8_t *p10 = &pixels.rgba[((size_t)iv * pixels.width + iu2) * 4];
				const uint8_t *p01 = &pixels.rgba[((size_t)iv2 * pixels.width + iu) * 4];
				const uint8_t *p11 = &pixels.rgba[((size_t)iv2 * pixels.width + iu2) * 4];

				for (int i = 0; i < 4; i++) {
					float top = p00[i] * (1.0f - tu) + p10[i] * tu;
					float bottom = p01[i] * (1.0f - tu) + p11[i] * tu;

					rgba[i] = (top * (1.0f - tv) + bottom * tv) / 255.0f;
				}
			}

			blend(x, y, rgba[0] * tintR * tintA, rgba[1] * tintG * tintA, rgba[2] * tintB * tintA, rgba[3] * tintA);
		}
	}
}

void TSoftwareGraphics::DrawWithLightMap(TSpriteHandle *sprite, const wxRect &sourceRect, const FloatRect &destRect, float alpha,
                                         bool mirrored, void * /*lightMap*/, const wxPoint & /*offset*/) {
	unsigned int white = 0xFFFFFFFF;

	Draw(sprite, sourceRect, destRect, alpha, mirrored, white, -1, 0.0f, wxPoint{-1, -1}, 1.0f, 1.0f, 0);
}

void TSoftwareGraphics::DrawBox(const wxRect &rect, unsigned int color, float alpha) {
	float a = (float)((color >> 24) & 0xFF) / 255.0f * alpha;
	float r = (float)(color & 0xFF) / 255.0f * a;
	float g = (float)((color >> 8) & 0xFF) / 255.0f * a;
	float b = (float)((color >> 16) & 0xFF) / 255.0f * a;

	for (int y = rect.y + (int)_offsetY; y < rect.y + (int)_offsetY + rect.height; y++) {
		for (int x = rect.x + (int)_offsetX; x < rect.x + (int)_offsetX + rect.width; x++)
			blend(x, y, r, g, b, a);
	}
	_drawn++;
}

void TSoftwareGraphics::DrawLine(const wxPoint &from, const wxPoint &to, unsigned int color, float alpha) {
	float a = (float)((color >> 24) & 0xFF) / 255.0f * alpha;
	float r = (float)(color & 0xFF) / 255.0f * a;
	float g = (float)((color >> 8) & 0xFF) / 255.0f * a;
	float b = (float)((color >> 16) & 0xFF) / 255.0f * a;
	int x = from.x + (int)_offsetX;
	int y = from.y + (int)_offsetY;
	int x2 = to.x + (int)_offsetX;
	int y2 = to.y + (int)_offsetY;
	int dx = std::abs(x2 - x);
	int dy = -std::abs(y2 - y);
	int stepX = x < x2 ? 1 : -1;
	int stepY = y < y2 ? 1 : -1;
	int error = dx + dy;

	for (;;) {
		blend(x, y, r, g, b, a);
		if (x == x2 && y == y2)
			break;

		int doubled = 2 * error;

		if (doubled >= dy) {
			error += dy;
			x += stepX;
		}
		if (doubled <= dx) {
			error += dx;
			y += stepY;
		}
	}
	_drawn++;
}

// The frame is shown: it is kept and the next one starts black.
void TSoftwareGraphics::Swap() {
	_shown = _frame;
	_shownDrawn = _drawn;
	_drawn = 0;
	std::fill(_frame.begin(), _frame.end(), 0);
	_offsetX = 0.0f;
	_offsetY = 0.0f;
	SetFadeDrawn(false);
}

static uint32_t crcTable[256];

static uint32_t crc32Of(const uint8_t *data, size_t size, uint32_t crc = 0xFFFFFFFFu) {
	if (!crcTable[1]) {
		for (uint32_t n = 0; n < 256; n++) {
			uint32_t c = n;

			for (int k = 0; k < 8; k++)
				c = (c & 1) ? 0xEDB88320u ^ (c >> 1) : c >> 1;
			crcTable[n] = c;
		}
	}
	for (size_t i = 0; i < size; i++)
		crc = crcTable[(crc ^ data[i]) & 0xFF] ^ (crc >> 8);
	return crc;
}

static void putChunk(std::vector<uint8_t> &out, const char *type, const std::vector<uint8_t> &data) {
	auto big = [&](uint32_t value) {
		out.push_back((uint8_t)(value >> 24));
		out.push_back((uint8_t)(value >> 16));
		out.push_back((uint8_t)(value >> 8));
		out.push_back((uint8_t)value);
	};

	big((uint32_t)data.size());

	size_t start = out.size();

	out.insert(out.end(), type, type + 4);
	out.insert(out.end(), data.begin(), data.end());
	big(crc32Of(&out[start], out.size() - start) ^ 0xFFFFFFFFu);
}

bool TSoftwareGraphics::SavePNG(const char *path, int reduce) const {
	if (reduce < 1)
		reduce = 1;

	int width = _frameWidth / reduce;
	int height = _frameHeight / reduce;
	std::vector<uint8_t> raw;

	raw.reserve((size_t)height * (width * 3 + 1));
	for (int y = 0; y < height; y++) {
		raw.push_back(0);
		for (int x = 0; x < width; x++) {
			// (the pixels of a reduced picture are the average of the block)
			int sum[3] = {0, 0, 0};

			for (int by = 0; by < reduce; by++) {
				for (int bx = 0; bx < reduce; bx++) {
					const uint8_t *p = &_shown[((size_t)(y * reduce + by) * _frameWidth + x * reduce + bx) * 4];

					sum[0] += p[0];
					sum[1] += p[1];
					sum[2] += p[2];
				}
			}
			for (int i = 0; i < 3; i++)
				raw.push_back((uint8_t)(sum[i] / (reduce * reduce)));
		}
	}

	unsigned long compressedSize = raw.size() + raw.size() / 100 + 1024;
	std::vector<uint8_t> compressed(compressedSize);

	if (zlibCompress(compressed.data(), &compressedSize, raw.data(), raw.size()) != 0)
		return false;
	compressed.resize(compressedSize);

	std::vector<uint8_t> png = {0x89, 'P', 'N', 'G', 0x0D, 0x0A, 0x1A, 0x0A};
	std::vector<uint8_t> header;

	for (int shift = 24; shift >= 0; shift -= 8)
		header.push_back((uint8_t)(width >> shift));
	for (int shift = 24; shift >= 0; shift -= 8)
		header.push_back((uint8_t)(height >> shift));
	header.push_back(8); // bits
	header.push_back(2); // RGB
	header.push_back(0);
	header.push_back(0);
	header.push_back(0);
	putChunk(png, "IHDR", header);
	putChunk(png, "IDAT", compressed);
	putChunk(png, "IEND", std::vector<uint8_t>());

	FILE *file = std::fopen(path, "wb");

	if (!file)
		return false;
	std::fwrite(png.data(), 1, png.size(), file);
	std::fclose(file);
	return true;
}

// Reconstructed from Deponia_Linux.asm lines 759351-761700 (TPictureWebP, 11 methods and the writer callback). libwebp
// (third_party/libwebp, version 0.5.0) does the decoding and the encoding, as in the original.
#include "TPictureFormat.h"

#include <cstring>

#include <webp/decode.h>
#include <webp/encode.h>

#include "Diagnostics.h"
#include "TMemoryBuffer.h"
#include "TPictureMEM.h"
#include "WxStub.h"
#include "baselib/file.h"

// The biggest picture the reader accepts: width and height from 1 to 19999.
static const int kMaxSize = 19999;

static unsigned long readBigEndian(const unsigned char *p) {
	return ((unsigned long)p[0] << 24) | ((unsigned long)p[1] << 16) | ((unsigned long)p[2] << 8) | p[3];
}

static void writeBigEndian(unsigned char *p, unsigned long value) {
	p[0] = (unsigned char)(value >> 24);
	p[1] = (unsigned char)(value >> 16);
	p[2] = (unsigned char)(value >> 8);
	p[3] = (unsigned char)value;
}

// A picture of a composed file may have the content flag "PNG header encrypted": then the eight bytes at 0x10 of the file (two
// big endian numbers: for a WebP the size of the first chunk and the first four bytes of its data) are encrypted with the name
// of the file (see TPictureFormat.h). They are decrypted in place.
static void decryptHeader(TFile &file, unsigned char *at) {
	TMemoryBuffer buffer;

	buffer << (long)readBigEndian(at);
	buffer << (long)readBigEndian(at + 4);
	buffer.Decrypt(file.GetOriginalPath().GetName(), nullptr, 0);

	writeBigEndian(at, readBigEndian(buffer.GetData()));
	writeBigEndian(at + 4, readBigEndian(buffer.GetData() + 4));
	buffer.ReleaseMemory();
}

// Confirmed (asm lines 759971-759992)
TPictureWebP::TPictureWebP() {
	_format = 5;
}

TPictureWebP::~TPictureWebP() {
	CleanUp();
}

// Confirmed (asm lines 759351-759375)
bool TPictureWebP::CleanUp() {
	delete[] _data;
	_data = nullptr;
	return true;
}

// Confirmed (asm lines 759993-760904). The size of a lossy picture (a "VP8 " chunk) and of one with extended features ("VP8X")
// is read from the first 30 bytes of the file; of any other (a lossless "VP8L") libwebp reads it from the whole file, which stays
// in the format for the pixels.
bool TPictureWebP::ReadHeader(TFile &file, int *outWidth, int *outHeight) {
	unsigned char head[32];

	file.Seek(0, 0);
	std::memset(head, 0, sizeof(head));
	file.ReadMem(head, 30);

	if (file.IsPNGHeaderEncrypted())
		decryptHeader(file, head + 0x10);

	if (std::memcmp(head + 12, "VP8X", 4) == 0) {
		*outWidth = (head[0x18] | (head[0x19] << 8) | (head[0x1A] << 16)) + 1;
		*outHeight = (head[0x1B] | (head[0x1C] << 8) | (head[0x1D] << 16)) + 1;
		_width = *outWidth;
		_height = *outHeight;

		if (*outWidth < 1 || *outWidth > kMaxSize || *outHeight < 1 || *outHeight > kMaxSize) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"error invalid size %d %d", *outWidth, *outHeight);
			return false;
		}
		return true;
	}

	if (std::memcmp(head + 12, "VP8 ", 4) == 0) {
		if (head[0x17] != 0x9D || head[0x18] != 1 || head[0x19] != 0x2A) {
			if (wxLog::loglevel >= 0)
				wxLog::logexpanded(L"error VP8 detected but no chunk signature");
			return false;
		}
		*outWidth = (head[0x1A] | (head[0x1B] << 8)) & 0x3FFF;
		*outHeight = (head[0x1C] | (head[0x1D] << 8)) & 0x3FFF;
		return true;
	}

	CleanUp();
	file.Seek(0, 0);
	_size = (int)file.GetFileLength();
	_data = new unsigned char[_size];
	file.ReadMem(_data, _size);

	if (file.IsPNGHeaderEncrypted())
		decryptHeader(file, _data + 0x10);

	WebPBitstreamFeatures features;
	VP8StatusCode status = WebPGetFeatures(_data, _size, &features);

	if (status != VP8_STATUS_OK) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"File %ls %ls has wrong format, error %d", file.GetOriginalPath().GetName().wc_str(),
			                   file.GetOriginalPath().GetFullName().wc_str(), (int)status);
		return false;
	}

	_width = features.width;
	_height = features.height;
	*outWidth = _width;
	*outHeight = _height;
	return true;
}

// Confirmed (asm lines 759860-759970). The data and its size are the format's own (the lua function loadMemoryWEBP does not set
// them: the original asks libwebp about no data and fails here, as the arguments are not used).
bool TPictureWebP::ReadHeader(char * /*data*/, unsigned long /*size*/, int *outWidth, int *outHeight) {
	WebPBitstreamFeatures features;
	VP8StatusCode status = WebPGetFeatures(_data, _size, &features);

	if (status != VP8_STATUS_OK) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Load from memory failed, error %d", (int)status);
		return false;
	}

	_width = features.width;
	_height = features.height;
	*outWidth = _width;
	*outHeight = _height;
	return true;
}

// The picture is decoded into `picture` as RGBA, the colours multiplied by the alpha when the picture has the flag at +0x70.
static bool decodeInto(const unsigned char *data, int size, int width, int height, TPictureMEM &picture) {
	unsigned char *pixels = reinterpret_cast<unsigned char *>(picture.GetMemoryData());
	size_t needed = (size_t)width * height * 4;

	if (!pixels || (size_t)picture.GetMemorySize() < needed)
		return false;

	if (!WebPDecodeRGBAInto(data, size, pixels, needed, width * 4))
		return false;

	if (picture.IsField70()) {
		for (size_t i = 0; i < (size_t)width * height; i++) {
			unsigned char *p = pixels + i * 4;
			unsigned int alpha = p[3];

			p[0] = (unsigned char)(p[0] * alpha / 255);
			p[1] = (unsigned char)(p[1] * alpha / 255);
			p[2] = (unsigned char)(p[2] * alpha / 255);
		}
	}
	return true;
}

// Confirmed (asm lines 760905-761631)
bool TPictureWebP::ReadData(TFile &file, TPictureMEM &picture) {
	CleanUp();
	file.Seek(0, 0);
	_size = (int)file.GetFileLength();
	_data = new unsigned char[_size];
	file.ReadMem(_data, _size);

	if (file.IsPNGHeaderEncrypted())
		decryptHeader(file, _data + 0x10);

	WebPBitstreamFeatures features;
	VP8StatusCode status = WebPGetFeatures(_data, _size, &features);

	if (status != VP8_STATUS_OK) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"File %ls %ls has wrong format, error %d", file.GetOriginalPath().GetName().wc_str(),
			                   file.GetOriginalPath().GetFullName().wc_str(), (int)status);
		return false;
	}

	_width = features.width;
	_height = features.height;

	bool result = decodeInto(_data, _size, _width, _height, picture);

	CleanUp();
	return result;
}

// Confirmed (asm lines 759400-759457): decodes the data that the header (the memory form) was read from.
bool TPictureWebP::ReadData(TPictureMEM &picture) {
	if (!_data)
		return false;

	bool result = decodeInto(_data, _size, _width, _height, picture);

	CleanUp();
	return result;
}

// The writer of libwebp puts the bytes into the file (asm lines 759375-759399).
static int writeToFile(const uint8_t *data, size_t size, const WebPPicture *picture) {
	wxFile *file = static_cast<wxFile *>(picture->custom_ptr);

	file->Write(data, size);
	return (int)size;
}

// Confirmed (asm lines 759458-759859). The picture is encoded lossy (quality 85) from its BGRA pixels: a picture with 3 bytes to a
// pixel gets an opaque alpha, a flipped one (lines from the bottom) is turned, one in RGB order gets its red and blue changed.
bool TPictureWebP::Write(const wxFileName &path, TPictureMEM &picture, int width, int height, int bytesPerPixel, bool /*flag*/) {
	wxFile file;

	if (!file.Open(path.GetFullPath(), wxString(L"w"))) {
		if (wxLog::loglevel >= 0)
			wxLog::logexpanded(L"Can not open %ls for writing", path.GetFullPath().wc_str());
		return false;
	}

	WebPPicture webp;
	WebPConfig config;

	if (!WebPPictureInit(&webp) || !WebPConfigInit(&config))
		return false;

	config.quality = 85.0f;
	webp.writer = writeToFile;
	webp.custom_ptr = &file;
	webp.width = width;
	webp.height = height;

	const unsigned char *source = reinterpret_cast<const unsigned char *>(picture.GetMemoryData());
	unsigned char *expanded = nullptr;
	unsigned char *turned = nullptr;
	int pixels = width * height;

	if (picture.GetBytesPerPixel() == 3 || bytesPerPixel == 3) {
		// (3 bytes to a pixel: alpha 255)
		expanded = new unsigned char[(size_t)pixels * 4];
		for (int i = 0; i < pixels; i++) {
			expanded[i * 4] = source[i * 3];
			expanded[i * 4 + 1] = source[i * 3 + 1];
			expanded[i * 4 + 2] = source[i * 3 + 2];
			expanded[i * 4 + 3] = 0xFF;
		}
		source = expanded;
	} else if (picture.IsFlipped()) {
		turned = new unsigned char[(size_t)pixels * 4];
		for (int y = 0; y < height; y++)
			std::memcpy(turned + (size_t)y * width * 4, source + (size_t)(height - 1 - y) * width * 4, (size_t)width * 4);
		source = turned;
	}

	unsigned char *copy = new unsigned char[(size_t)pixels * 4];

	std::memcpy(copy, source, (size_t)pixels * 4);
	if (picture.IsRgbOrder()) {
		for (int i = 0; i < pixels; i++)
			std::swap(copy[i * 4], copy[i * 4 + 2]);
	}

	webp.use_argb = 1;
	webp.argb = reinterpret_cast<uint32_t *>(copy);
	webp.argb_stride = width;

	bool ok = WebPEncode(&config, &webp) != 0;

	// (the pixels belong to this function)
	webp.argb = nullptr;
	WebPPictureFree(&webp);
	file.Close();
	delete[] copy;
	delete[] expanded;
	delete[] turned;
	return ok;
}

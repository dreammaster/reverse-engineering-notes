// Not yet assert-confirmed to a specific file; stays at the top level.
//
// TPictureIO::GetFormat() picks one of these from the first four bytes of the file ("RIFF" -> WebP, "?PNG" -> PNG, whatever the
// first byte is) or, failing that, from the first three letters of the extension (PNG, WEB[P], JPG, GIF, PCX; any case).
//
// The base class is confirmed in full (Deponia_Linux.asm lines 771397-771454 and 739426-739476): a vtable pointer plus one int
// (a format code; PNG sets 3). A format reads the header (the size) and the pixels of a picture from a TFile (ReadHeader() and
// ReadData() of the first form) or from memory (the second form: `data`/`size`), and writes a picture (Write()). The decoders
// wrap the vendored libpng, jpgd, libwebp, giflib and the PCX reader (manifest/vendor_libraries.tsv); they are not
// reconstructed: a ScummVM port gives the formats its own image decoders (Image::PNGDecoder, Image::JPEGDecoder, ...). The
// stubs below fail, so no picture can be loaded yet.
//
// What a port's decoders must do besides decoding: a picture in a composed file may have the content flag "PNG header
// encrypted" (TFile::IsPNGHeaderEncrypted(), see the notes of baselib/file.h): then the width and height that the header
// holds - two big-endian longs, which the engine's own writer encrypts with TMemoryBuffer::Encrypt() under the name of the
// file (TFile::GetOriginalPath().GetName()) - have to be decrypted with TMemoryBuffer::Decrypt() before they are used. The
// PNG reader does it in its read callback for the bytes 0x10 to 0x17 of the file (userReadData(), asm lines 754615-754780),
// the WebP reader in ReadHeader() (asm lines 759993-760330) and ReadData() (asm lines 760905-761400).
#pragma once

class TPictureMEM;
class TFile;
class wxFileName;

/** The kinds of picture the engine reads and writes. */
enum class TPictureKind { kPNG, kWebP, kJPG, kGIF, kPCX };

class TPictureFormat {
public:
	TPictureFormat() = default;
	virtual ~TPictureFormat() = default;

	int GetFormat() const {
		return _format;
	}

	/** The size of the picture in `file`. */
	virtual bool ReadHeader(TFile &file, int *outWidth, int *outHeight) = 0;
	/** The size of the picture in `size` bytes at `data`. */
	virtual bool ReadHeader(char *data, unsigned long size, int *outWidth, int *outHeight) = 0;
	/** The pixels of the picture in `file` into `picture` (made by InitMemory(true): RGBA). */
	virtual bool ReadData(TFile &file, TPictureMEM &picture) = 0;
	/** The pixels of the picture whose header was read with the memory form. */
	virtual bool ReadData(TPictureMEM &picture) = 0;
	/** Writes `picture` (`width` x `height`, `bytesPerPixel` bytes to a pixel; `flag`: the picture is a loaded image) to a file. */
	virtual bool Write(const wxFileName &file, TPictureMEM &picture, int width, int height, int bytesPerPixel, bool flag) = 0;

protected:
	int _format = 0;
};

/** A format whose decoder is not reconstructed. */
class TPictureFormatStub : public TPictureFormat {
public:
	bool ReadHeader(TFile &, int *, int *) override {
		return false;
	}
	bool ReadHeader(char *, unsigned long, int *, int *) override {
		return false;
	}
	bool ReadData(TFile &, TPictureMEM &) override {
		return false;
	}
	bool ReadData(TPictureMEM &) override {
		return false;
	}
	bool Write(const wxFileName &, TPictureMEM &, int, int, int, bool) override {
		return false;
	}
};

class TPicturePNG : public TPictureFormatStub {
public:
	TPicturePNG() {
		_format = 3;
	}

	/** The PNG colour type of the header that was read (6: RGBA). */
	int GetColorType() const {
		return _colorType;
	}

private:
	int _colorType = 0;
};

/** WebP, read and written with libwebp (third_party/libwebp, TPictureWebP.cpp). Layout of the original: the format code
 *  (+0x08, 5), the size of the data (+0x0C), the picture's size (+0x10, +0x14) and the file data that was read (+0x18). */
class TPictureWebP : public TPictureFormat {
public:
	TPictureWebP();
	~TPictureWebP() override;

	/** Frees the data of the file. */
	bool CleanUp();

	bool ReadHeader(TFile &file, int *outWidth, int *outHeight) override;
	bool ReadHeader(char *data, unsigned long size, int *outWidth, int *outHeight) override;
	bool ReadData(TFile &file, TPictureMEM &picture) override;
	bool ReadData(TPictureMEM &picture) override;
	bool Write(const wxFileName &file, TPictureMEM &picture, int width, int height, int bytesPerPixel, bool flag) override;

private:
	int _size = 0;                      // +0x0C
	int _width = 0;                     // +0x10
	int _height = 0;                    // +0x14
	unsigned char *_data = nullptr;     // +0x18
};

class TPictureJPG : public TPictureFormatStub {
};

class TPictureGIF : public TPictureFormatStub {
};

class TPicturePCX : public TPictureFormatStub {
};

/** The port's decoders: when this is set, it is asked for the decoder of every picture the engine opens (or nullptr for
 *  the stub, which fails to read); the engine deletes what it is given. (In the ScummVM port it makes the decoders of the
 *  image classes of ScummVM; the tests use it to read pictures of their own.) */
extern TPictureFormat *(*g_createPictureFormat)(TPictureKind kind);

/** The decoder for `kind`: the port's, or the stub. */
TPictureFormat *CreatePictureFormat(TPictureKind kind);

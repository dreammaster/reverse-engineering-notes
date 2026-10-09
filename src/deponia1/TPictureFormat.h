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
#pragma once

class TPictureMEM;
class TFile;
class wxFileName;

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

class TPictureWebP : public TPictureFormatStub {
};

class TPictureJPG : public TPictureFormatStub {
};

class TPictureGIF : public TPictureFormatStub {
};

class TPicturePCX : public TPictureFormatStub {
};

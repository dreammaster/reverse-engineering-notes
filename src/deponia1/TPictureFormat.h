// Not yet assert-confirmed to a specific file; stays at the top level.
// TPictureIO::GetFormat picks one of these based on file magic bytes
// (checked first: "RIFF....WEBP" -> WebP, PNG signature -> PNG) or,
// failing that, a single-letter format-hint string ('P'->PNG, 'W'->WebP,
// 'J'->JPG, 'G'->GIF, and a final 'P' check that - per the disassembly -
// is only reachable after the first 'P' check already failed, so it can
// seemingly never match; reproduced as observed rather than "fixed" - see
// NOTES.md).
//
// The base class itself is confirmed in full (Deponia_Linux.asm lines
// 771397-771454, all 4 of its own methods: ctor/dtor x2/GetFormat) - a
// vtable pointer plus one int (a format code, confirmed read directly by
// GetFormat(); default-constructed to 0, but no confirmed evidence ties any
// particular nonzero value to a specific format, so subclasses don't set
// one here). ReadHeader()/ReadData()/Write() are genuinely abstract in the
// original (only ever called through subclass overrides, never given a
// body of their own) - declared pure virtual here with the confirmed
// signatures (asm lines 739426-739476) so a future pass has the right
// shape to implement against, but none of the actual decoders are reversed:
// they wrap the real vendored libpng/libwebp/jpgd libraries (manifest/
// vendor_libraries.tsv) which should be linked directly rather than
// reimplemented - the stub bodies below just return failure so existing
// callers (TPictureIO::GetFormat()) keep compiling against real instances.
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

	virtual bool ReadHeader(char *data, unsigned long size, int *outWidth, int *outHeight) = 0;
	virtual bool ReadData(TPictureMEM &picture) = 0;
	virtual bool Write(const wxFileName &file, TPictureMEM &picture, int a, int b, int c, bool d) = 0;

protected:
	int _format = 0;
};

class TPicturePNG : public TPictureFormat {
public:
	bool ReadHeader(char */*data*/, unsigned long /*size*/, int */*outWidth*/, int */*outHeight*/) override {
		return false;
	}
	bool ReadData(TPictureMEM &/*picture*/) override {
		return false;
	}
	bool Write(const wxFileName &/*file*/, TPictureMEM &/*picture*/, int /*a*/, int /*b*/, int /*c*/,
	           bool /*d*/) override {
		return false;
	}
};

class TPictureWebP : public TPictureFormat {
public:
	bool ReadHeader(char */*data*/, unsigned long /*size*/, int */*outWidth*/, int */*outHeight*/) override {
		return false;
	}
	bool ReadData(TPictureMEM &/*picture*/) override {
		return false;
	}
	bool Write(const wxFileName &/*file*/, TPictureMEM &/*picture*/, int /*a*/, int /*b*/, int /*c*/,
	           bool /*d*/) override {
		return false;
	}
};

class TPictureJPG : public TPictureFormat {
public:
	bool ReadHeader(char */*data*/, unsigned long /*size*/, int */*outWidth*/, int */*outHeight*/) override {
		return false;
	}
	bool ReadData(TPictureMEM &/*picture*/) override {
		return false;
	}
	bool Write(const wxFileName &/*file*/, TPictureMEM &/*picture*/, int /*a*/, int /*b*/, int /*c*/,
	           bool /*d*/) override {
		return false;
	}
};

class TPictureGIF : public TPictureFormat {
public:
	bool ReadHeader(char */*data*/, unsigned long /*size*/, int */*outWidth*/, int */*outHeight*/) override {
		return false;
	}
	bool ReadData(TPictureMEM &/*picture*/) override {
		return false;
	}
	bool Write(const wxFileName &/*file*/, TPictureMEM &/*picture*/, int /*a*/, int /*b*/, int /*c*/,
	           bool /*d*/) override {
		return false;
	}
};

class TPicturePCX : public TPictureFormat {
public:
	bool ReadHeader(char */*data*/, unsigned long /*size*/, int */*outWidth*/, int */*outHeight*/) override {
		return false;
	}
	bool ReadData(TPictureMEM &/*picture*/) override {
		return false;
	}
	bool Write(const wxFileName &/*file*/, TPictureMEM &/*picture*/, int /*a*/, int /*b*/, int /*c*/,
	           bool /*d*/) override {
		return false;
	}
};

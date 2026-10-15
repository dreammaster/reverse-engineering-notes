// A picture format for the tests, so that pictures have a size: a file of "FAKE <width> <height>" is a picture of that
// size with opaque grey pixels. (The real decoders are the port's: see TPictureFormat.h.)
#pragma once

#include <cstdio>
#include <cstring>
#include <string>
#include <sys/stat.h>

#include "baselib/file.h"
#include "TPictureFormat.h"
#include "TPictureMEM.h"

class TFakePicture : public TPictureFormat {
public:
	bool ReadHeader(TFile &file, int *width, int *height) override {
		char text[64];

		file.Seek(0, 0);
		std::memset(text, 0, sizeof(text));
		for (size_t i = 0; i < sizeof(text) - 1 && file.ReadByte(text[i]); i++) {
		}
		return parse(text, width, height);
	}

	bool ReadHeader(char *data, unsigned long size, int *width, int *height) override {
		std::string text(data, size > 63 ? 63 : size);

		return parse(text.c_str(), width, height);
	}

	bool ReadData(TFile &file, TPictureMEM &picture) override {
		file.Seek(0, 0);
		return fill(picture);
	}

	bool ReadData(TPictureMEM &picture) override {
		return fill(picture);
	}

	bool Write(const wxFileName &, TPictureMEM &, int, int, int, bool) override {
		return false;
	}

private:
	static bool parse(const char *text, int *width, int *height) {
		return std::sscanf(text, "FAKE %d %d", width, height) == 2;
	}

	static bool fill(TPictureMEM &picture) {
		for (int y = 0; y < picture.GetMemHeight(); y++) {
			char *line = picture.GetLine(y);

			if (!line)
				return false;

			for (int x = 0; x < picture.GetMemWidth(); x++) {
				line[x * 4] = (char)0x80;
				line[x * 4 + 1] = (char)0x80;
				line[x * 4 + 2] = (char)0x80;
				line[x * 4 + 3] = (char)0xFF;
			}
		}

		return true;
	}
};

inline TPictureFormat *createFakePicture(TPictureKind) {
	return new TFakePicture();
}

/** Writes the file of a picture of `width` x `height` (the directory of it has to be there). */
inline void writeFakePicture(const char *path, int width, int height) {
	std::FILE *file = std::fopen(path, "wb");

	if (file) {
		std::fprintf(file, "FAKE %d %d", width, height);
		std::fclose(file);
	}
}

/** Makes the picture formats of the engine read the pictures above. */
inline void useFakePictures() {
	g_createPictureFormat = createFakePicture;
	::mkdir("pics");
}

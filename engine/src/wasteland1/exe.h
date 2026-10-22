#pragma once
// WL.EXE loader: unpacks the Microsoft EXEPACK compressed program image (load module only).
#include "wasteland1/codec.h"

namespace wl {

struct ExeImage {
	Bytes image;                  // unpacked load module (dest_len paragraphs), segment fixups applied for `loadSegment`
	uint16_t entryIp = 0, entryCs = 0, initialSp = 0, initialSs = 0;
};

// loadSegment: segment the image is assumed to be loaded at (relocations add it to every fixed-up word).
ExeImage loadExePack(const Bytes &exeFile, uint16_t loadSegment);

} // namespace wl

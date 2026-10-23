/* Running a program that tools/asm2cpp.py translated: a Machine16 is set up with the parameter block the original's parent would give
 * (the magic word, the video context's segment and the inherited file handles) and the generated class's run() is called. */

#ifndef YENDOR1_SHARED_GENERATEDPROGRAM_H
#define YENDOR1_SHARED_GENERATEDPROGRAM_H

#include "shared/Launcher.h"
#include "shared/Machine16.h"

namespace Yendor1 {
namespace Shared {

/** The inherited handles are numbered by the offset of their field in the parameter block (see Machine16.cpp). */
inline void fillInheritedParams(Params &p) {
	static const uint8_t offsets[] = {0x5C, 0x5E, 0x60, 0x62, 0x64, 0x66, 0x68, 0x6A, 0x70, 0x72, 0x74, 0x76, 0x78, 0x7A};
	p.raw[0x58] = 0x34;
	p.raw[0x59] = 0x12;
	p.raw[0x5A] = (uint8_t)Machine16::kSegCtx;
	p.raw[0x5B] = (uint8_t)(Machine16::kSegCtx >> 8);
	for (size_t i = 0; i < sizeof(offsets); i++) {
		p.raw[offsets[i]] = offsets[i];
		p.raw[offsets[i] + 1] = 0;
	}
}

template <class Gen>
int runGenerated(Runtime &rt, Launcher *launcher, const Params &params, const char *exeName) {
	Params p = params;
	fillInheritedParams(p);
	Machine16 machine(rt, launcher, p);
	if (!machine.loadProgram(exeName, Gen::kDataSegmentImageOffset))
		return 0xFE;
	machine.tileFileOffset = Gen::kTileFile;
	machine.tileIndexOffset = Gen::kTileIndex;
	machine.tileStatusOffset = Gen::kTileStatus;
	Gen program(machine);
	return program.run();
}

} // namespace Shared
} // namespace Yendor1

#endif

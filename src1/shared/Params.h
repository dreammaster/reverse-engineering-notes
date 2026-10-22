/* The parameter block SW.EXE hands to every program in the DOS command tail (127 bytes at PSP:0080h; see
 * docs1/program-graph.md). In this port a program receives it as a plain struct; the magic word, the video context segment
 * and the inherited file handles at +58h.. do not exist here. */

#ifndef YENDOR1_SHARED_PARAMS_H
#define YENDOR1_SHARED_PARAMS_H

#include "shared/Types.h"

namespace Yendor1 {
namespace Shared {

struct Params {
	byte raw[128];

	Params() { for (int i = 0; i < 128; i++) raw[i] = 0; }

	/** Offsets are the original's `parm_XX` names (XX = offset in the block). */
	byte parm(int offset) const { return raw[offset]; }
	void setParm(int offset, byte v) { raw[offset] = v; }
};

} // namespace Shared
} // namespace Yendor1

#endif

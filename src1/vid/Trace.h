/* One-line text form of a driver call and the context fields it uses; the same format yemu's --log62 writes for the original,
 * so a reimplemented program's calls can be diffed against the real thing. */

#ifndef YENDOR1_VID_TRACE_H
#define YENDOR1_VID_TRACE_H

#include "vid/Video.h"

#include <string>

namespace Yendor1 {
namespace Vid {

/** "AH AL x1=.. y1=.. ... img=HASH" (image hash: FNV-1a of the planar data for AH=0Ah and 0Ch, else 0). */
std::string formatCall(const Video &video, int ah, int al);

} // namespace Vid
} // namespace Yendor1

#endif

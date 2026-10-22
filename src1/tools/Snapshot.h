/* Loader for the machine snapshots yemu --dump writes at every program start and end (*.ydmp): the parameter block, the video
 * driver's context block, the cursor store and the EGA planes. Development/test aid, not part of the engine. */

#ifndef YENDOR1_TOOLS_SNAPSHOT_H
#define YENDOR1_TOOLS_SNAPSHOT_H

#include "shared/Blob.h"
#include "shared/Params.h"
#include "vid/Video.h"

#include <string.h>

namespace Yendor1 {
namespace Tools {

struct Snapshot {
	Shared::Params params;
	Shared::byte ctx[0x980];
	Shared::byte cursor[Vid::kCursorSlots * Vid::kCursorSlotBytes];
	Shared::byte planes[4][65536];
	int displayedPage;

	bool load(const char *path) {
		Shared::Blob b;
		if (!b.load(path))
			return false;
		const size_t need = 4 + 4 + 128 + 0x980 + sizeof(cursor) + 4 * 65536 + 1;
		if (b.size() < need || memcmp(b.data(), "YDMP", 4) != 0)
			return false;
		const Shared::byte *p = b.data() + 8;
		memcpy(params.raw, p, 128); p += 128;
		memcpy(ctx, p, 0x980); p += 0x980;
		memcpy(cursor, p, sizeof(cursor)); p += sizeof(cursor);
		for (int i = 0; i < 4; i++) { memcpy(planes[i], p, 65536); p += 65536; }
		displayedPage = *p;
		return true;
	}

	/** Put a Video into this state. */
	void apply(Vid::Video &v) const {
		v.loadContext(ctx);
		for (int i = 0; i < 4; i++)
			v.loadPlane(i, planes[i]);
		memcpy(v.cursorStore(), cursor, sizeof(cursor));
		v.setDisplayedPage(displayedPage);
	}
};

} // namespace Tools
} // namespace Yendor1

#endif

#include "vid/Fonts.h"

namespace Yendor1 {
namespace Vid {

bool Fonts::load(const char *dir) {
	char path[512];
	bool ok = true;
	if (!Shared::joinPath(path, sizeof(path), dir, "FONT8X8.BIN") || !_f8.load(path) || !have8x8()) {
		_f8.clear();
		ok = false;
	}
	if (!Shared::joinPath(path, sizeof(path), dir, "FONT8X14.BIN") || !_f14.load(path) || !have8x14()) {
		_f14.clear();
		ok = false;
	}
	return ok;
}

void Fonts::attach(Video &video) const {
	video.setFont8x8(have8x8() ? _f8.data() : 0);
	video.setFont8x14(have8x14() ? _f14.data() : 0);
}

} // namespace Vid
} // namespace Yendor1

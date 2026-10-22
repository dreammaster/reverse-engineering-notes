/* Lockstep check of Yendor1::Vid against the real VID16.DRV running in the emulator: at every INT 62h the emulator's state
 * (EGA planes, context block, image memory) is copied into a Vid::Video, the call is made on both, and the results are compared. */

#ifndef YEMU_COMPARE_H
#define YEMU_COMPARE_H

#include "Machine.h"
#include "vid/Fonts.h"
#include "vid/Video.h"

#include <map>
#include <string>
#include <vector>

namespace Yemu {

class VidCompare {
public:
	VidCompare();
	void attach(Machine &m, const std::string &fontsDir);
	void report(FILE *out) const;
	int mismatches() const { return _mismatchTotal; }

private:
	bool onCall(Machine &m);
	std::string diffPlanes(Machine &m, const Yendor1::Vid::Video &v);

	Yendor1::Vid::Fonts _fonts;
	Yendor1::Vid::Video *_video;
	std::map<int, int> _calls, _bad;
	std::vector<std::string> _details;
	int _mismatchTotal;
	uint16_t _ctxSeg;
	std::vector<uint8_t> _shadow;
};

} // namespace Yemu

#endif

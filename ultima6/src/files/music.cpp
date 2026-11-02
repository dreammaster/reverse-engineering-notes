#include "files/music.h"

#include "files/lzw.h"

namespace Ultima6 {

bool MusicTrack::load(const Bytes &lzwFile) {
	events.clear();
	totalDelay = 0;
	if (!lzwDecompress(lzwFile, 0, data))
		return false;

	size_t i = 0, n = data.size();
	while (i < n) {
		Event e;
		e.offset = i;
		int b = data[i++];
		e.command = b >> 4;
		e.channel = b & 15;
		e.dataOffset = i;
		e.dataLength = 0;
		if (e.command <= PROGRAM) {
			e.dataLength = 1;
		} else if (e.command == SUBCOMMAND) {
			switch (e.channel) {
			case CALL:
				e.dataLength = 3;
				break;
			case DELAY:
				e.dataLength = 1;
				break;
			case LOAD_INSTRUMENT:
				e.dataLength = 12;
				break;
			case FADE_OUT:
			case FADE_IN:
				e.dataLength = 1;
				break;
			default:
				break;
			}
		} else if (e.command != LOOP_POINT && e.command != RETURN) {
			return false;
		}
		if (i + e.dataLength > n)
			return false;
		if (e.command == SUBCOMMAND && e.channel == DELAY)
			totalDelay += data[i];
		if (e.command == SUBCOMMAND && e.channel == CALL) {
			size_t target = data[i + 1] | (data[i + 2] << 8);
			if (target >= n)
				return false;
		}
		i += e.dataLength;
		events.push_back(e);
	}
	return !events.empty();
}

} // namespace Ultima6

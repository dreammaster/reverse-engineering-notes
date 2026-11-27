#ifndef ULTIMA6_ULTIMA6_SE_TITLE_H
#define ULTIMA6_ULTIMA6_SE_TITLE_H

#include "engine/cpu8086.h"
#include "files/data_file.h"

#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * The title sequence of Savage Empire (SAVAGE.EXE's title_sequence, sub_16FCD: the crowd and the planet, the pieces of the Origin FX logo that fly together, the logo that grows, the sparks),
 * run as the original's own code. SAVAGE.EXE is loaded into the megabyte of Cpu8086 (relocated, its data segment as at the start of the program), the drawing drivers TM.DRV and SUPERTM.DRV are
 * loaded as the game loads them (their tables of entries are copied into the far pointers dword_20666 ...), and title_sequence is called with the arguments main gives it. What lies outside it is
 * done by the host here, at the addresses of the original's routines: loading an item of an *.LZC (load_lzc_item), the music, the keys, the waits (wait_retrace and delay_counts give the time that
 * the sequence takes: tick() lets it run as far as the clock allows), the heap and the sound effects. The screen is the driver's: pixels() are the 320 x 200 of the front screen and palette() the
 * VGA's colour registers as the driver's out instructions set them.
 *
 * APPROX(105): see docs/approximations.md: the code is run, not ported; the sound effects are left out.
 */
class SeTitle {
public:
	/** `music`: the music flag word_20FDE (the sequence then waits for the cues of the song: see cue()). */
	SeTitle(const std::string &dir, bool music, uint32_t seed = 1);

	bool ok() const {
		return _ok;
	}
	bool finished() const {
		return _finished;
	}
	/** Time goes by (ms of the original's clock). */
	void tick(unsigned ms);
	/** A key was pressed: the sequence ends as soon as it looks. */
	void key() {
		_key = true;
	}
	/** The song has reached its cue number n (word_1FB6E): the sequence goes on to what waits for it. */
	void cue(int n);
	const uint8_t *pixels() const;
	/** The 256 colour registers, 6 bits a component. */
	const uint8_t (*palette() const)[3] {
		return _dac;
	}
	/** True when the sequence started a song (the caller plays item 0x13 of MUSIC.LZC) / stopped it (fade out over `stopTicks`). */
	bool takeMusicStart();
	int takeMusicStop();
	/** The error that stopped the run, if any (a tool of the emulation: an instruction not known). */
	const Cpu8086 &cpu() const {
		return *_cpu;
	}
	const char *error() const {
		return _error.c_str();
	}
	uint64_t steps() const {
		return _steps;
	}

private:
	void setup();
	bool loadExe();
	bool loadDriver(const char *name, uint16_t seg, const uint16_t *table);
	void hle(Cpu8086 &c);
	void ret(Cpu8086 &c, uint16_t ax, uint16_t dx = 0);
	uint16_t arg(Cpu8086 &c, int n) const;
	void runSlice();
	uint16_t dsw(uint16_t off) const;
	uint8_t dsb(uint16_t off) const;
	void setDsw(uint16_t off, uint16_t v);
	int itemSegment(const std::string &file, int item);

	std::string _dir, _error;
	bool _ok = false, _finished = false, _music = false, _key = false;
	uint32_t _seed;
	std::unique_ptr<Cpu8086> _cpu;
	std::map<uint32_t, int> _hle; // linear address -> routine
	double _clock = 0, _wakeAt = 0;
	uint64_t _steps = 0;
	uint16_t _nextSeg, _heap;
	std::map<std::string, int> _items; // "FILE:item" -> segment
	// the VGA's colour registers
	uint8_t _dac[256][3];
	int _dacWrite = 0, _dacRead = 0, _dacPhase = 0, _dacReadPhase = 0;
	bool _musicStart = false;
	int _musicStop = -1;
	int _cues = 0;
};

} // namespace Ultima6
} // namespace Ultima6

#endif

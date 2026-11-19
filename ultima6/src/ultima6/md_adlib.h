#ifndef ULTIMA6_ULTIMA6_MD_ADLIB_H
#define ULTIMA6_ULTIMA6_MD_ADLIB_H

#include "engine/cpu8086.h"
#include "engine/opl2.h"
#include "files/data_file.h"

#include <memory>
#include <string>
#include <vector>

namespace Ultima6 {
namespace Ultima6 {

/**
 * The AdLib sound driver of Martian Dreams, "Origin Sound System" (item 1 of STRAX.DRV, with the timbres of item 1 of MD.TIM): the game's sequencer (md_music.h) hands it the events of a MIDI
 * file and its timer interrupt (60 times a second) turns the voices' envelopes and slides. The driver is machine code of the game's own files; it is run on Cpu8086 and what it writes to the
 * AdLib ports (0x388, 0x389) goes to the chip model. Its entries (the words at the head of the block, as MARTIAN.EXE's sub_21956 reads them): note(velocity, note, channel), controller(value,
 * controller, channel), pitch bend(second byte, first byte, channel), program(program, channel), reset and the timer interrupt. Channels are the MIDI channel + 1.
 *
 * APPROX(94): the code is run, not ported; the waits of the driver between two writes to the chip (it reads the timer chip's counter) are answered by a counter that falls 40 a read.
 */
class MdAdlib {
public:
	MdAdlib();

	/** Reads STRAX.DRV and MD.TIM of the game's directory and starts the driver. */
	bool load(const std::string &dir);
	bool ok() const {
		return _ok;
	}
	/** What stopped the driver the last time it stopped (nullptr if it never did). */
	const char *error() const {
		return _error;
	}

	void note(int channel, int note, int velocity);
	void controller(int channel, int controller, int value);
	void pitchBend(int channel, int first, int second);
	void program(int channel, int program);
	void reset();
	/** The driver's timer interrupt: one tick (60 a second). */
	void tick();

	/** The processor, for tracing. */
	Cpu8086 &cpu() {
		return *_cpu;
	}
	Opl2 &chip() {
		return _chip;
	}
	/** Every write to the chip since the last clearLog() (register, value), for tests. */
	const std::vector<std::pair<int, int>> &log() const {
		return _log;
	}
	void clearLog() {
		_log.clear();
	}

private:
	bool call(uint16_t entry, const uint16_t *args, int count);

	std::unique_ptr<Cpu8086> _cpu;
	Opl2 _chip;
	bool _ok = false;
	const char *_error = nullptr;
	uint16_t _entry[10];
	int _oplRegister = 0;
	int _pit = 0x1000;
	std::vector<std::pair<int, int>> _log;
};

} // namespace Ultima6
} // namespace Ultima6

#endif

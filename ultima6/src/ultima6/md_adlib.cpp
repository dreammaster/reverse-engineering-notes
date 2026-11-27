#include "ultima6/md_adlib.h"

#include "files/lib_n.h"

#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

namespace {
const uint16_t CODE_SEG = 0x2000;   // the driver's block (its data are in the code segment)
const uint16_t TIMBRE_SEG = 0x4000; // the timbres of MD.TIM
const uint16_t STACK_SEG = 0x1000;
const uint16_t RETURN_SEG = 0x9000;
const uint16_t CHAIN_SEG = 0x9100;
const uint16_t PORT = 0x388;
} // namespace

MdAdlib::MdAdlib() : _cpu(new Cpu8086()) {
	std::memset(_entry, 0, sizeof _entry);
}

bool MdAdlib::load(const std::string &dir) {
	_ok = false;
	Bytes raw, block, tim, timbres;
	LibN lib, timLib;
	if (!readFile(joinPath(dir, "STRAX.DRV"), raw) || !lib.load(raw, true) || !lib.get(1, block) || block.size() < 0x30 || block.size() > 0xF000)
		return false;
	// the timbres: MD.TIM of Martian Dreams, SAVAGE.TIM of Savage Empire (the same driver, STRAX.DRV, and the same layout)
	if ((!readFile(joinPath(dir, "MD.TIM"), tim) && !readFile(joinPath(dir, "SAVAGE.TIM"), tim)) || !timLib.load(tim, true) || !timLib.get(1, timbres) || timbres.size() > 0xF000)
		return false;
	Cpu8086 &c = *_cpu;
	std::memcpy(c.memory() + Cpu8086::linear(CODE_SEG, 0), block.data(), block.size());
	std::memcpy(c.memory() + Cpu8086::linear(TIMBRE_SEG, 0), timbres.data(), timbres.size());
	c.write8(RETURN_SEG, 0, 0xF4);
	// the timer interrupt chains to the vector it replaced (dword_16: the game's installer, sub_1E226, writes it there): here a handler that returns at once
	c.write8(CHAIN_SEG, 0, 0xCF);
	c.write16(CODE_SEG, 0x16, 0);
	c.write16(CODE_SEG, 0x18, CHAIN_SEG);
	for (int i = 0; i < 10; ++i)
		_entry[i] = c.read16(CODE_SEG, (uint16_t)(2 + 2 * i));
	// the ports: the chip, and the timer chip's counter the driver reads to wait between two writes
	c.onOut = [this](uint16_t port, uint16_t value, bool) {
		if (port == PORT) {
			_oplRegister = value & 0xFF;
		} else if (port == PORT + 1) {
			_chip.write(_oplRegister, value & 0xFF);
			_log.push_back(std::make_pair(_oplRegister, (int)(value & 0xFF)));
		}
	};
	bool high = false;
	c.onIn = [this, high](uint16_t port, bool) mutable -> uint16_t {
		if (port != 0x40)
			return 0;
		uint16_t v = (uint16_t)(high ? (_pit >> 8) : _pit);
		if (high) {
			_pit -= 40;
			if (_pit < 0)
				_pit += 0x4DAE;
		}
		high = !high;
		return v & 0xFF;
	};
	_chip.reset();
	// the game: init (answers where its table is), load the timbres, the card's port, reset
	call(_entry[0], nullptr, 0);
	const uint16_t load[2] = {TIMBRE_SEG, 0}; // the segment is pushed first
	call(_entry[1], load, 2);
	const uint16_t port[1] = {PORT};
	call(_entry[8], port, 1);
	call(_entry[2], nullptr, 0);
	_ok = true;
	return true;
}

// a far call of an entry with the arguments pushed in the order given (the entry pops them itself)
bool MdAdlib::call(uint16_t entry, const uint16_t *args, int count) {
	Cpu8086 &c = *_cpu;
	c.ss = STACK_SEG;
	c.ds = CODE_SEG;
	c.es = CODE_SEG;
	c.sp = 0xF000;
	for (int i = 0; i < count; ++i)
		c.push16(args[i]);
	c.push16(RETURN_SEG);
	c.push16(0);
	c.cs = CODE_SEG;
	c.ip = entry;
	bool ok = c.run(Cpu8086::linear(RETURN_SEG, 0), 5000000);
	if (!ok)
		_error = c.error() ? c.error() : "stopped";
	return ok;
}

void MdAdlib::note(int channel, int n, int velocity) {
	const uint16_t a[3] = {(uint16_t)channel, (uint16_t)n, (uint16_t)velocity};
	call(_entry[4], a, 3);
}

void MdAdlib::controller(int channel, int ctrl, int value) {
	const uint16_t a[3] = {(uint16_t)channel, (uint16_t)ctrl, (uint16_t)value};
	call(_entry[5], a, 3);
}

void MdAdlib::pitchBend(int channel, int first, int second) {
	const uint16_t a[3] = {(uint16_t)channel, (uint16_t)first, (uint16_t)second};
	call(_entry[6], a, 3);
}

void MdAdlib::program(int channel, int prog) {
	const uint16_t a[2] = {(uint16_t)channel, (uint16_t)prog};
	call(_entry[7], a, 2);
}

void MdAdlib::reset() {
	call(_entry[2], nullptr, 0);
}

// the timer interrupt: an interrupt frame (flags, return address) and the entry
void MdAdlib::tick() {
	Cpu8086 &c = *_cpu;
	c.ss = STACK_SEG;
	c.sp = 0xF000;
	c.push16(0x0202);
	c.push16(RETURN_SEG);
	c.push16(0);
	c.cs = CODE_SEG;
	c.ip = _entry[3];
	c.run(Cpu8086::linear(RETURN_SEG, 0), 5000000);
}

} // namespace Ultima6
} // namespace Ultima6

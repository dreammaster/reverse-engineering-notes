#pragma once
// Native image of the memory WL.EXE works on: the 64 KB data segment (DS = 1CE2h), the 57 KB graphics buffer segment
// (2AE2h) and the EGA planes. Ports of the original routines operate on this image with the same offsets as the
// original, so a state captured from the wlemu oracle can be loaded, a ported routine run, and the result compared
// byte for byte with what the original code produced.
#include "wl/ega.h"
#include "fod/bytes.h"
#include <deque>
#include <functional>
#include <string>
#include <vector>

namespace wl {

using fod::Bytes;

constexpr uint32_t kDsLinear = 0x1CE20;      // DS = 1CE2h
constexpr uint32_t kGfxLinear = 0x2AE20;     // seg003 (graphics work buffer, segment 2AE2h)
constexpr size_t kDsSize = 0xE000;
constexpr size_t kGfxSize = 0x39000 - 0x2AE20;

struct Machine {
	Bytes mem = Bytes(1 << 20, 0);
	Ega ega;
	uint16_t regs[14] = {};                  // ax cx dx bx sp bp si di es cs ss ds ip flags (as saved by wlemu)

	// BIOS-level inputs the ported routines poll (not part of the state file)
	std::deque<uint16_t> keys;              // keyboard buffer: scancode << 8 | ascii
	std::function<void()> onIdle;           // called while a routine waits for input or time (lets the front end run)
	std::function<void()> onPortraitTick;   // the location portrait animation step (WL.EXE 190F8), called from input/scroll loops

	// data segment accessors (offset relative to DS)
	uint8_t &d8(uint16_t off) { return mem[kDsLinear + off]; }
	uint16_t d16(uint16_t off) const { return uint16_t(mem[kDsLinear + off] | (mem[kDsLinear + off + 1] << 8)); }
	void setD16(uint16_t off, uint16_t v) { mem[kDsLinear + off] = uint8_t(v); mem[kDsLinear + off + 1] = uint8_t(v >> 8); }
	uint8_t d8(uint16_t off) const { return mem[kDsLinear + off]; }

	bool loadState(const std::string &path);   // wlemu "save" file
	// Differences against another state: data segment, graphics buffer and the four planes. Prints at most `limit` lines.
	size_t diff(const Machine &other, size_t limit, bool verbose = true) const;
};

} // namespace wl

#pragma once
// Native image of the memory WL.EXE works on: the 64 KB data segment (DS = 1CE2h), the 57 KB graphics buffer segment
// (2AE2h) and the EGA planes. Ports of the original routines operate on this image with the same offsets as the
// original, so a state captured from the wlemu oracle can be loaded, a ported routine run, and the result compared
// byte for byte with what the original code produced.
#include "wasteland1/ega.h"
#include "shared/bytes.h"
#include <deque>
#include <functional>
#include <memory>
#include <string>
#include <vector>

namespace wl {

namespace rc { struct Sys; }

using shared::Bytes;

constexpr uint32_t kDsLinear = 0x1CE20;      // DS = 1CE2h
constexpr uint32_t kGfxLinear = 0x2AE20;     // seg003 (graphics work buffer, segment 2AE2h)
constexpr size_t kDsSize = 0xE000;
constexpr size_t kGfxSize = 0x39000 - 0x2AE20;

// Registers for calling routines of the original program that have not been ported yet (Machine::callLegacy).
struct X86Regs {
	uint8_t al = 0, ah = 0, bl = 0, bh = 0, cl = 0, ch = 0, dl = 0, dh = 0;
	uint16_t si = 0, di = 0, bp = 0;
	bool cf = false, zf = false, sf = false;
	uint16_t ax() const { return uint16_t(al | (ah << 8)); }
	uint16_t bx() const { return uint16_t(bl | (bh << 8)); }
	uint16_t cx() const { return uint16_t(cl | (ch << 8)); }
	uint16_t dx() const { return uint16_t(dl | (dh << 8)); }
	void setAx(uint16_t v) { al = uint8_t(v); ah = uint8_t(v >> 8); }
	void setBx(uint16_t v) { bl = uint8_t(v); bh = uint8_t(v >> 8); }
	void setCx(uint16_t v) { cl = uint8_t(v); ch = uint8_t(v >> 8); }
	void setDx(uint16_t v) { dl = uint8_t(v); dh = uint8_t(v >> 8); }
};

struct Machine {
	Bytes mem = Bytes(1 << 20, 0);
	Ega ega;
	uint16_t regs[14] = {};                  // ax cx dx bx sp bp si di es cs ss ds ip flags (as saved by wlemu)

	// BIOS-level inputs the ported routines poll (not part of the state file)
	std::deque<uint16_t> keys;              // keyboard buffer: scancode << 8 | ascii
	std::function<void()> onIdle;           // called while a routine waits for input or time (lets the front end run)
	std::shared_ptr<rc::Sys> sys;           // DOS/BIOS state of the recompiled program (see recomp_rt.h)
	std::function<void(uint16_t, X86Regs &)> callLegacy;   // runs the original routine at the given offset of the code segment
	std::function<uint8_t()> onPortraitTick;   // the location portrait animation step (WL.EXE 190F8, returns its al), called from input/scroll loops

	// data segment accessors (offset relative to DS)
	uint8_t &d8(uint16_t off) { return mem[kDsLinear + off]; }
	uint16_t d16(uint16_t off) const { return uint16_t(mem[kDsLinear + off] | (mem[kDsLinear + off + 1] << 8)); }
	void setD16(uint16_t off, uint16_t v) { mem[kDsLinear + off] = uint8_t(v); mem[kDsLinear + off + 1] = uint8_t(v >> 8); }
	uint8_t d8(uint16_t off) const { return mem[kDsLinear + off]; }

	bool loadState(const std::string &path);   // wlemu "save" file
	bool saveState(const std::string &path) const;   // same layout (registers, 1 MB of memory, EGA)
	// Differences against another state: data segment, graphics buffer and the four planes. Prints at most `limit` lines.
	size_t diff(const Machine &other, size_t limit, bool verbose = true) const;
};

} // namespace wl

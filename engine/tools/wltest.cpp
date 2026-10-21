// wltest: differential tests of the native Wasteland routines against the original code running in the 8086 oracle.
//
//   wltest <image.bin> <gamedir> <state.bin> <test> [args...]
// The state file comes from `wlemu ... save`. Each test pokes some globals, runs the original routine in the interpreter
// and the native port on a copy of the same state, then compares data segment, graphics buffer and EGA planes.
#include "emu.h"
#include "wl/machine.h"
#include "wl/ui.h"
#include "wl/globals.h"
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <map>

namespace {

struct Args {
	std::vector<std::string> v;
	unsigned hex(size_t i, unsigned def = 0) const { return i < v.size() ? unsigned(std::strtoul(v[i].c_str(), nullptr, 16)) : def; }
};

struct Regs { uint16_t ax = 0, bx = 0, cx = 0, dx = 0, si = 0, di = 0; };

struct TestCase {
	uint16_t entry;                                                              // near address in the code segment (offset from 10000h)
	std::function<Regs(wl::Machine &, const Args &)> setup;                     // pokes the shared start state, returns register inputs
	std::function<void(wl::Machine &, const Args &, const Regs &)> native;      // the port
};

std::map<std::string, TestCase> &registry() {
	static std::map<std::string, TestCase> r;
	return r;
}

void registerTests() {
	using namespace wl;
	// writeCharacterAt <char> [x y inverse]
	registry()["writeCharacterAt"] = {0x060C,
	    [](Machine &m, const Args &a) {
		    if (a.v.size() > 2) { m.d8(g::rectTextX) = uint8_t(a.hex(1)); m.d8(g::rectTextY) = uint8_t(a.hex(2)); }
		    if (a.v.size() > 3) m.d8(g::inverseText) = uint8_t(a.hex(3));
		    Regs r; r.ax = uint16_t(a.hex(0)); return r;
	    },
	    [](Machine &m, const Args &, const Regs &r) { writeCharacterAt(m, uint8_t(r.ax)); }};
	// writeColorCharacter <glyph> [x y]
	registry()["writeColorCharacter"] = {0x0CB6,
	    [](Machine &m, const Args &a) {
		    if (a.v.size() > 2) { m.d8(g::stringXPos) = uint8_t(a.hex(1)); m.d8(g::stringYPos) = uint8_t(a.hex(2)); }
		    Regs r; r.ax = uint16_t(a.hex(0)); return r;
	    },
	    [](Machine &m, const Args &, const Regs &r) { writeColorCharacter(m, uint8_t(r.ax)); }};
	// clearRect <leftCol> <rightCol> <topLine> <bottomLine>
	registry()["clearRect"] = {0x0762,
	    [](Machine &m, const Args &a) {
		    m.d8(g::rectLeftCol) = uint8_t(a.hex(0)); m.d8(g::rectRightCol) = uint8_t(a.hex(1));
		    m.d8(g::rectTopLine) = uint8_t(a.hex(2)); m.d8(g::rectBottomLine) = uint8_t(a.hex(3));
		    return Regs{};
	    },
	    [](Machine &m, const Args &, const Regs &) { clearRect(m); }};
}

} // namespace

int main(int argc, char **argv) {
	if (argc < 5) { std::fprintf(stderr, "usage: wltest <image.bin> <gamedir> <state.bin> <test> [args]\n"); return 1; }
	registerTests();
	const std::string name = argv[4];
	if (!registry().count(name)) { std::fprintf(stderr, "unknown test %s\n", name.c_str()); return 1; }
	TestCase &t = registry()[name];
	Args args;
	for (int i = 5; i < argc; ++i) args.v.push_back(argv[i]);

	wl::Machine start;
	if (!start.loadState(argv[3])) { std::fprintf(stderr, "cannot load state\n"); return 1; }
	const Regs regs = t.setup(start, args);

	// original code
	emu::Machine em(argv[2]);
	em.boot(fod::readFile(argv[1]));
	em.cpu.mem = start.mem;
	em.ega = start.ega;
	em.cpu.ax = regs.ax; em.cpu.bx = regs.bx; em.cpu.cx = regs.cx; em.cpu.dx = regs.dx; em.cpu.si = regs.si; em.cpu.di = regs.di;
	em.cpu.es = 0xA000;
	const std::string why = em.callNear(uint16_t(t.entry));
	wl::Machine ref;
	ref.mem = em.cpu.mem;
	ref.ega = em.ega;

	// native port
	wl::Machine nat = start;
	t.native(nat, args, regs);

	const size_t n = nat.diff(ref, 40);
	std::printf("%s: original %s, %zu differing bytes\n", name.c_str(), why.c_str(), n);
	return n == 0 ? 0 : 2;
}

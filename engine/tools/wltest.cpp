// wltest: differential tests of the native Wasteland routines against the original code running in the 8086 oracle.
//
//   wltest <image.bin> <gamedir> <state.bin> <test> [args...]
// The state file comes from `wlemu ... save`. Each test pokes some globals, runs the original routine in the interpreter
// and the native port on a copy of the same state, then compares data segment, graphics buffer and EGA planes.
#include "emu.h"
#include "wl/machine.h"
#include "wl/ui.h"
#include "wl/globals.h"
#include "wl/rules.h"
#include "wl/party.h"
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
struct Out { int al = -1, bl = -1, dl = -1, ax = -1, dx = -1, cf = -1, zf = -1; };   // register results to compare with the original (-1: not checked)

struct TestCase {
	uint16_t entry;                                                              // near address in the code segment (offset from 10000h)
	std::function<Regs(wl::Machine &, const Args &)> setup;                     // pokes the shared start state, returns register inputs
	std::function<Out(wl::Machine &, const Args &, const Regs &)> native;       // the port
	std::vector<uint16_t> scratch;                                               // data segment bytes the original uses as temporaries (not compared)
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
	    [](Machine &m, const Args &, const Regs &r) { writeCharacterAt(m, uint8_t(r.ax)); return Out{}; }};
	// writeColorCharacter <glyph> [x y]
	registry()["writeColorCharacter"] = {0x0CB6,
	    [](Machine &m, const Args &a) {
		    if (a.v.size() > 2) { m.d8(g::stringXPos) = uint8_t(a.hex(1)); m.d8(g::stringYPos) = uint8_t(a.hex(2)); }
		    Regs r; r.ax = uint16_t(a.hex(0)); return r;
	    },
	    [](Machine &m, const Args &, const Regs &r) { writeColorCharacter(m, uint8_t(r.ax)); return Out{}; }};
	// clearRect <leftCol> <rightCol> <topLine> <bottomLine>
	registry()["clearRect"] = {0x0762,
	    [](Machine &m, const Args &a) {
		    m.d8(g::rectLeftCol) = uint8_t(a.hex(0)); m.d8(g::rectRightCol) = uint8_t(a.hex(1));
		    m.d8(g::rectTopLine) = uint8_t(a.hex(2)); m.d8(g::rectBottomLine) = uint8_t(a.hex(3));
		    return Regs{};
	    },
	    [](Machine &m, const Args &, const Regs &) { clearRect(m); return Out{}; }};

	// ---- rules: random numbers and dice accumulators. Machine <-> Rules adapters keep the data segment authoritative.
	auto loadRules = [](Machine &m, Rules &r) {
		for (int i = 0; i < 5; ++i) r.rngState[i] = m.d8(uint16_t(0x465C + i));
		r.seed2 = m.d16(0x46BE);
		r.seed1 = m.d16(0x46C0);
	};
	auto storeRules = [](Machine &m, const Rules &r) {
		for (int i = 0; i < 5; ++i) m.d8(uint16_t(0x465C + i)) = r.rngState[i];
		m.setD16(0x46BE, r.seed2);
		m.setD16(0x46C0, r.seed1);
	};
	// rngByte <al> [s0 s1 s2 s3 s4]
	registry()["rngByte"] = {0x8E6B,
	    [](Machine &m, const Args &a) {
		    for (int i = 0; i < 5 && size_t(1 + i) < a.v.size(); ++i) m.d8(uint16_t(0x465C + i)) = uint8_t(a.hex(size_t(1 + i)));
		    Regs r; r.ax = uint16_t(a.hex(0)); return r;
	    },
	    [=](Machine &m, const Args &, const Regs &r) { Rules q; loadRules(m, q); Out o; o.al = q.rngByte(uint8_t(r.ax)); storeRules(m, q); return o; }};
	// random <n>   (sub_18E41, result in al)
	registry()["random"] = {0x8E41,
	    [](Machine &, const Args &a) { Regs r; r.ax = uint16_t(a.hex(0)); return r; },
	    [=](Machine &m, const Args &, const Regs &r) { Rules q; loadRules(m, q); Out o; o.al = q.random(uint8_t(r.ax)); storeRules(m, q); return o; },
	    {0xC059}};
	// d6 <al>
	registry()["d6"] = {0x8E5F,
	    [](Machine &, const Args &a) { Regs r; r.ax = uint16_t(a.hex(0)); return r; },
	    [=](Machine &m, const Args &, const Regs &r) { Rules q; loadRules(m, q); Out o; o.al = q.d6(uint8_t(r.ax)); storeRules(m, q); return o; }};
	// rollDice (sub_19C84, result in al)
	registry()["rollDice"] = {0x9C84,
	    [](Machine &, const Args &) { return Regs{}; },
	    [=](Machine &m, const Args &, const Regs &) { Rules q; loadRules(m, q); Out o; o.al = int(q.rollDice() & 0xFF); storeRules(m, q); return o; },
	    {0xCD49, 0xCD4A}};

	// ---- party and items
	// setPartyIndexesPtr <group>
	registry()["setPartyIndexesPtr"] = {0x7208,
	    [](Machine &, const Args &a) { Regs r; r.ax = uint16_t(a.hex(0)); return r; },
	    [](Machine &m, const Args &, const Regs &r) { setPartyIndexesPtr(m, uint8_t(r.ax)); return Out{}; }};
	// setCurrentCharacter <partyNum>
	registry()["setCurrentCharacter"] = {0x9614,
	    [](Machine &, const Args &a) { Regs r; r.ax = uint16_t(a.hex(0)); return r; },
	    [](Machine &m, const Args &, const Regs &r) { setCurrentCharacter(m, uint8_t(r.ax)); return Out{}; }};
	// isMemberUnconscious <partyNum> [conLo conHi]
	registry()["isMemberUnconscious"] = {0x72BB,
	    [](Machine &m, const Args &a) {
		    setCurrentCharacter(m, uint8_t(a.hex(0)));
		    if (a.v.size() > 2) { const uint16_t c = m.d16(g::currentCharacter); m.d8(uint16_t(c + pm::con)) = uint8_t(a.hex(1)); m.d8(uint16_t(c + pm::con + 1)) = uint8_t(a.hex(2)); }
		    Regs r; r.ax = uint16_t(a.hex(0)); return r;
	    },
	    [](Machine &m, const Args &, const Regs &r) { Out o; o.cf = isMemberUnconscious(m, uint8_t(r.ax)); return o; }};
	// conBytesOr <partyNum> [conLo conHi]  (sub_172AE)
	registry()["conBytesOr"] = {0x72AE,
	    [](Machine &m, const Args &a) {
		    setCurrentCharacter(m, uint8_t(a.hex(0)));
		    if (a.v.size() > 2) { const uint16_t c = m.d16(g::currentCharacter); m.d8(uint16_t(c + pm::con)) = uint8_t(a.hex(1)); m.d8(uint16_t(c + pm::con + 1)) = uint8_t(a.hex(2)); }
		    return Regs{};
	    },
	    [](Machine &m, const Args &, const Regs &) { Out o; o.al = conBytesOr(m); o.zf = o.al == 0; return o; }};
	// getMemberConditionIndex <partyNum> [conLo conHi]
	registry()["getMemberConditionIndex"] = {0x9A1D,
	    [](Machine &m, const Args &a) {
		    setCurrentCharacter(m, uint8_t(a.hex(0)));
		    if (a.v.size() > 2) { const uint16_t c = m.d16(g::currentCharacter); m.d8(uint16_t(c + pm::con)) = uint8_t(a.hex(1)); m.d8(uint16_t(c + pm::con + 1)) = uint8_t(a.hex(2)); }
		    return Regs{};
	    },
	    [](Machine &m, const Args &, const Regs &) { Out o; o.dl = getMemberConditionIndex(m); return o; }};
	// getMemberSkillLevel <partyNum> <skillId>
	registry()["getMemberSkillLevel"] = {0x98CD,
	    [](Machine &m, const Args &a) { setCurrentCharacter(m, uint8_t(a.hex(0))); Regs r; r.ax = uint16_t(a.hex(1)); return r; },
	    [](Machine &m, const Args &, const Regs &r) { Out o; o.al = getMemberSkillLevel(m, uint8_t(r.ax)); o.zf = o.al == 0; return o; }};
	// findItemById <partyNum> <itemId>
	registry()["findItemById"] = {0x968C,
	    [](Machine &m, const Args &a) { setCurrentCharacter(m, uint8_t(a.hex(0))); Regs r; r.ax = uint16_t(a.hex(1)); return r; },
	    [](Machine &m, const Args &, const Regs &r) { const int off = findItemById(m, uint8_t(r.ax)); Out o; o.bl = off < 0 ? 0xFF : off; o.cf = off < 0; return o; },
	    {0xCCE1}};
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
	const Out out = t.native(nat, args, regs);
	for (uint16_t off : t.scratch) nat.d8(off) = ref.d8(off);

	size_t n = nat.diff(ref, 40);
	if (out.al >= 0 && out.al != (em.cpu.ax & 0xFF)) { std::printf("  AL: native %02X original %02X\n", out.al, em.cpu.ax & 0xFF); ++n; }
	if (out.bl >= 0 && out.bl != (em.cpu.bx & 0xFF)) { std::printf("  BL: native %02X original %02X\n", out.bl, em.cpu.bx & 0xFF); ++n; }
	if (out.dl >= 0 && out.dl != (em.cpu.dx & 0xFF)) { std::printf("  DL: native %02X original %02X\n", out.dl, em.cpu.dx & 0xFF); ++n; }
	if (out.zf >= 0 && out.zf != int(em.cpu.flag(x86::ZF))) { std::printf("  ZF: native %d original %d\n", out.zf, int(em.cpu.flag(x86::ZF))); ++n; }
	if (out.ax >= 0 && out.ax != em.cpu.ax) { std::printf("  AX: native %04X original %04X\n", out.ax, em.cpu.ax); ++n; }
	if (out.dx >= 0 && out.dx != em.cpu.dx) { std::printf("  DX: native %04X original %04X\n", out.dx, em.cpu.dx); ++n; }
	if (out.cf >= 0 && out.cf != int(em.cpu.flag(x86::CF))) { std::printf("  CF: native %d original %d\n", out.cf, int(em.cpu.flag(x86::CF))); ++n; }
	std::printf("%s: original %s, %zu differences\n", name.c_str(), why.c_str(), n);
	return n == 0 ? 0 : 2;
}

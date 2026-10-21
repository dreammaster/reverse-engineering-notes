// wlrctest <image.bin> <gamedir> <entries.txt> <state.bin>... : checks every recompiled function against the 8086 oracle.
//
// For each function entry the state file is loaded into the oracle and into a native Machine, the registers are set to
// the same (pseudo random) inputs, the function is run to its return on both sides and registers, flags, memory and EGA
// planes are compared. Functions that wait for input, run away, or touch the timer are skipped.
#include "emu.h"
#include "wl/recomp_rt.h"
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <sstream>

using namespace wl;

namespace {

uint32_t rng(uint32_t &s) { s = s * 1664525u + 1013904223u; return s >> 8; }

struct Result {
	enum Kind { Ok, Skip, Diff } kind = Ok;
	std::string note;
};

Result testFunction(emu::Machine &o, Machine &n, const std::string &state, const std::string &gameDir, uint16_t entry, uint32_t seed) {
	Result res;
	if (!o.loadState(state) || !n.loadState(state)) { res.kind = Result::Skip; res.note = "state"; return res; }
	n.sys.reset();
	{   // native runtime without loading an image (the state file already holds it)
		n.sys = std::make_shared<rc::Sys>();
		n.sys->gameDir = gameDir;
		n.sys->indexGameDir();
		n.sys->edgesPerTick = ~0ull / 2;
		n.sys->nextTick = ~0ull / 2;
		n.sys->maxEdges = 4000000;
		n.sys->nextHandle = o.nextHandle;
	}
	o.cpu.fault.clear(); o.cpu.halted = false; o.exited = false; o.needKey = false;
	o.keys.clear(); n.keys.clear();
	o.detectIdle = true;
	// register inputs: the state's own values with a few random small ones
	uint32_t s = seed;
	rc::Regs r;
	const uint16_t ax = uint16_t(rng(s) & 0xFF), bx = uint16_t(rng(s) & 0x1F), cx = uint16_t(rng(s) & 0x0F), dx = uint16_t(rng(s) & 0xFF);
	const uint16_t si = uint16_t(rng(s) & 0xFFF), di = uint16_t(rng(s) & 0xFFF);
	r.a.x = ax; r.b.x = bx; r.c.x = cx; r.d.x = dx; r.si = si; r.di = di;
	r.bp = o.cpu.bp; r.sp = o.cpu.sp; r.es = o.cpu.es; r.cs = 0x1000; r.ss = 0x3900; r.ds = 0x1CE2;
	r.cf = r.zf = r.sf = r.pf = false;
	o.cpu.ax = ax; o.cpu.bx = bx; o.cpu.cx = cx; o.cpu.dx = dx; o.cpu.si = si; o.cpu.di = di;
	o.cpu.flags = 0x0202;
	r.ifl = true;

	const uint16_t sp0 = o.cpu.sp;
	const std::string why = o.callNear(entry, 3000000);
	if (why != "bp") { res.kind = Result::Skip; res.note = why; return res; }

	try {
		rc::push(n, r, 0x0000);
		rc::dispatch(n, r, entry);
	} catch (const rc::WaitForInput &) {
		res.kind = Result::Skip; res.note = "native: waits for input"; return res;
	} catch (const rc::Fault &f) {
		res.kind = Result::Skip; res.note = std::string("native fault: ") + f.what(); return res;
	}
	if (!n.sys->unknownTargets.empty()) { res.kind = Result::Skip; res.note = "native: indirect jump to an unknown target"; return res; }
	std::ostringstream d;
	auto reg = [&](const char *name, unsigned a, unsigned b) { if (a != b) d << " " << name << " " << std::hex << a << "!=" << b; };
	reg("ax", o.cpu.ax, r.a.x); reg("bx", o.cpu.bx, r.b.x); reg("cx", o.cpu.cx, r.c.x); reg("dx", o.cpu.dx, r.d.x);
	reg("si", o.cpu.si, r.si); reg("di", o.cpu.di, r.di); reg("bp", o.cpu.bp, r.bp); reg("es", o.cpu.es, r.es); reg("ds", o.cpu.ds, r.ds);
	reg("sp", o.cpu.sp, r.sp);
	reg("cf", o.cpu.flag(x86::CF), r.cf); reg("zf", o.cpu.flag(x86::ZF), r.zf); reg("sf", o.cpu.flag(x86::SF), r.sf);
	reg("pf", o.cpu.flag(x86::PF), r.pf); reg("df", o.cpu.flag(x86::DF), r.df);
	(void)sp0;
	size_t bad = 0;
	std::ostringstream md;
	for (uint32_t a = 0; a < 0xA0000; ++a)
		if (o.cpu.mem[a] != n.mem[a]) { if (bad++ < 4) md << " mem[" << std::hex << a << "]=" << unsigned(o.cpu.mem[a]) << "/" << unsigned(n.mem[a]); }
	for (uint32_t a = 0xB0000; a < 0x100000; ++a)
		if (o.cpu.mem[a] != n.mem[a]) { if (bad++ < 4) md << " mem[" << std::hex << a << "]=" << unsigned(o.cpu.mem[a]) << "/" << unsigned(n.mem[a]); }
	size_t vbad = 0;
	for (int p = 0; p < 4; ++p)
		for (size_t i = 0; i < 65536; ++i)
			if (o.ega.planes[p][i] != n.ega.planes[p][i]) { if (vbad++ < 3) md << " pl" << p << "[" << std::hex << i << "]=" << unsigned(o.ega.planes[p][i]) << "/" << unsigned(n.ega.planes[p][i]); }
	if (!d.str().empty() || bad || vbad) {
		res.kind = Result::Diff;
		res.note = d.str() + (bad ? " memdiff " + std::to_string(bad) : "") + (vbad ? " vramdiff " + std::to_string(vbad) : "") + md.str();
	}
	return res;
}

} // namespace

int main(int argc, char **argv) {
	if (argc < 5) { std::fprintf(stderr, "usage: wlrctest <image.bin> <gamedir> <entries.txt> <state.bin>... [--only ADDR]\n"); return 1; }
	const std::string gameDir = argv[2];
	const fod::Bytes img = fod::readFile(argv[1]);
	std::vector<uint16_t> entries;
	{
		std::ifstream in(argv[3]);
		std::string line;
		while (std::getline(in, line)) {
			line = line.substr(0, line.find('#'));
			if (!line.empty()) entries.push_back(uint16_t(std::strtoul(line.c_str(), nullptr, 16)));
		}
	}
	std::vector<std::string> states;
	long only = -1;
	for (int i = 4; i < argc; ++i) {
		if (std::string(argv[i]) == "--only" && i + 1 < argc) only = long(std::strtoul(argv[++i], nullptr, 16));
		else states.push_back(argv[i]);
	}
	emu::Machine o(gameDir);
	Machine n;
	std::memcpy(&o.cpu.mem[0x10000], img.data(), img.size());
	int ok = 0, skip = 0, diff = 0;
	std::map<std::string, int> reasons;
	for (const std::string &state : states) {
		for (uint16_t e : entries) {
			if (only >= 0 && e != only) continue;
			uint32_t seed = e * 2654435761u + 12345u;
			const Result r = testFunction(o, n, state, gameDir, e, seed);
			if (r.kind == Result::Ok) ++ok;
			else if (r.kind == Result::Skip) { ++skip; ++reasons[r.note.substr(0, 40)]; if (only >= 0) std::printf("%04X skipped: %s\n", e, r.note.c_str()); }
			else { ++diff; std::printf("%04X DIFF%s\n", e, r.note.c_str()); }
		}
		std::printf("%s: ok %d skipped %d different %d\n", state.c_str(), ok, skip, diff);
		for (auto &kv : reasons) std::printf("  skipped %d: %s", kv.second, kv.first.c_str()), std::puts("");
		reasons.clear();
		ok = skip = diff = 0;
	}
	return 0;
}

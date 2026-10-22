// wlrctest <image.bin> <gamedir> <entries.txt> <state.bin>... : checks every recompiled function against the 8086 oracle.
//
// For each function entry the state file is loaded into the oracle and into a native Machine, the registers are set to
// the same (pseudo random) inputs, the function is run to its return on both sides and registers, flags, memory and EGA
// planes are compared. Functions that wait for input, run away, or touch the timer are skipped.
#include "emu.h"
#include "wasteland1/recomp_rt.h"
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

using namespace wl;

namespace {

uint32_t rng(uint32_t &s) { s = s * 1664525u + 1013904223u; return s >> 8; }

struct Result {
	enum Kind { Ok, Skip, Diff } kind = Ok;
	std::string note;
	std::set<std::string> regs;   // registers/flags that differ (candidates for the junk list of a port)
};

std::vector<uint16_t> gKeys;   // keys offered to both machines before each run (--keys)

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
		rc::installCallbacks(n);
	}
	o.cpu.fault.clear(); o.cpu.halted = false; o.exited = false; o.needKey = false;
	o.keys.clear(); n.keys.clear();
	for (uint16_t k : gKeys) { o.keys.push_back(k); n.keys.push_back(k); }
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
	uint32_t junk = 0;
	const uint16_t *scratch = nullptr;
	for (const rc::PortInfo *p = rc::kPortInfo; p->name; ++p)
		if (p->address == entry) { junk = p->junk; scratch = p->scratch; }
	const uint32_t deadStackEnd = 0x39000u + std::max<uint16_t>(r.sp, o.cpu.sp);   // the stack below sp is dead after the return: callees leave residue there
	auto isScratch = [&](uint32_t a) {
		if (a >= 0x39000u && a < deadStackEnd) return true;
		if (!scratch || a < kDsLinear || a >= kDsLinear + 0x10000) return false;
		for (const uint16_t *q = scratch; *q != 0xFFFF; q += 2)
			if (a - kDsLinear >= q[0] && a - kDsLinear <= q[1]) return true;
		return false;
	};
	std::ostringstream d;
	auto reg = [&](const char *name, unsigned a, unsigned b, uint32_t bit = 0) { if (a != b && !(junk & bit)) { d << " " << name << " " << std::hex << a << "!=" << b; res.regs.insert(name); } };
	// 16 bit registers are compared byte-wise so that the junk list can name single bytes
	reg("al", o.cpu.ax & 0xFF, r.a.b.l, rc::j_al); reg("ah", o.cpu.ax >> 8, r.a.b.h, rc::j_ah);
	reg("bl", o.cpu.bx & 0xFF, r.b.b.l, rc::j_bl); reg("bh", o.cpu.bx >> 8, r.b.b.h, rc::j_bh);
	reg("cl", o.cpu.cx & 0xFF, r.c.b.l, rc::j_cl); reg("ch", o.cpu.cx >> 8, r.c.b.h, rc::j_ch);
	reg("dl", o.cpu.dx & 0xFF, r.d.b.l, rc::j_dl); reg("dh", o.cpu.dx >> 8, r.d.b.h, rc::j_dh);
	reg("si", o.cpu.si, r.si, rc::j_si); reg("di", o.cpu.di, r.di, rc::j_di); reg("bp", o.cpu.bp, r.bp, rc::j_bp);
	reg("es", o.cpu.es, r.es, rc::j_es); reg("ds", o.cpu.ds, r.ds);
	reg("sp", o.cpu.sp, r.sp, rc::j_sp);
	reg("cf", o.cpu.flag(x86::CF), r.cf, rc::j_cf); reg("zf", o.cpu.flag(x86::ZF), r.zf, rc::j_zf); reg("sf", o.cpu.flag(x86::SF), r.sf, rc::j_sf);
	reg("pf", o.cpu.flag(x86::PF), r.pf, rc::j_pf); reg("df", o.cpu.flag(x86::DF), r.df, rc::j_df);
	(void)sp0;
	size_t bad = 0;
	std::ostringstream md;
	for (uint32_t a = 0; a < 0xA0000; ++a)
		if (o.cpu.mem[a] != n.mem[a] && !isScratch(a) && !(a >= 0x10000 && a < kDsLinear)) { if (bad++ < 4) md << " mem[" << std::hex << a << "]=" << unsigned(o.cpu.mem[a]) << "/" << unsigned(n.mem[a]); }
	for (uint32_t a = 0xB0000; a < 0x100000; ++a)
		if (o.cpu.mem[a] != n.mem[a] && !isScratch(a)) { if (bad++ < 4) md << " mem[" << std::hex << a << "]=" << unsigned(o.cpu.mem[a]) << "/" << unsigned(n.mem[a]); }
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
	if (argc < 5) { std::fprintf(stderr, "usage: wlrctest <image.bin> <gamedir> <entries.txt> <state.bin>... [--only ADDR] [--ported]\n"); return 1; }
	const std::string gameDir = argv[2];
	const shared::Bytes img = shared::readFile(argv[1]);
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
	bool portedOnly = false;
	for (int i = 4; i < argc; ++i) {
		if (std::string(argv[i]) == "--only" && i + 1 < argc) only = long(std::strtoul(argv[++i], nullptr, 16));
		else if (std::string(argv[i]) == "--ported") portedOnly = true;
		else if (std::string(argv[i]) == "--keys" && i + 1 < argc) {
			std::stringstream ks(argv[++i]);
			std::string k;
			while (std::getline(ks, k, ',')) gKeys.push_back(uint16_t(std::strtoul(k.c_str(), nullptr, 16)));
		}
		else states.push_back(argv[i]);
	}
	emu::Machine o(gameDir);
	Machine n;
	std::memcpy(&o.cpu.mem[0x10000], img.data(), img.size());
	int ok = 0, skip = 0, diff = 0;
	std::map<std::string, int> reasons;
	std::map<uint16_t, std::set<std::string>> junkSeen;
	for (const std::string &state : states) {
		for (uint16_t e : entries) {
			if (only >= 0 && e != only) continue;
			if (portedOnly) {
				bool ported = false;
				for (const rc::PortInfo *p = rc::kPortInfo; p->name; ++p) ported |= p->address == e;
				if (!ported) continue;
			}
			uint32_t seed = e * 2654435761u + 12345u;
			const Result r = testFunction(o, n, state, gameDir, e, seed);
			if (r.kind == Result::Ok) ++ok;
			else if (r.kind == Result::Skip) { ++skip; ++reasons[r.note.substr(0, 40)]; if (only >= 0) std::printf("%04X skipped: %s\n", e, r.note.c_str()); }
			else { ++diff; std::printf("%04X DIFF%s\n", e, r.note.c_str()); }
			if (portedOnly) junkSeen[e].insert(r.regs.begin(), r.regs.end());
		}
		std::printf("%s: ok %d skipped %d different %d\n", state.c_str(), ok, skip, diff);
		for (auto &kv : reasons) std::printf("  skipped %d: %s", kv.second, kv.first.c_str()), std::puts("");
		reasons.clear();
		ok = skip = diff = 0;
	}
	if (portedOnly) {
		std::printf("registers that differ in any run (junk candidates):\n");
		for (auto &kv : junkSeen) {
			std::string list;
			for (const std::string &n : kv.second) list += (list.empty() ? "" : ",") + n;
			std::printf("%04X %s\n", kv.first, list.c_str());
		}
	}
	return 0;
}

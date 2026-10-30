#include "host.h"

#include <cstdio>
#include <cstring>

struct FuncName { uint32_t linear; const char *name; const char *stubTarget; };
extern const FuncName kFuncNames[];
extern const unsigned kFuncNameCount;
struct HostEntry { const char *name; void (*fn)(Cpu &); };
extern const HostEntry kHosts[];
extern const unsigned kHostCount;

static Host *g_host = nullptr;
Host &host() { if (!g_host) g_host = new Host(); return *g_host; }
Cpu &liftedCpu() { return host().cpu; }

void hostUnimplemented(const char *name, Cpu &c) {
	Event e;
	e.name = name;
	for (int i = 0; i < 8; i++)
		e.w[i] = c.arg(i);
	host().events.push_back(e);
	c.ax.x = 0;
	c.dx.x = 0;
}

// Calls through a function pointer in game memory (call_object_handler, command tables, ...): find the lifted function at that address.
static void farHook(Cpu &c, uint16_t seg, uint16_t off) {
	Host &h = host();
	uint32_t lin = (uint32_t)seg * 16 + off;
	auto it = h.liftedAt.find(lin);
	if (it != h.liftedAt.end()) {
		if (c.onCall) {
			auto n = h.nameAt.find(lin);
			c.onCall(c, n != h.nameAt.end() ? n->second.first.c_str() : "?");
		}
		it->second(c);
		return;
	}
	// an RTLink stub IDA did not turn into a function: E8 rel16 (call rtlink_thunk), EA off16 seg16 (jmp far real_function)
	if (h.nameAt.find(lin) == h.nameAt.end() && c.rd8(0, 0) == c.rd8(0, 0)) {
		const uint8_t *m = h.mem + lin;
		if (m[0] == 0xE8 && m[3] == 0xEA) {
			uint32_t target = (uint32_t)(m[6] | (m[7] << 8)) * 16 + (m[4] | (m[5] << 8));
			auto jt = h.liftedAt.find(target);
			if (jt != h.liftedAt.end()) {
				if (c.onCall) c.onCall(c, "stub->lifted");
				jt->second(c);
				return;
			}
			char buf[96];
			snprintf(buf, sizeof(buf), "unlifted call to %04X:%04X (RTLink stub -> %05X)", seg, off, target);
			h.problem(buf);
			c.ax.x = c.dx.x = 0;
			return;
		}
	}
	auto n = h.nameAt.find(lin);
	std::string name = n != h.nameAt.end() ? (n->second.second.empty() ? n->second.first : n->second.second) : "";
	auto hf = h.hostByName.find(name);
	if (hf != h.hostByName.end()) {
		if (c.onCall) c.onCall(c, name.c_str());
		hf->second(c);
		return;
	}
	char buf[80];
	snprintf(buf, sizeof(buf), "unlifted call to %04X:%04X", seg, off);
	std::string ctx;
	for (unsigned i = 0; i < 6 && i < h.recentPos; i++)
		ctx += " " + h.recent[(h.recentPos - 1 - i) % 6];
	h.problem(std::string(buf) + (name.empty() ? "" : " (" + name + ")") + " after:" + ctx);
	c.ax.x = c.dx.x = 0;
}

static void traceCall(Cpu &c, const char *name) {
	Host &h = host();
	h.recent[h.recentPos++ % 6] = name;
	if (!h.tracing || h.calls.size() > 100000)
		return;
	Event e;
	e.name = name;
	for (int i = 0; i < 8; i++)
		e.w[i] = c.arg(i);
	h.calls.push_back(e);
}

void Host::init(const std::string &imagePath, const std::string &xanthstr) {
	image = Legend::readFile(imagePath);
	msgs = new Legend::MessageFile(Legend::readFile(xanthstr));
	objectDat = Legend::readFile(xanthstr.substr(0, xanthstr.find_last_of("/\\")) + "/OBJECT.DAT");
	for (unsigned i = 0; i < kLiftedCount; i++) {
		liftedAt[kLifted[i].linear] = kLifted[i].fn;
		std::string nm = kLifted[i].name, alias;
		for (char ch : nm) { if (ch == '?') alias += "_q"; else alias.push_back(ch); }
		liftedByName[nm] = kLifted[i].fn;      // original name (object_has_attr?) and C identifier (object_has_attr_q)
		liftedByName[alias] = kLifted[i].fn;
	}
	for (unsigned i = 0; i < kFuncNameCount; i++)
		nameAt[kFuncNames[i].linear] = std::make_pair(std::string(kFuncNames[i].name), std::string(kFuncNames[i].stubTarget));
	for (unsigned i = 0; i < kHostCount; i++)
		hostByName[kHosts[i].name] = kHosts[i].fn;
	cpu.mem = mem;
	cpu.callFarHook = farHook;
	cpu.onCall = traceCall;
	reset();
}

void Host::reset() {
	memset(mem, 0, sizeof(mem));
	memcpy(mem, image.data(), image.size());
	// object_get_name? returns unk_80741 + nameOffset: game_init reads the whole OBJECT.DAT (u16 pool size, then the NUL-separated names) to 0x80740
	if (!objectDat.empty() && 0x80740 + objectDat.size() < sizeof(mem))
		memcpy(mem + 0x80740, objectDat.data(), objectDat.size());
	cpu = Cpu();
	cpu.ax.x = cpu.bx.x = cpu.cx.x = cpu.dx.x = 0;
	cpu.mem = mem;
	cpu.callFarHook = farHook;
	cpu.onCall = traceCall;
	cpu.ds = cpu.es = cpu.cs = SEG_dseg_para;
	cpu.ss = SEG_dseg_para;                           // DOS small-model stack: SS == DS (the stack sits at the top of the data segment, `mov [bx], al` on a stack address uses DS)
	cpu.sp = 0xFFF0;
	events.clear();
	calls.clear();
	scratch = 0;
}

uint32_t Host::call(const char *name, std::initializer_list<uint16_t> args, int retWords) {
	auto it = liftedByName.find(name);
	if (it == liftedByName.end()) {
		problem(std::string("no lifted function ") + name);
		return 0;
	}
	uint16_t sp0 = cpu.sp;
	std::vector<uint16_t> a(args);
	for (size_t i = a.size(); i-- > 0;)
		cpu.push(a[i]);
	int rw = cpu.retWords;
	cpu.retWords = retWords;
	for (int i = 0; i < retWords; i++)
		cpu.push(0);
	it->second(cpu);
	cpu.sp = sp0;
	cpu.retWords = rw;
	return ((uint32_t)cpu.dx.x << 16) | cpu.ax.x;
}

std::string Host::str(uint16_t seg, uint16_t off, size_t max, bool *ok) const {
	std::string s;
	if (ok) *ok = false;
	for (size_t i = 0; i < max; i++) {
		uint8_t ch = cpu.rd8(seg, (uint16_t)(off + i));
		if (!ch) {
			if (ok) *ok = true;
			return s;
		}
		s.push_back((char)ch);
	}
	return s;
}

uint32_t Host::putString(const std::string &s) {
	// ring of 64 buffers of 512 bytes at D000:0000
	unsigned off = (scratch++ % 64) * 512;
	std::string t = s.substr(0, 500);
	for (size_t i = 0; i <= t.size(); i++)
		cpu.wr8(0xD000, (uint16_t)(off + i), i < t.size() ? (uint8_t)t[i] : 0);
	return (0xD000u << 16) | off;
}

std::string Host::message(uint32_t id, bool *ok) const {
	if (ok) *ok = false;
	if ((id >> 24) != 0xF0)
		return "";
	unsigned block = (id >> 16) & 0xFF, idx = id & 0xFFFF;
	if (block >= msgs->blockCount() || idx >= msgs->stringCount(block))
		return "";
	if (ok) *ok = true;
	return msgs->string(block, idx);
}

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
	if (h.mem[lin & 0x1FFFFF] == 0xCF) {               // a vector that still points at an IRET (BIOS default)
		c.sp = (uint16_t)(c.sp + 6);
		return;
	}
	auto it = h.liftedAt.find(lin);
	if (it != h.liftedAt.end()) {
		if (c.onCall) {
			auto n = h.nameAt.find(lin);
			c.onCall(c, n != h.nameAt.end() ? n->second.first.c_str() : "?");
		}
		it->second(c);                                 // lifted: the callee pops its own return address
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
			c.sp = (uint16_t)(c.sp + 2 * c.retWords);
			return;
		}
	}
	auto n = h.nameAt.find(lin);
	std::string name = n != h.nameAt.end() ? (n->second.second.empty() ? n->second.first : n->second.second) : "";
	auto hf = h.hostByName.find(name);
	if (hf != h.hostByName.end()) {
		if (c.onCall) c.onCall(c, name.c_str());
		c.popBytes = 2 * c.retWords;
		hf->second(c);
		c.sp = (uint16_t)(c.sp + c.popBytes);          // host function: pop the return words (plus any argument bytes it asks for)
		return;
	}
	char buf[80];
	snprintf(buf, sizeof(buf), "unlifted call to %04X:%04X", seg, off);
	std::string ctx;
	for (unsigned i = 0; i < 6 && i < h.recentPos; i++)
		ctx += " " + h.recent[(h.recentPos - 1 - i) % 6];
	{ char eb[160]; snprintf(eb, sizeof(eb), " | es:bx=%04X:%04X [%04X %04X %04X %04X %04X %04X]", c.es, c.bx.x, c.rd16(c.es, c.bx.x), c.rd16(c.es, (uint16_t)(c.bx.x + 2)), c.rd16(c.es, (uint16_t)(c.bx.x + 4)), c.rd16(c.es, (uint16_t)(c.bx.x + 6)), c.rd16(c.es, (uint16_t)(c.bx.x + 8)), c.rd16(c.es, (uint16_t)(c.bx.x + 10))); ctx += eb; }
	ctx += " | labels:";
	for (unsigned i = 0; i < 4; i++) { const char *l = c.hist[(c.hpos - 4 + i) & 63]; ctx += std::string(" ") + (l ? l : "-"); }
	h.problem(std::string(buf) + (name.empty() ? "" : " (" + name + ")") + " after:" + ctx);
	c.ax.x = c.dx.x = 0;
	c.sp = (uint16_t)(c.sp + 2 * c.retWords);
}

static void traceCall(Cpu &c, const char *name) {
	Host &h = host();
	h.recent[h.recentPos++ % 6] = name;
	if (!h.tracing || h.calls.size() > 4000000)
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
	cpu.irqHook = [](Cpu &cc) { host().machine.timerTick(cc); };
	machine.gameDir = xanthstr.substr(0, xanthstr.find_last_of("/\\"));
	machine.scanGameDir();
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
	machine.reset();
	machine.install(cpu);
	cpu.irqHook = [](Cpu &cc) { host().machine.timerTick(cc); };
	// installed drivers the program probes through the interrupt vector table: a mouse driver (vector 33h must not point at an IRET) and an EMS manager
	// (its device header at the vector's segment carries the name EMMXXXX0 at offset 0Ah)
	cpu.wr16(0, 0x33 * 4, 0x0010); cpu.wr16(0, 0x33 * 4 + 2, 0xC800);
	cpu.wr16(0, 0x67 * 4, 0x0000); cpu.wr16(0, 0x67 * 4 + 2, 0xC400);
	for (int v = 0; v < 256; v++)                         // every other vector points at an IRET (F000:FFF0), like the BIOS default
		if (v != 0x33 && v != 0x67) { cpu.wr16(0, (uint16_t)(v * 4), 0xFFF0); cpu.wr16(0, (uint16_t)(v * 4 + 2), 0xF000); }
	mem[0xFFFF0] = 0xCF;
	memcpy(mem + 0xC4000 + 0x0A, "EMMXXXX0", 8);
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
	try {
		it->second(cpu);
	} catch (const LongJmp &j) {      // jump to a setjmp buffer whose function is not running (the main loop when a test calls a function directly)
		stray.push_back(j);
		cpu.ax.x = cpu.dx.x = 0;
	}
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

void Machine::reset() {
	for (auto &f : files)
		if (f.second.fp)
			fclose(f.second.fp);
	files.clear();
	vfs.clear();
	// the configuration INSTALL.EXE would have written (read_configuration: GAMEDATA / SAVEDATA folders, MOUSE, MUSIC, SOUND devices)
	{
		std::string ini = "GAMEDATA=C:\\XANTH\\\r\nSAVEDATA=C:\\XANTH\\\r\nMOUSE=MOUSE\r\nMUSIC=NONE\r\nSOUND=NONE\r\n";
		vfs["legend.ini"] = std::vector<uint8_t>(ini.begin(), ini.end());
	}
	nextHandle = 5;
	stdoutText.clear();
	openLog.clear();
	ems.clear();
	for (auto &m : emsMap)
		m = {0, 0};
	nextEms = 1;
	emsUsed = 0;
	mouseScript.clear();
	keys.clear();
	mouseX = 160; mouseY = 100; mouseButtons = 0; mouseVisible = 0;
	pressCount[0] = pressCount[1] = releaseCount[0] = releaseCount[1] = 0;
	memset(palette, 0, sizeof(palette));
	dacRead = dacWrite = dacSub = 0;
	paletteWrites = 0;
	memset(vectors, 0, sizeof(vectors));
	dta = 0;
	clockCs = 0;
	pitCount = 0; pitFlip = 0;
	picMask = port61 = retrace = 0;
	videoMode = 3;
	initArena();
}

void callThroughHook(Cpu &c, uint16_t seg, uint16_t off) { farHook(c, seg, off); }

// IRQ0: run the program's INT 08h service routine (if it installed one) as the CPU would: push FLAGS, CS, IP and jump to the vector
void Machine::timerTick(Cpu &c) {
	uint32_t v8 = (uint32_t)c.rd16(0, 0x22) << 16 | c.rd16(0, 0x20), v1c = (uint32_t)c.rd16(0, 0x72) << 16 | c.rd16(0, 0x70);
	uint32_t vec = v8 != 0xF000FFF0u ? v8 : v1c;                  // INT 08h if the program took it over, else the BIOS timer tick user hook INT 1Ch
	if (inIsr || vec == 0xF000FFF0u || !vec)
		return;
	inIsr = true;
	uint16_t sp0 = c.sp, ss = c.ss, ds = c.ds, es = c.es, bp = c.bp;
	Reg ax = c.ax, bx = c.bx, cx = c.cx, dx = c.dx;
	uint16_t si = c.si, di = c.di;
	bool cf = c.CF, zf = c.ZF, sf = c.SF, of = c.OF;
	c.push(0x0202);
	c.push(c.cs);
	c.push(0);
	int rw = c.retWords;
	c.retWords = 3;
	callThroughHook(c, (uint16_t)(vec >> 16), (uint16_t)vec);
	c.retWords = rw;
	c.sp = sp0; c.ss = ss; c.ds = ds; c.es = es; c.bp = bp;
	c.ax = ax; c.bx = bx; c.cx = cx; c.dx = dx; c.si = si; c.di = di;
	c.CF = cf; c.ZF = zf; c.SF = sf; c.OF = of;
	irqCount++;
	inIsr = false;
}

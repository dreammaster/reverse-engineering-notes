#include "engine.h"
#include <cctype>
#include <cstdlib>
#include <cstring>

namespace shannara {

static uint64_t fnv(const uint8_t *p, size_t n, uint64_t h = 1469598103934665603ull) {
	for (size_t i = 0; i < n; i++) {
		h ^= p[i];
		h *= 1099511628211ull;
	}
	return h;
}

Engine::Engine(const std::string &exePath, const std::string &gameDir) : _str(readFile(gameDir + "/SHANSTR.DAT")) {
	_exe = new LeExe(readFile(exePath));
	_m = new Machine(*_exe);
	registerAllGenerated(*_m);
	// what load_object_dat? does at startup: the name/description pool
	Bytes d = readFile(gameDir + "/OBJECT.DAT");
	Cursor c(d);
	size_t n = c.u16();
	_m->writeBytes(0xCD9D0, c.ptr(), n);
	if (const char *cs = getenv("SHAN_CLOCK_STEP"))
		_clockStep = (uint32_t)atoi(cs);
	installShims();
}

// a message argument: a pointer to a C string, or a SHANSTR id tagged 0xF0 in the top byte (as the original's wprintf accepts)
std::string Engine::message(uint32_t v) {
	if (v >= 0xF0000000u) {
		try {
			std::string s = _str.string((int)((v >> 16) & 0xFF), (int)(v & 0xFFFF));
			while (!s.empty() && s.back() == 0)
				s.pop_back();
			return s;
		} catch (...) {
			char b[40];
			snprintf(b, sizeof b, "<bad message id %08x>", v);
			return b;
		}
	}
	try {
		return _m->cstr(v);
	} catch (...) {
		return "(null)";
	}
}

std::string Engine::format(const std::string &fmt, int argi) {
	std::string out;
	size_t i = 0;
	auto arg = [&]() { return _m->stackArg(argi++); };
	while (i < fmt.size()) {
		char c = fmt[i];
		if (c != '%') {
			out.push_back(c);
			i++;
			continue;
		}
		i++;
		while (i < fmt.size() && strchr("-+ #0123456789.l", fmt[i]))
			i++;
		char t = i < fmt.size() ? fmt[i] : '%';
		i++;
		if (t == '%') {
			out.push_back('%');
		} else if (t == 's') {
			uint32_t v = arg();
			out += v == 0 ? "(null)" : message(v);
		} else if (t == 'd' || t == 'u') {
			uint32_t v = arg();
			out += t == 'd' ? std::to_string((int32_t)v) : std::to_string(v);
		} else if (t == 'c') {
			out.push_back((char)(arg() & 0xFF));
		} else {
			arg();
		}
	}
	return out;
}

void installCrtShims(Machine &m);

void Engine::installShims() {
	Machine &m = *_m;
	m.setShim("wprintf", [this](Machine &mm) {
		uint32_t first = mm.stackArg(0);
		_out += format(message(first), 1);
		mm.eax = 0;
		mm.esp += 4;
	});
	m.setShim("wputchar", [this](Machine &mm) {
		_out.push_back((char)(mm.eax & 0xFF));
		mm.eax = 0;
		mm.finishShim("wputchar");
	});
	m.setShim("sprintf_", [this](Machine &mm) {
		uint32_t buf = mm.stackArg(0);
		std::string s = format(message(mm.stackArg(1)), 2);
		mm.writeBytes(buf, s.c_str(), s.size() + 1);
		mm.eax = (uint32_t)s.size();
		mm.esp += 4;
	});
	// the game's timing loops spin on the clock; by default it stays at 0 like the oracle's stub (parity tests). Set SHAN_CLOCK_STEP=7 to let it advance on every read so waits terminate
	for (const char *n : {"get_clock", "j___clock_", "__clock_", "clock_"})
		m.setShim(n, [this](Machine &mm) { _clock += _clockStep; mm.eax = _clock; mm.finishShim("get_clock"); });
	m.setShim("new_pointer", [](Machine &mm) {
		uint32_t size = mm.eax;
		uint32_t p = mm.heapPtr;
		mm.heapPtr += (size + 15) & ~15u;
		mm.eax = p;
		mm.finishShim("new_pointer");
	});
	installCrtShims(m);
}

uint32_t Engine::command(int verb, int dobj, int iobj, int actor) {
	Machine &m = *_m;
	m.w16(0xCD92C, (uint16_t)actor);
	m.w16(0xADE98, (uint16_t)actor);
	m.esp = Machine::kStackTop;
	m.budget = _budget;
	m.eax = (uint32_t)verb;
	m.edx = (uint32_t)dobj;
	m.ebx = (uint32_t)iobj;
	m.ecx = m.esi = m.edi = m.ebp = 0;
	m.push(0);
	m.call(0x1B7B0); // run_command?
	return m.eax;
}

uint32_t Engine::callFunction(uint32_t addr, uint32_t eax, uint32_t edx, uint32_t ebx, uint32_t ecx) {
	Machine &m = *_m;
	m.esp = Machine::kStackTop;
	m.budget = _budget;
	m.eax = eax;
	m.edx = edx;
	m.ebx = ebx;
	m.ecx = ecx;
	m.esi = m.edi = m.ebp = 0;
	m.push(0);
	m.call(addr);
	return m.eax;
}

std::string Engine::takeText() {
	std::string s;
	for (char c : _out)
		s.push_back(c == 0x0B || c == '\r' ? '\n' : c);
	_out.clear();
	return s;
}

uint64_t Engine::stateHash() {
	Machine &m = *_m;
	static const size_t prefix[6] = {0, 33, 41, 48, 24, 49};
	int n = m.r16(0x217A0);
	uint64_t h = 1469598103934665603ull;
	for (int i = 0; i < n; i++) {
		uint32_t p = m.r32(0x20919 + 5 * i);
		int t = m.r8(0x20918 + 5 * i);
		h = fnv(m.ptr(p), prefix[t], h);
	}
	return fnv(m.ptr(0xCD8AC), 100, h);
}

#include "save_regions.inc"

std::vector<std::pair<uint32_t, uint32_t>> Engine::regions() const {
	std::vector<std::pair<uint32_t, uint32_t>> v;
	for (auto &r : kSaveRegions)
		v.push_back({r.addr, r.size});
	return v;
}

uint32_t Engine::dataCrc() {
	static uint32_t table[256];
	if (!table[1]) {
		for (uint32_t i = 0; i < 256; i++) {
			uint32_t c = i;
			for (int k = 0; k < 8; k++)
				c = c & 1 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
			table[i] = c;
		}
	}
	uint32_t c = 0xFFFFFFFFu;
	const uint8_t *p = _m->ptr(0xA0000);
	for (uint32_t i = 0; i < 0x50000; i++)
		c = table[(c ^ p[i]) & 0xFF] ^ (c >> 8);
	return c ^ 0xFFFFFFFFu; // equals zlib.crc32 of the data object + BSS (0xA0000..0xF0000)
}

uint64_t Engine::regionHash() {
	uint64_t h = 1469598103934665603ull;
	for (auto &r : kSaveRegions)
		h = fnv(_m->ptr(r.addr), r.size, h);
	return h;
}

void Engine::snapshot() {
	_snapMem = _m->mem;
	_snapHeap = _m->heapPtr;
	_snapOut = _out.size();
	_snapClock = _clock;
}

void Engine::restore() {
	_m->mem = _snapMem;
	_m->heapPtr = _snapHeap;
	_out.resize(_snapOut);
	_clock = _snapClock;
}

} // namespace shannara

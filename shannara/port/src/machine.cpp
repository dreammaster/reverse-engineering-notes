#include "machine.h"
#include <cstring>

namespace shannara {

Machine::Machine(const LeExe &exe, size_t memSize, uint32_t delta) : mem(memSize, 0) {
	exe.mapInto(mem.data(), mem.size(), delta);
}

void Machine::memcpy_(void *d, const void *s, size_t n) {
	memmove(d, s, n);
}

std::string Machine::cstr(uint32_t a, size_t max) const {
	std::string s;
	while (s.size() < max) {
		uint8_t c = r8(a + (uint32_t)s.size());
		if (!c)
			break;
		s.push_back((char)c);
	}
	return s;
}

void Machine::call(uint32_t addr) {
	if (callHook)
		callHook(*this, addr);
	auto it = _fns.find(addr);
	if (it != _fns.end()) {
		trace.push_back(addr);
		it->second(*this);
		trace.pop_back();
	} else {
		callShim(addr);
	}
}

void Machine::callShim(uint32_t addr) {
	auto n = _shimNames.find(addr);
	char hexname[16];
	snprintf(hexname, sizeof hexname, "?%X", addr);
	std::string name = n != _shimNames.end() && n->second != "?" ? n->second : hexname;
	auto s = _shimByName.find(name);
	if (s != _shimByName.end()) {
		s->second(*this);
		return;
	}
	// default: a no-op that returns 0 and pops the return slot plus any callee-cleaned arguments
	unknownShimCalls[name]++;
	auto r = _retn.find(addr);
	if (logEvents) { // the engine boundary as a stream: name, register arguments and the callee-cleaned stack words
		char b[256];
		snprintf(b, sizeof b, "%s %x %x %x %x", name.c_str(), eax, edx, ebx, ecx);
		std::string e = b;
		for (uint32_t k = 0; r != _retn.end() && k < (uint32_t)r->second / 4 && k < 8; k++) {
			snprintf(b, sizeof b, " %x", r32(esp + 4 + 4 * k));
			e += b;
		}
		events.push_back(e);
	}
	eax = 0;
	esp += 4 + (r != _retn.end() ? (uint32_t)r->second : 0);
}

} // namespace shannara

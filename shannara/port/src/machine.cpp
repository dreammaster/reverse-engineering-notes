#include "machine.h"
#include <cstring>

namespace shannara {

Machine::Machine(const LeExe &exe) : mem(kMemSize, 0) {
	exe.mapInto(mem.data(), mem.size());
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
	std::string name = n != _shimNames.end() ? n->second : "?";
	auto s = _shimByName.find(name);
	if (s != _shimByName.end()) {
		s->second(*this);
		return;
	}
	// default: a no-op that returns 0 and pops the return slot plus any callee-cleaned arguments
	unknownShimCalls.push_back(name);
	auto r = _retn.find(addr);
	eax = 0;
	esp += 4 + (r != _retn.end() ? (uint32_t)r->second : 0);
}

} // namespace shannara

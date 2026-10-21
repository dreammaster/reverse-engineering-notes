#include "wl/machine.h"
#include <cstdio>
#include <map>

namespace wl {

bool Machine::loadState(const std::string &path) {
	FILE *f = std::fopen(path.c_str(), "rb");
	if (!f) return false;
	bool ok = std::fread(regs, sizeof regs, 1, f) == 1;
	ok = ok && std::fread(mem.data(), 1, mem.size(), f) == mem.size();
	ok = ok && std::fread(&ega, sizeof ega, 1, f) == 1;
	std::fclose(f);
	return ok;
}

size_t Machine::diff(const Machine &o, size_t limit, bool verbose) const {
	size_t total = 0, shown = 0;
	auto report = [&](const char *area, size_t off, unsigned a, unsigned b) {
		++total;
		if (verbose && shown < limit) { std::printf("  %s %05zX: %02X -> %02X\n", area, off, a, b); ++shown; }
	};
	for (size_t i = 0; i < kDsSize; ++i)
		if (mem[kDsLinear + i] != o.mem[kDsLinear + i]) report("ds ", i, mem[kDsLinear + i], o.mem[kDsLinear + i]);
	for (size_t i = 0; i < kGfxSize; ++i)
		if (mem[kGfxLinear + i] != o.mem[kGfxLinear + i]) report("gfx", i, mem[kGfxLinear + i], o.mem[kGfxLinear + i]);
	for (int p = 0; p < 4; ++p)
		for (size_t i = 0; i < 65536; ++i)
			if (ega.planes[p][i] != o.ega.planes[p][i]) {
				static const char *names[4] = {"pl0", "pl1", "pl2", "pl3"};
				report(names[p], i, ega.planes[p][i], o.ega.planes[p][i]);
			}
	return total;
}

} // namespace wl

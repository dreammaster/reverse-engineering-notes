#include "savestate.h"
#include <cstring>

namespace shannara {

#include "save_regions.inc"

static const uint32_t kTailAAddr = 0xADDEC, kTailBAddr = 0xCCBB8;
static const size_t kTailASize = 4, kTailBSize = 0x30;
static const int kLegacySkipA = 89, kLegacySkipB = 90; // regions missing from old files (dword_AE9F4, dword_AE9F8)

uint32_t SaveState::regionAddress(int i) { return kSaveRegions[i].addr; }
uint32_t SaveState::regionSize(int i) { return kSaveRegions[i].size; }

SaveState SaveState::initial(const LeExe &exe) {
	SaveState s;
	s.objects = RetailTables::load(exe).objects;
	for (int i = 0; i < kRegionCount; i++) {
		Bytes b(kSaveRegions[i].size);
		exe.read(kSaveRegions[i].addr, b.data(), b.size());
		s.regions.push_back(b);
	}
	s.tailA.resize(kTailASize);
	exe.read(kTailAAddr, s.tailA.data(), kTailASize);
	s.tailB.resize(kTailBSize);
	exe.read(kTailBAddr, s.tailB.data(), kTailBSize);
	return s;
}

void SaveState::load(const Bytes &file) {
	if (file.size() != kFullSize && file.size() != kLegacySize)
		throw FormatError("unexpected savegame size");
	bool legacy = file.size() == kLegacySize;
	Cursor c(file);
	for (size_t i = 0; i < kHeaderSize; i++)
		header[i] = c.u8();
	for (ObjectRecord &o : objects) {
		size_t n = objectPrefix(o.type);
		for (size_t i = 0; i < n; i++)
			o.raw[i] = c.u8();
	}
	for (int r = 0; r < kRegionCount; r++) {
		if (legacy && (r == kLegacySkipA || r == kLegacySkipB))
			continue;
		for (uint8_t &b : regions[r])
			b = c.u8();
	}
	for (uint8_t &b : tailA)
		b = c.u8();
	for (uint8_t &b : tailB)
		b = c.u8();
	if (c.left() != 0)
		throw FormatError("trailing bytes in savegame");
}

Bytes SaveState::save(bool legacy) const {
	Bytes out(header, header + kHeaderSize);
	for (const ObjectRecord &o : objects)
		out.insert(out.end(), o.raw.begin(), o.raw.begin() + objectPrefix(o.type));
	for (int r = 0; r < kRegionCount; r++) {
		if (legacy && (r == kLegacySkipA || r == kLegacySkipB))
			continue;
		out.insert(out.end(), regions[r].begin(), regions[r].end());
	}
	out.insert(out.end(), tailA.begin(), tailA.end());
	out.insert(out.end(), tailB.begin(), tailB.end());
	return out;
}

uint8_t *SaveState::at(uint32_t addr) {
	for (int i = 0; i < kRegionCount; i++)
		if (addr >= kSaveRegions[i].addr && addr < kSaveRegions[i].addr + kSaveRegions[i].size)
			return &regions[i][addr - kSaveRegions[i].addr];
	return nullptr;
}

bool SaveState::gameFlag(int n) const {
	const uint8_t *p = at(0xCD8AC);
	if (!p || n < 1 || n > 800)
		return false;
	return (p[(n - 1) >> 3] >> ((n - 1) & 7)) & 1;
}

} // namespace shannara

#include "fod/state.h"

namespace fod {

Disk1 Disk1::parse(const Bytes &file) {
	if (file.size() < kSize) throw std::runtime_error("DISK1 too small");
	Reader r(file);
	Disk1 d;
	d.saveCounter = r.u32();
	d.regionAndSpeed = r.u16();
	d.gfxMode = r.u8();
	d.uniqueItems = r.u8();
	d.hours = r.u8();
	d.minutes = r.u8();
	d.mapX = r.s16();
	d.mapY = r.s16();
	d.cash = r.u32();
	r.bytes(d.maps, 10);
	r.bytes(d.mapsX, 10);
	r.bytes(d.mapsY, 10);
	d.mapIndex = r.u8();
	d.partyCount = r.u8();
	r.bytes(d.partyIds, 5);
	d.weekday = r.u8();
	d.facing = r.u16();
	for (auto &m : d.roster) r.bytes(m.b.data(), Member::kSize);
	d.tail.resize(kSize - 0x6B6);
	r.bytes(d.tail.data(), d.tail.size());
	return d;
}

Bytes Disk1::serialize() const {
	Bytes out(kSize, 0);
	auto put16 = [&](size_t o, uint16_t v) { out[o] = uint8_t(v); out[o + 1] = uint8_t(v >> 8); };
	auto put32 = [&](size_t o, uint32_t v) { put16(o, uint16_t(v)); put16(o + 2, uint16_t(v >> 16)); };
	put32(0, saveCounter);
	put16(4, regionAndSpeed);
	out[6] = gfxMode; out[7] = uniqueItems; out[8] = hours; out[9] = minutes;
	put16(0xA, uint16_t(mapX)); put16(0xC, uint16_t(mapY));
	put32(0xE, cash);
	std::memcpy(&out[0x12], maps, 10);
	std::memcpy(&out[0x1C], mapsX, 10);
	std::memcpy(&out[0x26], mapsY, 10);
	out[0x30] = mapIndex; out[0x31] = partyCount;
	std::memcpy(&out[0x32], partyIds, 5);
	out[0x37] = weekday;
	put16(0x38, facing);
	for (size_t i = 0; i < roster.size(); ++i) std::memcpy(&out[0x3A + i * Member::kSize], roster[i].b.data(), Member::kSize);
	std::memcpy(&out[0x6B6], tail.data(), std::min(tail.size(), size_t(kSize - 0x6B6)));
	return out;
}

Disk1 Disk1::newGame() {
	Disk1 d;
	d.saveCounter = 1;
	d.hours = 9; d.minutes = 0; d.weekday = 1; d.uniqueItems = 0; d.facing = 0;
	d.mapX = 10; d.mapY = 6; d.cash = 100;
	d.maps[0] = 0x1C; d.mapsX[0] = 10; d.mapsY[0] = 6; d.mapIndex = 0;
	d.partyCount = 0;
	d.tail.assign(kSize - 0x6B6, 0);
	for (size_t i = 0; i < d.roster.size(); ++i) {
		Member &m = d.roster[i];
		m.b.fill(0);
		m.itemCount() = 6;
		for (int s = 0; s < 32; ++s) m.itemId(s) = 0xFF;
		const uint8_t start[6] = {2, 0x0C, 0x0C, 0x20, 0x29, 0x37};
		for (int s = 0; s < 6; ++s) m.itemId(s) = start[s];
		m.itemAmmo(0) = 9;
		m.rank() = 1;
		m.setU32(0x5E, 0x5DC);
		m.readied() = 0xFF;
		for (int a = 0; a < 3; ++a) m.u8(0x4B + a) = 0xFF;
		m.flags() = 1;
	}
	return d;
}

} // namespace fod

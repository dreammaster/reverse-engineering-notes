#include "wasteland1/gamefile.h"
#include <algorithm>

namespace wl {

std::string Npc::name() const {
	std::string s(reinterpret_cast<const char *>(raw.data()), std::min<size_t>(14, raw.size()));
	s.resize(std::strlen(s.c_str()));
	return s;
}

namespace {

uint16_t w16(const Bytes &b, size_t o) { return uint16_t(b.at(o) | (b.at(o + 1) << 8)); }

bool isSaveGame(const Bytes &b) {
	std::vector<int> seen;
	for (int i = 1; i < 8; ++i) {
		const int v = b[size_t(i)];
		if (v > 7) return false;
		if (v != 0 && std::find(seen.begin(), seen.end(), v) != seen.end()) return false;
		seen.push_back(v);
	}
	return true;
}

int blockType(const Bytes &f, const MsqBlock &blk) {   // 0 map, 1 savegame, 2 shop list
	const Bytes b = xorDecrypt(f.data() + blk.offset + 4, blk.size - 4, 9);
	if (blk.size == 4614 && b.size() >= 9 && isSaveGame(b)) return 1;
	if (blk.size == 766 && b.size() >= 3 && b[0] == 0x60 && b[1] == 0x60 && b[2] == 0x60) return 2;
	return 0;
}

} // namespace

GameMap parseMapBlock(const Bytes &file, const MsqBlock &blk) {
	GameMap m;
	m.msqSize = blk.size;
	const uint8_t *p = file.data() + blk.offset + 4;
	const size_t n = blk.size - 4;
	// map size: look at the deobfuscated first 6189 bytes (same probe as the Wasteland Suite)
	Bytes probe = xorDecrypt(p, n, 6189);
	auto sizeOk = [&](int s) {
		const size_t o = size_t(s) * s * 3 / 2;
		return o + 44 < probe.size() && probe[o + 6] == 0 && probe[o + 7] == 0 && probe[o + 44] == s;
	};
	const bool is32 = sizeOk(32), is64 = sizeOk(64);
	if (is32 == is64) throw std::runtime_error("cannot determine map size");
	m.size = is32 ? 32 : 64;
	const size_t encSize = w16(probe, size_t(m.size) * m.size * 3 / 2);   // = stringsOffset: the obfuscated part
	Bytes d = xorDecrypt(p, n, encSize);
	d.insert(d.end(), p + 2 + encSize, p + n);
	m.decoded = d;
	// tile map offset: the last "00 hi 00 00 .. 00 00" header
	{
		size_t i = d.size() - 9;
		for (; i > 0; --i)
			if (d[i] == 0 && d[i + 1] == ((m.size * m.size) >> 8) && d[i + 2] == 0 && d[i + 3] == 0 && d[i + 6] == 0 && d[i + 7] == 0) break;
		if (i == 0) throw std::runtime_error("tile map not found");
		m.tilemapOffset = i;
	}
	size_t pos = 0;
	m.actionClass.resize(size_t(m.size) * m.size);
	for (size_t i = 0; i < m.actionClass.size() / 2; ++i) {
		m.actionClass[i * 2] = d[pos] >> 4;
		m.actionClass[i * 2 + 1] = d[pos] & 15;
		++pos;
	}
	m.action.assign(d.begin() + long(pos), d.begin() + long(pos) + long(m.size) * m.size);
	pos += size_t(m.size) * m.size;
	m.stringsOffset = w16(d, pos); m.monsterNamesOffset = w16(d, pos + 2); m.monsterDataOffset = w16(d, pos + 4);
	for (int i = 0; i < 16; ++i) m.actionClassOffset[i] = w16(d, pos + 6 + 2 * size_t(i));
	m.nibble6Offset = w16(d, pos + 38); m.npcOffset = w16(d, pos + 40);
	pos += 44;
	++pos;   // map size byte
	m.info = {d[pos], d[pos + 1], d[pos + 2], d[pos + 3], d[pos + 4], d[pos + 5], d[pos + 6], w16(d, pos + 7), d[pos + 9]};
	pos += 10;
	std::memcpy(m.battleStrings, d.data() + pos, 37);

	// tile map: u32 size, u32 unknown, then a Huffman tree and its data
	{
		BitReader r(d.data(), d.size(), m.tilemapOffset + 8);
		HuffmanTree t(r);
		m.tiles.resize(size_t(m.size) * m.size);
		for (auto &b : m.tiles) { const int v = t.decode(r); b = uint8_t(v < 0 ? 0 : v); }
	}
	// monsters
	if (m.monsterDataOffset) {
		const size_t qty = (m.stringsOffset - m.monsterDataOffset) / 8;
		size_t np = m.monsterNamesOffset, dp = m.monsterDataOffset;
		for (size_t i = 0; i < qty; ++i) {
			MapMonster mm{};
			while (d.at(np)) mm.name += char(d[np++]);
			++np;
			mm.experience = w16(d, dp);
			mm.skill = d[dp + 2]; mm.randomDamage = d[dp + 3];
			mm.maxGroupSize = d[dp + 4] >> 4; mm.ac = d[dp + 4] & 15;
			mm.fixedDamage = d[dp + 5] >> 4; mm.weaponType = d[dp + 5] & 15;
			mm.monsterType = d[dp + 6]; mm.picture = d[dp + 7];
			dp += 8;
			m.monsters.push_back(mm);
		}
	}
	// npcs
	if (m.npcOffset && w16(d, m.npcOffset) == 0) {
		const size_t first = w16(d, m.npcOffset + 2);
		const size_t qty = (first - m.npcOffset - 2) / 2;
		if (qty >= 1 && qty <= 255) {
			size_t np = first;
			for (size_t i = 0; i < qty && np + 0x100 <= d.size(); ++i, np += 0x100) {
				Npc c;
				c.raw.assign(d.begin() + long(np), d.begin() + long(np) + 0x100);
				m.npcs.push_back(std::move(c));
			}
		}
	}
	// strings: 60-byte char table, offsets table, 5-bit codes (LSB first) in groups of four
	{
		const size_t start = m.stringsOffset;
		const size_t end = m.tilemapOffset;
		const uint8_t *table = d.data() + start;
		const size_t q = start + 60;
		std::vector<size_t> offs;
		offs.push_back(w16(d, q));
		const size_t quantity = offs[0] / 2;
		for (size_t i = 1; i < quantity; ++i) {
			const size_t t = w16(d, q + 2 * i);
			if (t + start + 60 >= end || t < offs.back()) { if (i == quantity - 1) continue; break; }
			offs.push_back(t);
		}
		for (size_t g = 0; g < offs.size(); ++g) {
			BitReader r(d.data(), end, offs[g] + 60 + start);
			for (int j = 0; j < 4; ++j) {
				bool upper = false, high = false;
				std::string s;
				for (;;) {
					const int idx = r.bits(5, true);
					if (idx < 0) break;
					if (idx == 0x1F) { high = true; continue; }
					if (idx == 0x1E) { upper = true; continue; }
					const int ch = table[idx + (high ? 0x1E : 0)];
					if (ch == 0) break;
					char c = char(ch);
					if (upper && c >= 'a' && c <= 'z') c = char(c - 32);
					s += c;
					upper = high = false;
				}
				m.strings.push_back(s);
			}
		}
	}
	if (m.nibble6Offset)
		for (int i = 0; i < 128; ++i) m.specialActions.push_back(w16(d, m.nibble6Offset + 2 * size_t(i)));
	return m;
}

GameFile loadGameFile(const std::string &path) {
	GameFile g;
	g.data = shared::readFile(path);
	g.blocks = splitMsq(g.data);
	for (size_t i = 0; i < g.blocks.size(); ++i) {
		const int t = blockType(g.data, g.blocks[i]);
		if (t == 0) g.maps.push_back(parseMapBlock(g.data, g.blocks[i]));
		else if (t == 1) g.saveBlocks.push_back(i);
		else g.shopBlocks.push_back(i);
	}
	return g;
}

} // namespace wl

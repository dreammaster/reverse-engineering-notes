// Shops (KEH shopLoop/shopBuy/shopSell, script op 37) and paid services (serviceLoop, op 45).
#include "fod/progress.h"
#include <algorithm>
#include <cstdio>

namespace fod {

namespace {

uint32_t rd32(const uint8_t *p) { return uint32_t(p[0]) | (uint32_t(p[1]) << 8) | (uint32_t(p[2]) << 16) | (uint32_t(p[3]) << 24); }
std::string fixedStr(const uint8_t *p, size_t n) { return std::string(reinterpret_cast<const char *>(p), strnlen(reinterpret_cast<const char *>(p), n)); }

struct PortraitGuard {   // KEH showShopHeader: the picture stays up while the screen is open
	Host &h;
	PortraitGuard(Host &host, int id) : h(host) { h.showPortrait(id); }
	~PortraitGuard() { h.hidePortrait(); }
};

std::string pad(const std::string &s, size_t n) { return s.size() >= n ? s.substr(0, n) : s + std::string(n - s.size(), ' '); }

std::string cashLine(const World &w, const Member &m) {
	return m.name() + ", you have $" + std::to_string(w.disk.cash) + "\r";
}

// F1..F5 select another conscious member (KEH selectMemberByFKey).
bool selectMember(World &w, int key, int &member) {
	const int s = key - 0x3B;
	if (s < 0 || s >= w.partyCount() || World::conState(w.party(s)) >= 1) return false;
	member = s;
	return true;
}

void shopBuy(World &w, Host &h, Bytes &shop, int &member) {
	for (;;) {
		Member &m = w.party(member);
		std::vector<int> rows;
		const int count = shop[0x2A] | (shop[0x2B] << 8);
		for (int i = 0; i < count; ++i)
			if (shop[0x2C + 0x10 * size_t(i) + 1] != 0) rows.push_back(i);
		if (rows.empty()) { h.message("We have nothing more to sell."); return; }
		std::vector<std::string> labels;
		for (int i : rows) {
			const uint8_t *row = &shop[0x2C + 0x10 * size_t(i)];
			labels.push_back(pad(w.item(row[0]).name, 16) + " " + std::to_string(rd32(row + 6)));
		}
		const int pick = h.pickFromList(cashLine(w, m) + "Buy which item:", labels);
		if (pick < 0) return;
		uint8_t *row = &shop[0x2C + 0x10 * size_t(rows[size_t(pick)])];
		const uint32_t price = rd32(row + 6);
		if (price > w.disk.cash) { h.message("You do not have enough cash for that."); continue; }
		if (m.itemCount() >= 32) { h.message("Your inventory is full."); continue; }
		h.message(m.name() + " buys 1 " + w.item(row[0]).name + ".");
		w.initItemSlot(m, m.itemCount(), row[0]);
		++m.itemCount();
		w.disk.cash -= price;
		--row[1];
	}
}

void shopSell(World &w, Host &h, Bytes &shop, int &member) {
	bool sold = false;
	for (;;) {
		Member &m = w.party(member);
		const int count = shop[0x2A] | (shop[0x2B] << 8);
		std::vector<std::pair<int, int>> rows;   // (member item slot, shop row)
		for (int row = 0; row < count; ++row) {
			const uint8_t *r = &shop[0x2C + 0x10 * size_t(row)];
			for (int slot = 0; slot < m.itemCount(); ++slot)
				if (m.itemId(slot) == r[0] && r[1] != 0xFF) rows.emplace_back(slot, row);
		}
		if (rows.empty()) {
			if (!sold) h.message("You have nothing we want.");
			return;
		}
		std::vector<std::string> labels;
		for (const auto &pr : rows) {
			const uint8_t *r = &shop[0x2C + 0x10 * size_t(pr.second)];
			labels.push_back(pad(w.item(r[0]).name, 15) + " " + std::to_string(rd32(r + 2)));
		}
		const int pick = h.pickFromList(cashLine(w, m) + "Sell which item?", labels);
		if (pick < 0) return;
		const auto [slot, row] = rows[size_t(pick)];
		uint8_t *r = &shop[0x2C + 0x10 * size_t(row)];
		h.message(m.name() + " sells 1 " + w.item(m.itemId(slot)).name + ".");
		w.removeItemSlot(m, slot, 0xFF);
		++r[1];
		w.disk.cash += rd32(r + 2);
		sold = true;
	}
}

} // namespace

void runShop(World &w, Host &h, int shopIndex) {
	auto &shops = w.map->shops();
	if (size_t(shopIndex) >= shops.size()) return;
	Bytes &shop = shops[size_t(shopIndex)];
	PortraitGuard portrait(h, shop[0x28]);
	h.print("\r" + fixedStr(&shop[0x14], 20) + "\r");
	int member = 0;
	for (int i = 0; i < w.partyCount(); ++i)
		if (World::conState(w.party(i)) < 1) { member = i; break; }
	for (bool leave = false; !leave;) {
		h.showMenu(cashLine(w, w.party(member)) + "B)uy    S)ell    L)eave", false);
		const int k = h.readKey();
		if (k >= 0x3B && k <= 0x3F) selectMember(w, k, member);
		else if (k == 'B') shopBuy(w, h, shop, member);
		else if (k == 'S') shopSell(w, h, shop, member);
		else if (k == 'L' || k == 0xFF) leave = true;
	}
}

void runService(World &w, Host &h, int serviceIndex) {
	const Bytes &file = w.data.services;
	size_t base = 0;
	for (int i = 0; i < serviceIndex; ++i) {
		if (base + 0x2C > file.size()) return;
		base += 0x2C + 0x20 * size_t(file[base + 0x2A] | (file[base + 0x2B] << 8));
	}
	if (base + 0x2C > file.size()) return;
	const int count = file[base + 0x2A] | (file[base + 0x2B] << 8);
	PortraitGuard portrait(h, file[base + 0x28]);
	h.print("\r" + fixedStr(&file[base + 0x14], 20) + "\r");
	int member = 0;
	for (int i = 0; i < w.partyCount(); ++i)
		if (World::conState(w.party(i)) < 5) { member = i; break; }
	auto readService = [&](const Bytes &f) { (void)f; };
	(void)readService;

	// work on a mutable copy only for reading rows
	for (bool leave = false; !leave;) {
		Member &m = w.party(member);
		const Profession &p = w.data.archetypes.professions().at(m.profession());
		std::vector<std::string> labels;
		for (int i = 0; i < count; ++i) {
			const uint8_t *row = &file[base + 0x2C + 0x20 * size_t(i)];
			labels.push_back(pad(fixedStr(row, 16), 16) + " " + std::to_string(rd32(row + 0x16)));
		}
		const int pick = h.pickFromList(cashLine(w, m).substr(0, cashLine(w, m).size() - 1), labels);
		if (pick < 0) break;
		const uint8_t *row = &file[base + 0x2C + 0x20 * size_t(pick)];
		const uint8_t effectKind = row[0x14], effectParam = row[0x15];
		const uint32_t cost = rd32(row + 0x16);
		const uint8_t reqKind = row[0x1A], reqIdx = row[0x1B];
		const uint32_t reqValue = rd32(row + 0x1C);

		uint64_t have = 0;
		switch (reqKind) {
		case 0: have = m.b[0x4E]; break;
		case 1: have = m.exp(); break;
		case 2: have = uint16_t(m.hp()); break;
		case 3: have = uint16_t(m.hpMax()); break;
		case 4: have = m.b[0x18 + reqIdx]; break;
		case 5: have = m.b[0x24 + reqIdx]; break;
		case 6: have = m.b[0x34 + reqIdx]; break;
		default: have = 0; break;
		}
		if (cost > w.disk.cash) { h.message("You do not have the funds for that."); continue; }
		if (have < reqValue) { h.message("That service is not available to you at this time."); continue; }

		bool ok = false;
		const uint8_t skillCap = p.raw[0x2C + (w.data.hdspct.at(0x3E6) | (w.data.hdspct.at(0x3E7) << 8))];
		switch (effectKind) {
		case 0: if (m.b[0x4E] < 100) { ++m.b[0x4E]; ok = true; } break;
		case 1: if (m.exp() < 9999999u) { m.setU32(0x5A, m.exp() + 1); ok = true; } break;
		case 2: if (m.hp() < m.hpMax()) { m.setHp(m.hp() + 1); ok = true; } break;
		case 3: if (uint16_t(m.hpMax()) < (p.raw[0x74] | (p.raw[0x75] << 8))) { m.setHpMax(m.hpMax() + 1); ok = true; } break;
		case 4: if (p.raw[0x2C + reqIdx] > m.b[0x18 + effectParam]) { ++m.b[0x18 + effectParam]; ok = true; } break;
		case 5: if (skillCap > m.b[0x24 + effectParam]) { ++m.b[0x24 + effectParam]; ok = true; } break;
		case 6: if (skillCap > m.b[0x34 + effectParam]) { ++m.b[0x34 + effectParam]; ok = true; } break;
		case 7:
			if (m.b[0x58] & (1u << effectParam)) {
				m.b[0x58] &= uint8_t(~(1u << effectParam));
				if (effectParam == 7) m.b[0x21] = m.b[0x54] = m.b[0x57] = m.b[0x53] = 0;
				ok = true;
			}
			break;
		}
		if (ok) w.disk.cash -= cost;
		else h.message("You have no need for this service.");
	}
}

} // namespace fod

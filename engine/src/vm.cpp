// Port of KEH.EXE doScript (0x198F4). Control flow, condition chaining and the opcode semantics follow the
// disassembly; UI-dependent opcodes call into Host. Opcodes marked "approx" mirror the original closely but
// skip purely cosmetic redraws.
#include "fod/vm.h"
#include "fod/progress.h"
#include <algorithm>
#include <cctype>
#include <cstring>

namespace fod {

namespace {

struct Cond {                        // KEH var_132[]/var_E6[]/var_74/var_48
	bool val[16] = {};
	bool elseTaken[16] = {};
	int depth = 0;
	bool acc = false;
	int prevMode = 0;
	bool combine(bool v, int mode) {  // KEH combineCondition (0x1972A)
		switch (prevMode) {
		case 0: acc = v; break;
		case 1: acc = acc && v; break;
		case 2: acc = acc || v; break;
		default: break;
		}
		prevMode = mode;
		return acc;
	}
};

int64_t sx(int v) { return int16_t(v); }

} // namespace

ScriptResult ScriptVM::run(const ScriptCall &call) {
	ScriptResult res;
	if (call.cellScript < 0 || call.cellScript == 0xFFFF || !_w.scripts || !_w.scripts->has(unsigned(call.cellScript))) return res;
	const ScriptBlock blk = _w.scripts->block(unsigned(call.cellScript));
	if (!(blk.eventMask & (1u << call.event))) return res;
	res.ran = true;

	World &w = _w;
	Disk1 &disk = w.disk;
	int cur = call.member;
	int savedMember = 0;
	int64_t acc = 0;
	int reg = 0;
	Cond cond;
	if (call.event == kEvSkill) cond.val[0] = call.argA != 0;
	if (!_h.npcMode() && call.event == kEvMonster) _h.beginEncounter(call.arg8);
	if (_h.npcMode()) {
		cur = 0;
		for (int i = 0; i < w.partyCount(); ++i)
			if (World::conState(w.party(i)) < 1) { cur = i; break; }
	}
	std::vector<Frame> frames;
	int menuDepth = 0;
	bool done = false;
	std::vector<std::pair<int, int>> skillSuccess;   // (skill index, difficulty) pending improvement

	// offset -> record index (jump targets are block-relative byte offsets)
	auto indexOf = [&](size_t offset) -> size_t {
		for (size_t i = 0; i < blk.records.size(); ++i)
			if (blk.records[i].offset >= offset) return i;
		return blk.records.size();
	};
	const auto &recs = blk.records;
	auto strAt = [&](int i) -> std::string {
		std::string s = i >= 0 ? w.scripts->string(unsigned(call.cellScript), unsigned(i)) : std::string();
		std::string out;
		for (char c : s) {
			if (c == '^') out += w.partyCount() ? w.party(std::min(cur, w.partyCount() - 1)).name() : "";
			else out += c;
		}
		return out;
	};
	auto selSlot = [&](int sel) { return w.resolveMember(sel, cur); };
	auto stat = [&](int sel, int kind, int idx) { return w.getStat(sel, kind, idx, cur); };

	size_t pc = 0;
	int guard = 0;
	while (!done && pc < recs.size() && ++guard < 200000) {
		const ScriptRecord &r = recs[pc++];
		if (r.eventClass != call.event) continue;
		auto P = [&](size_t i) -> int { return i < r.params.size() ? r.params[i] : 0; };
		const int p0 = P(0), p1 = P(1), p2 = P(2), p3 = P(3), p4 = P(4);
		bool *cv = &cond.val[std::min(cond.depth, 15)];

		switch (r.opcode) {
		case 1: {  // calc
			int64_t operand = (p0 & 1) == 0 ? sx(reg) : p1;
			switch (p0) {
			case 0: case 1: acc += operand; break;
			case 2: case 3: acc -= operand; break;
			case 4: case 5: if (operand) acc /= operand; break;
			case 6: case 7: acc *= operand; break;
			}
			acc = int32_t(acc);
			break;
		}
		case 2:    // move acc <-> reg
			switch (p0) {
			case 0: reg = int16_t(acc); break;
			case 1: acc = sx(reg); break;
			case 2: reg = 0; break;
			case 3: reg = int16_t(reg + int16_t(acc)); break;
			case 4: reg = int16_t(reg - int16_t(acc)); break;
			}
			break;
		case 3: acc = p0; break;
		case 4: acc = stat(p0, p1, p2); reg = int16_t(acc); break;
		case 5: w.setStat(p0, p1, p2, int32_t(acc), cur); checkLevelUps(w, _h); break;
		case 6: case 7: {
			int64_t best = w.getStat(1, p0, p1, cur);
			for (int s = 1; s <= w.partyCount(); ++s) {
				if (r.opcode == 7 && w.party(s - 1).hp() <= 0) continue;
				const int64_t val = w.getStat(s, p0, p1, cur);   // selector s (1-based) -> slot s-1
				if (r.opcode == 6 ? val > best : val < best) best = val;
			}
			acc = best;
			break;
		}
		case 8: acc = disk.cash; break;
		case 9: {   // has item
			const int slot = selSlot(p0);
			const bool has = slot >= 0 && slot < w.partyCount() && w.hasItem(w.party(slot), p1);
			*cv = has;
			*cv = cond.combine(has, p2);
			break;
		}
		case 10: {  // unique item / anyone has item
			bool v = false;
			if (w.isUnique(p0)) v = (disk.uniqueItems & (1u << w.item(p0).uniqueBit)) != 0;
			else
				for (int s = 0; s < w.partyCount() && !v; ++s) v = w.hasItem(w.party(s), p0);
			*cv = cond.combine(v, p1);
			break;
		}
		case 11: {  // item housekeeping
			const int slot = selSlot(p0);
			if (slot < 0 || slot >= w.partyCount()) break;
			Member &m = w.party(slot);
			auto removeWithFlag = [&](int mask) {
				for (int i = m.itemCount() - 1; i >= 0; --i)
					if (w.item(m.itemId(i)).flags & mask) w.removeItemSlot(m, i, 0xFF);
			};
			if (p1 < 4) {
				if (p1 == 0) removeWithFlag(0x80);
				else if (p1 == 1) removeWithFlag(1);
				else if (p1 == 2) removeWithFlag(2);
				else
					for (int i = m.itemCount() - 1; i >= 0; --i)
						if (!(w.item(m.itemId(i)).flags & 3)) w.removeItemSlot(m, i, 0xFF);
			} else {
				const int id = p1 - 4;
				if (w.isUnique(id)) disk.uniqueItems &= uint8_t(~(1u << w.item(id).uniqueBit));
				else
					for (int i = 0; i < m.itemCount(); ++i)
						if (m.itemId(i) == id) { w.removeItemSlot(m, i, 0xFF); break; }
			}
			break;
		}
		case 12: {  // change map
			bool go = p3 != 0 || _h.yesNo("Enter new location?");
			if (!go) break;
			if (p2 == 0) {
				if (disk.mapIndex > 0) {
					w.syncChangesToDisk();
					--disk.mapIndex;
					w.loadMap(disk.maps[disk.mapIndex]);
					w.setPartyPosition(disk.mapsX[disk.mapIndex], disk.mapsY[disk.mapIndex]);
				}
			} else if (p2 == 1) {
				w.setPartyPosition(p0, p1);
			} else {
				const int newMap = p2 - 1;
				w.syncChangesToDisk();
				disk.mapsX[disk.mapIndex] = uint8_t(disk.mapX);
				disk.mapsY[disk.mapIndex] = uint8_t(disk.mapY);
				bool found = false;
				unsigned at = 0;
				for (unsigned i = 0; i <= disk.mapIndex; ++i)
					if (disk.maps[i] == newMap) { found = true; at = i; }
				if (found) disk.mapIndex = uint8_t(at);
				else {
					++disk.mapIndex;
					disk.maps[disk.mapIndex] = uint8_t(newMap);
				}
				disk.mapsX[disk.mapIndex] = uint8_t(p0);
				disk.mapsY[disk.mapIndex] = uint8_t(p1);
				w.loadMap(newMap);
				w.setPartyPosition(p0, p1);
			}
			res.changedMap = true;
			res.request = p3 == 2 ? 0 : -1;
			_h.redrawMap();
			return res;
		}
		case 13:   // for-each-member loop begin
			savedMember = cur;
			cur = 0;
			frames.push_back({pc, indexOf(size_t(p0)), cond.depth});
			break;
		case 14:   // next member
			if (!frames.empty() && cur < w.partyCount() - 1) {
				++cur;
				pc = frames.back().loopStart;
			} else {
				cur = savedMember;
				if (!frames.empty()) frames.pop_back();
			}
			break;
		case 67:   // break out of loop
			if (!frames.empty()) {
				pc = frames.back().loopEnd;
				frames.pop_back();
			}
			break;
		case 15: {  // text input
			_h.print("\r>");
			_typed = _h.readLine();
			break;
		}
		case 16: {  // compare typed text with script string
			auto lower = [](std::string s) { for (auto &c : s) c = char(std::tolower((unsigned char)c)); return s; };
			*cv = lower(_typed) == lower(strAt(p0));
			break;
		}
		case 17: case 70: {   // damage party now (70: ignore armor)
			const int lo = p1, hi = p2;
			_h.print(strAt(p0));
			for (int s = 0; s < w.partyCount(); ++s) {
				Member &m = w.party(s);
				if (World::conState(m) == 5) continue;
				int dmg = w.rng.range(std::min(lo, hi), std::max(lo, hi));
				m.setHp(m.hp() - dmg);
				_h.print(m.name() + " takes " + std::to_string(dmg) + " pts of damage.\r");
			}
			break;
		}
		case 0: case 41: case 71: {  // hazard (approx): store parameters and apply once
			_hazardMin = p1; _hazardMax = p2; _hazardInterval = p3;
			_h.print(strAt(p0));
			for (int s = 0; s < w.partyCount(); ++s) {
				Member &m = w.party(s);
				if (World::conState(m) == 5) continue;
				m.setHp(m.hp() - w.rng.range(std::min(p1, p2), std::max(p1, p2)));
			}
			break;
		}
		case 18:   // heal each member by p0
			for (int s = 0; s < w.partyCount(); ++s) {
				Member &m = w.party(s);
				m.setHp(std::min<int>(m.hp() + w.rng.range(p0, p0), m.hpMax()));
			}
			break;
		case 19: {  // compare
			bool v = false;
			if (p0 < 12) {
				const int64_t a = p0 < 6 ? p2 : sx(reg);
				switch (p0 % 6) {
				case 0: v = acc == a; break;
				case 1: v = acc > a; break;
				case 2: v = acc < a; break;
				case 3: v = acc >= a; break;
				case 4: v = acc <= a; break;
				case 5: v = acc != a; break;
				}
			} else v = acc == (p0 < 15 ? p0 - 12 : p0 - 15);
			*cv = cond.combine(v, p1);
			break;
		}
		case 20: {  // IF
			const bool c = *cv;
			const bool jump = (p0 & 1) == 0 ? !c : c;
			if (jump) {
				cond.elseTaken[std::min(cond.depth, 15)] = true;
				pc = indexOf(size_t(p1));
			} else {
				cond.elseTaken[std::min(cond.depth, 15)] = false;
				++cond.depth;
			}
			break;
		}
		case 21:   // ELSE
			if (cond.elseTaken[std::min(cond.depth, 15)]) ++cond.depth;
			else pc = indexOf(size_t(p0));
			break;
		case 22: if (cond.depth > 0) --cond.depth; break;
		case 23:   // print string
			_h.print("\r\r");
			_h.print(strAt(p0));
			break;
		case 24:   // drop loot packet p0-1 at (p1, p2)
			w.dropPacketLoot(p0 - 1, p1, p2);
			break;
		case 25: w.applyCellChange(disk.mapX, disk.mapY, 0x19, p0, 0); _h.redrawMap(); break;
		case 26: w.applyCellChange(disk.mapX, disk.mapY, 0x1A, p0, p1); break;
		case 27: w.applyCellChange(disk.mapX, disk.mapY, 0x1B, p0, 0); break;
		case 28: case 29: case 30:
			w.recordCellChange(r.opcode, p2, p0, p1, p3, p4);
			_h.redrawMap();
			break;
		case 31: _h.sound(p0); break;
		case 32: acc = w.rng.range(p0, p1); break;
		case 34: {  // skill / attribute check (approx of the party-wide check)
			int kind = 4, idx = p0;
			if (p0 >= 0x1C) { kind = 6; idx = p0 - 0x1C; }
			else if (p0 >= 0x0C) { kind = 5; idx = p0 - 0x0C; }
			const int difficulty = p1;
			bool ok = false;
			if (call.event == kEvUseItem) {
				*cv = false;
				if (call.arg8 == p0) {
					ok = w.getStat(0, kind, idx, cur) >= difficulty;
					if (!ok && kind == 5 && cur < w.partyCount()) {
						const int chance = World::attrBonus(w.party(cur).attr(4)) * 2;
						ok = chance > 0 && w.rng.range(1, 100) <= chance;
					}
				}
			} else {
				int best = -1;
				int bestMember = 0;
				for (int s = 0; s < w.partyCount(); ++s) {
					const int v = int(w.getStat(s + 1, kind, idx, cur));
					if (v > best) { best = v; bestMember = s; }
				}
				cur = bestMember;
				ok = best >= difficulty;
				if (!ok && kind != 4) {   // luck roll from the best living member's attribute
					int bestBonus = 0;
					for (int s = 0; s < w.partyCount(); ++s) {
						const Member &m = w.party(s);
						if (w.conState(m) == 0 && (kind == 5 ? m.b[0x24 + idx] : m.b[0x34 + idx]))
							bestBonus = std::max(bestBonus, int(m.attr(kind == 5 ? 4 : 1)));
					}
					const int chance = World::attrBonus(bestBonus) * 2;
					ok = chance > 0 && w.rng.range(1, 100) < chance;
				}
			}
			*cv = cond.combine(ok, p2);
			if (ok && kind != 4) skillSuccess.emplace_back(idx, difficulty);
			if (p2 == 0)
				for (auto &sd : skillSuccess) (void)sd;   // skill improvement pass omitted (cosmetic progression)
			if (p2 == 0) skillSuccess.clear();
			break;
		}
		case 35: {  // readied slot holds item p0
			bool v = false;
			if (cur < w.partyCount()) v = call.arg8 < w.party(cur).itemCount() && w.party(cur).itemId(call.arg8) == p0;
			*cv = cond.combine(v, p1);
			break;
		}
		case 36: *cv = cond.combine(call.arg8 == p0 && call.event == kEvUseItem, p1); break;
		case 37: _h.shop(p0, cur); break;
		case 38: break;     // hide party sprite (cosmetic)
		case 39: *cv = _h.yesNo(strAt(p0)); break;
		case 40: w.setPartyPosition(disk.mapX, disk.mapY); break;
		case 42:   // set monster state by state value
			for (auto &m : w.map->monsters())
				if (m.raw[0x5D] == p0) m.raw[0x5D] = uint8_t(p1);
			break;
		case 43: {  // roster member p0 is in the party and alive
			bool v = false;
			if (disk.tail[size_t(p0)] & 1) {
				for (int s = 0; s < w.partyCount(); ++s)
					if (w.party(s).rosterId() == p0) v = w.party(s).hp() > 0;
			}
			*cv = cond.combine(v, p1);
			break;
		}
		case 44: *cv = cond.combine((disk.tail[size_t(p0)] & 2) != 0, p1); break;
		case 45: _h.service(p0, cur); break;
		case 46: acc = disk.hours * 100 + disk.minutes; break;
		case 47: {  // menu begin: show text, read a key
			++menuDepth;
			const bool npc = _h.npcMode();
			for (;;) {
				_h.showMenu(strAt(p0), npc);
				const int k = _h.readKey();
				if (k >= '1' && k <= '9') { _lastKey = k - '0'; break; }
				if (k == 0xFF && menuDepth > 1) {
					frames.push_back({pc - 1, indexOf(size_t(p1)), cond.depth});
					pc = frames.back().loopEnd;
					frames.pop_back();
					--menuDepth;
					_lastKey = 0xFF;
					break;
				}
				if (npc) {
					if (k == 'A') { res.request = 8; res.monsterIndex = call.arg8; done = true; break; }
					if (k >= 0x3B && k <= 0x3F) {
						const int sel = k - 0x3B;
						if (sel < w.partyCount() && World::conState(w.party(sel)) < 1) cur = sel;
						continue;
					}
					if (k == 'H') {
						ScriptCall c2 = call;
						c2.event = kEvHire; c2.member = cur;
						if (run(c2).ran) { done = true; break; }
						_h.message("I am not interested in joining up with the likes of you");
						_lastKey = 0xFF;
						break;
					}
					if (k == 'L') {
						auto &ms = w.map->monsters();
						if (size_t(call.arg8) < ms.size() && (ms[size_t(call.arg8)].raw[0x50] & 0x80)) {
							Monster &m = ms[size_t(call.arg8)];
							w.setMapCellHighBits(m.x(), m.y(), 0);
							m.raw[0x5B] = 0x80; m.raw[0x5C] = 0x80;
						}
						done = true;
						break;
					}
					if (k == 'U') {
						int slot = 0;
						const int mem = _h.chooseUseItem(cur, slot);
						_lastKey = 0xFF;
						if (mem >= 0) {
							cur = mem;
							ScriptCall c2 = call;
							c2.event = 2; c2.member = mem; c2.arg8 = slot;
							if (!run(c2).ran) _h.message("That doesn't affect me!");
						}
						break;
					}
					continue;
				}
				if (k == 'L' || k == 0xFF) { done = true; break; }
			}
			// remember the menu start for op 49 (return to menu) and end for op 48
			frames.push_back({pc - 1, indexOf(size_t(p1)), cond.depth});
			break;
		}
		case 48:   // leave the menu
			if (!frames.empty()) {
				pc = frames.back().loopEnd;
				cond.depth = frames.back().condDepth;
				frames.pop_back();
			}
			--menuDepth;
			_lastKey = 0;
			_h.redrawMap();
			break;
		case 49:   // back to the menu
			if (!frames.empty()) {
				pc = frames.back().loopStart;
				frames.pop_back();
			}
			--menuDepth;
			break;
		case 50: {  // reveal hidden ground items at (p0, p1)
			bool any = false;
			for (auto &gi : w.map->groundItems())
				if ((gi.x & 0x7F) == p0 && gi.y == p1) { gi.x &= 0x7F; any = true; }
			if (any && w.map->inBounds(p0, p1)) w.map->cell(p0, p1).flags |= 0x80;
			_h.redrawMap();
			break;
		}
		case 55: *cv = _lastKey == p0; break;
		case 56: {  // give item
			const int slot = selSlot(p0);
			if (slot < 0 || slot >= w.partyCount()) break;
			if (w.isUnique(p1)) disk.uniqueItems |= uint8_t(1u << w.item(p1).uniqueBit);
			else w.addItem(w.party(slot), p1);
			break;
		}
		case 57: _h.message(strAt(p0)); break;
		case 59: {  // replace/remove item p1 by p2
			const int slot = selSlot(p0);
			if (slot < 0 || slot >= w.partyCount()) break;
			Member &m = w.party(slot);
			for (int i = 0; i < m.itemCount(); ++i)
				if (m.itemId(i) == p1) { w.removeItemSlot(m, i, p2); break; }
			break;
		}
		case 60: acc = disk.weekday; break;
		case 63: {  // status bit test
			const int slot = selSlot(p0);
			const bool v = slot >= 0 && slot < w.partyCount() && (w.party(slot).status() & (1u << p1));
			*cv = cond.combine(v, p2);
			break;
		}
		case 64: { const int s = selSlot(p0); if (s >= 0 && s < w.partyCount()) w.party(s).status() |= uint8_t(1u << p1); break; }
		case 65: { const int s = selSlot(p0); if (s >= 0 && s < w.partyCount()) w.party(s).status() &= uint8_t(~(1u << p1)); break; }
		case 66: {  // pack full?
			const int slot = selSlot(p0);
			int used = 0;
			if (slot >= 0 && slot < w.partyCount()) {
				const Member &m = w.party(slot);
				for (int i = 0; i < m.b[0x52]; ++i) if (m.b[0x62 + 6 * i] != 0xFF) ++used;
			}
			*cv = cond.combine(used >= 32, p1);
			break;
		}
		case 68: done = true; break;
		case 69: disk.facing = uint16_t(p0); break;
		case 72: {   // retrieve / store a party member on this map
			for (bool leave = false; !leave;) {
				_h.showMenu("What do you wish?\r\r\r1. Retrieve a member\r2. Store a member\r\rL)eave", false);
				const int k = _h.readKey();
				auto &stored = w.map->storedMembers();
				if (k == '1') {
					if (stored.empty()) { _h.message("\rThere's no one here!"); continue; }
					if (w.partyCount() >= 5) { _h.message("\rYour party is full!"); continue; }
					std::vector<std::string> names;
					for (const Bytes &b : stored) names.push_back(std::string(reinterpret_cast<const char *>(b.data()), strnlen(reinterpret_cast<const char *>(b.data()), 24)));
					const int pick = _h.pickFromList("Pick up whom?", names);
					if (pick < 0) continue;
					const int slot = w.findFreeRosterSlot();
					std::memcpy(w.disk.roster[size_t(slot)].b.data(), stored[size_t(pick)].data(), 0x14C);
					w.disk.partyIds[w.disk.partyCount++] = uint8_t(slot);
					w.disk.tail[w.disk.roster[size_t(slot)].rosterId()] |= 1;
					stored.erase(stored.begin() + pick);
				} else if (k == '2') {
					if (w.partyCount() == 1) { _h.message("\rWith you gone, who's gonna type?"); continue; }
					if (stored.size() >= 3) { _h.message("\rWe're full right now!"); continue; }
					std::vector<std::string> names;
					for (int i = 0; i < w.partyCount(); ++i) names.push_back(w.party(i).name());
					const int pick = _h.pickFromList("Who will stay?", names);
					if (pick < 0) continue;
					stored.emplace_back(w.party(pick).b.begin(), w.party(pick).b.end());
					w.removeMemberFromParty(pick);
				} else if (k == 'L' || k == 0xFF) {
					leave = true;
				}
			}
			_h.redrawMap();
			break;
		}
		case 73:
			if (w.map->inBounds(p0, p1)) {
				MapCell &c = w.map->cell(p0, p1);
				c.tile = uint16_t((c.tile & 0x07FF) | (p2 << 11));
			}
			_h.redrawMap();
			break;
		case 74: _h.ending(p0); done = true; break;
		case 62: {  // push party one step in direction p0+1
			static const int dx[6] = {0, 0, 0, 1, -1, 0}, dy[6] = {0, -1, 1, 0, 0, 0};
			const int d = std::min(p0 + 1, 5);
			w.setPartyPosition(disk.mapX + dx[d], disk.mapY + dy[d]);
			_h.redrawMap();
			res.changedMap = false;
			done = true;
			break;
		}
		case 52: case 75: {   // move a monster / start a scripted fight with it
			size_t mi = 0;
			Monster *m = w.findMonsterById(p0, mi);
			if (!m || (disk.tail[m->id()] & 2)) break;
			if (r.opcode == 52) {
				w.setMapCellHighBits(m->x(), m->y(), 0);
				m->raw[0x5B] = uint8_t(p1); m->raw[0x5C] = uint8_t(p2);
				w.markEntityOnMap(m->raw[0x5F], m->x(), m->y(), m->raw[0x65]);
			}
			m->raw[0x60] = 0xFF;
			if (p3 == 1 || r.opcode == 75) {
				if (p3 == 1) m->raw[0x50] |= 0x80;
				res.request = 6;
				res.chainScript = m->script();
				res.monsterIndex = int(mi);
				done = true;
			}
			break;
		}
		case 54: {   // recruit the monster with this id
			*cv = false;
			size_t mi = 0;
			Monster *m = w.findMonsterById(p0, mi);
			if (m && !(disk.tail[m->id()] & 3)) w.addNpcToParty(mi);
			done = true;
			break;
		}
		case 58: {   // give a monster a new script
			size_t mi = 0;
			Monster *m = w.findMonsterById(p0, mi);
			if (m) { m->raw[0x66] = uint8_t(p1); m->raw[0x67] = uint8_t(p1 >> 8); }
			break;
		}
		case 53: {   // spawn the inactive group placed at (p0, p1)
			if (p0 == disk.mapX && p1 == disk.mapY) break;
			auto &groups = w.map->groups();
			const size_t start = (w.map->encounterPct() != 0 && w.map->groupTemplates() != 0) ? size_t(w.map->groupTemplates()) + 8 : 0;
			for (size_t i = start; i < groups.size(); ++i) {
				EnemyGroup &g = groups[i];
				if ((g.raw[9] & 0x0F) || g.raw[0] != p0 || g.raw[1] != p1) continue;
				w.spawnEnemyGroup(g);
				break;
			}
			break;
		}
		case 61: {   // is the group placed at (p0, p1) alive?
			*cv = false;
			for (const EnemyGroup &g : w.map->groups())
				if (g.raw[0] == p0 && g.raw[1] == p1) { *cv = (g.raw[9] & 0x0F) != 0; break; }
			break;
		}
		default:
			break;     // 33 and 51 are no-ops in the original
		}
	}
	return res;
}

} // namespace fod

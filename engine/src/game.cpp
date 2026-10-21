#include "fod/game.h"
#include "fod/combat.h"
#include "fod/party.h"
#include "fod/charsheet.h"
#include "fod/savegame.h"
#include "fod/sound.h"
#include "fod/creator.h"
#include <algorithm>
#include <cstring>

namespace fod {

namespace {
const int kDx[6] = {0, 0, 0, 1, -1, 0};
const int kDy[6] = {0, -1, 1, 0, 0, 0};
}

Disk1 Game::defaultParty(const GameData &data) {
	Disk1 d = Disk1::newGame();
	const char *names[4] = {"Lena", "Ross", "Maya", "Dutch"};
	const auto &profs = data.archetypes.professions();
	for (int i = 0; i < 4; ++i) {
		Member &m = d.roster[i];
		const Profession &p = profs[size_t(i) % profs.size()];
		std::memcpy(m.b.data(), names[i], std::strlen(names[i]));
		m.u8(0x48) = uint8_t(i % profs.size());
		m.u8(0x4A) = 0xFF;   // nothing readied
		int points = p.attrPoints;
		for (int a = 0; a < 7; ++a) m.attr(a) = p.attrMin[a];
		for (int a = 0; points > 0; a = (a + 1) % 7) {
			if (m.attr(a) < p.attrMax[a]) { ++m.attr(a); --points; }
			bool any = false;
			for (int k = 0; k < 7; ++k) if (m.attr(k) < p.attrMax[k]) any = true;
			if (!any) break;
		}
		for (int k = 0; k < 16; ++k) { m.activeSkill(k) = p.activeSkills[k]; m.passiveSkill(k) = p.passiveSkills[k]; }
		const int hp = (p.hpMin + p.hpMax) / 2;
		m.setHpMax(hp); m.setHp(hp); m.setU16(0x122, uint16_t(hp));
	}
	d.partyCount = 4;
	finishNewParty(d);
	return d;
}

Game::Game(const GameData &data, const Assets &assets, Platform &platform, Disk1 disk,
           std::vector<std::pair<int, Bytes>> maps, std::string saveDir)
    : _data(data), _assets(assets), _platform(platform), _w(data, std::move(disk)), _vm(_w, *this), _saveDir(std::move(saveDir)) {
	_w.importMaps(maps);
	_w.loadMap(_w.disk.maps[_w.disk.mapIndex]);
}

// ---------------------------------------------------------------- drawing
void Game::redraw() {
	if (_rosterActive) drawRoster(_screen, _assets, _w);
	else if (_sheetActive && _sheetMember < _w.partyCount()) drawCharacterSheet(_screen, _assets, _w, _sheetMember, _sheet);
	else renderMainScreen(_screen, _assets, _w, {});
	if (_dialogOpen) {
		// dialog frame (KEH _dialogBorder*) and text area (8,6)-(31,17)
		for (int y = 5; y <= 18; ++y)
			for (int x = 7; x <= 32; ++x) {
				const bool edgeX = x == 7 || x == 32, edgeY = y == 5 || y == 18;
				int g = 0;
				if (edgeX && edgeY) g = 0x84;
				else if (edgeY) g = (x == 8 || x == 31) ? 0x80 : 0x81;
				else if (edgeX) g = (y == 6 || y == 17) ? 0x82 : 0xB5;
				if (g) drawGlyph(_screen, _assets, x * 8, y * 8, g);
				else for (int py = 0; py < 8; ++py) for (int px = 0; px < 8; ++px) _screen.put(x * 8 + px, y * 8 + py, 0);
			}
		int row = 0, col = 0;
		std::string word;
		auto flushWord = [&]() {
			if (word.empty()) return;
			if (col + int(word.size()) > 24) { col = 0; ++row; }
			if (row < 12) drawText(_screen, _assets, 8 + col, 6 + row, word);
			col += int(word.size());
			word.clear();
		};
		for (char c : _dialog) {
			if (c == '\r') { flushWord(); col = 0; ++row; }
			else if (c == ' ') { flushWord(); if (col < 24) ++col; }
			else word += c;
		}
		flushWord();
	}
	_platform.present(_screen);
}

void Game::openDialog() { if (!_dialogOpen) { _dialogOpen = true; _dialog.clear(); } }

// ---------------------------------------------------------------- Host
void Game::sound(int n) { _platform.playPcm(renderSoundEffect(n), 22050); }

void Game::print(const std::string &text) {
	openDialog();
	_dialog += text;
}

void Game::message(const std::string &text) {
	openDialog();
	_dialog += text;
	redraw();
	for (;;) {
		const int k = _platform.waitKey();
		if (k < 0) { _quit = true; break; }
		if (k == 0xFE || k == 0xFF) break;
	}
	_dialogOpen = false;
	redraw();
}

int Game::readKey() {
	redraw();
	const int k = _platform.waitKey();
	if (k < 0) { _quit = true; return 0xFF; }
	return k;
}

bool Game::yesNo(const std::string &prompt) {
	openDialog();
	_dialog += prompt + "\r\rY)es  N)o";
	redraw();
	for (;;) {
		const int k = _platform.waitKey();
		if (k < 0) { _quit = true; return false; }
		if (k == 'Y') return true;
		if (k == 'N' || k == 0xFF) return false;
	}
}

std::string Game::readLine() {
	std::string line;
	for (;;) {
		openDialog();
		redraw();
		const int k = _platform.waitKey();
		if (k < 0) { _quit = true; break; }
		if (k == 0xFE) break;
		if (k == 8 && !line.empty()) line.pop_back();
		else if (k >= 32 && k < 127 && line.size() < 10) line += char(k);
	}
	return line;
}

void Game::showMenu(const std::string &text, bool) {
	openDialog();
	_dialog += text;
	redraw();
}

// ---------------------------------------------------------------- game logic
void Game::passTime() {
	_w.passTime(false);
	if (_w.hazard.interval > 0 && _w.timeTicks % _w.hazard.interval == 0) _vm.applyRepeatingHazard();
}

void Game::beginEncounter(int monsterIndex) {
	_npcMode = true;
	openDialog();
	if (size_t(monsterIndex) < _w.map->monsters().size()) _dialog += _w.map->monsters()[size_t(monsterIndex)].name() + "\r";
}

int Game::pickFromList(const std::string &title, const std::vector<std::string> &items) {
	openDialog();
	_dialog += title + "\r";
	for (size_t i = 0; i < items.size() && i < 9; ++i) _dialog += std::to_string(i + 1) + ". " + items[i] + "\r";
	for (;;) {
		const int k = readKey();
		if (k >= '1' && k < '1' + int(std::min<size_t>(items.size(), 9))) return k - '1';
		if (k == 0xFF) return -1;
	}
}

int Game::chooseUseItem(int member, int &slot) {
	Member &m = _w.party(member);
	std::vector<std::string> names;
	std::vector<int> slots;
	for (int i = 0; i < m.itemCount() && i < 9; ++i) {
		names.push_back(_w.item(m.itemId(i)).name);
		slots.push_back(i);
	}
	if (names.empty()) return -1;
	const int pick = pickFromList(m.name() + " uses:", names);
	if (pick < 0) return -1;
	slot = slots[size_t(pick)];
	return member;
}

void Game::startPendingFight(int monsterIndex) {
	_npcMode = false;
	auto &ms = _w.map->monsters();
	if (size_t(monsterIndex) >= ms.size()) return;
	Monster &mo = ms[size_t(monsterIndex)];
	mo.raw[0x60] = mo.raw[0x5B];
	mo.raw[0x61] = mo.raw[0x5C];
	Combat cb(_w, *this);
	Combatant c;
	c.type = 1; c.index = uint8_t(monsterIndex); c.sub = 0;
	c.speed = uint8_t(mo.raw[0x1A] * 2);
	c.command = 0x0E;
	c.distance = uint8_t(cb.distanceToParty(int8_t(mo.raw[0x60]), int8_t(mo.raw[0x61])));
	cb.list.push_back(c);
	const unsigned speedAttr = (_data.hdspct.at(0x3E8) | (_data.hdspct.at(0x3E9) << 8)) & 7;
	cb.maxSpeed = mo.raw[0x18 + speedAttr];
	cb.commands = defaultCombatCommands();
	const Combat::Outcome o = cb.run(false);
	if (o.wiped) { print("\rYour party has been defeated.\r"); _quit = true; }
	redraw();
}

void Game::handleScriptResult(const ScriptResult &r) {
	if (r.request == 8) startPendingFight(r.monsterIndex);
	else if (r.request == 6) runScriptChain(r.chainScript, kEvMonster, r.monsterIndex);
}

void Game::runScriptChain(int script, int event, int arg8) {
	int curScript = script, curEvent = event, curArg = arg8;
	for (int guard = 0; guard < 32 && !_quit; ++guard) {
		ScriptCall c;
		c.cellScript = curScript;
		c.event = curEvent;
		c.member = 0;
		c.arg8 = curArg;
		const ScriptResult r = _vm.run(c);
		if (r.request == 8) { startPendingFight(r.monsterIndex); break; }
		if (r.request == 6) {
			curScript = r.chainScript;
			curEvent = kEvMonster;
			curArg = r.monsterIndex;
			continue;
		}
		break;
	}
	_npcMode = false;
}

std::function<Combat::GroupAction(Combat &, int, Combatant &)> Game::defaultCombatCommands() {
	return [this](Combat &c, int slot, Combatant &rec) {
		print("\r" + _w.party(slot).name() + ", choose: A)ttack E)vade R)un\r");
		const int k = readKey();
		if (k == 'E') { rec.command = 1; rec.speed = uint8_t(rec.speed * 2); return Combat::GroupAction::None; }
		if (k == 'R') {
			print("G)et closer  F)lee\r");
			const int k2 = readKey();
			if (k2 == 'G') return Combat::GroupAction::GetCloser;
			if (k2 == 'F') return Combat::GroupAction::Flee;
		}
		c.autoCommand(slot, rec);
		return Combat::GroupAction::None;
	};
}

// KEH useUpAmmo (0x11A55): fire one round / spend a one-shot item, re-readying a second copy if there is one.
void Game::useUpAmmoFor(int slot, int itemSlot) {
	Member &m = _w.party(slot);
	const WeaponDef &wd = _w.weaponOf(m.itemId(itemSlot));
	if (!(wd.flags() & 2)) return;
	if (wd.flags() & 1) {
		if (m.itemAmmo(itemSlot) > 0) --m.itemAmmo(itemSlot);
		return;
	}
	const int readiedItem = m.readied() == 0xFF ? 0xFF : m.itemId(m.readied());
	_w.removeItemSlot(m, itemSlot, 0xFF);
	if (readiedItem == 0xFF) return;
	for (int i = 0; i < m.itemCount(); ++i)
		if (m.itemId(i) == readiedItem) { m.readied() = uint8_t(i); break; }
}

void Game::endOfTurn() {
	Combat cb(_w, *this);
	_w.moveMapEntities();
	_w.placeEntitiesOnMap();
	int n = cb.buildCombatants(false);
	if (n <= 0 && _w.tryRandomEncounter()) {
		message("\r\rUh oh! Here comes trouble!");
		n = cb.buildCombatants(false);
	}
	if (n <= 0) return;
	cb.commands = defaultCombatCommands();
	const Combat::Outcome o = cb.run(false);
	if (o.wiped) {
		print("\rYour party has been defeated.\r");
		_quit = true;
	}
}

bool Game::askDirection(int &dx, int &dy) {
	print("\rWhich way?\r");
	for (;;) {
		const int k = readKey();
		if (k == 0xFF) return false;
		int d = -1;
		if (k == 0xFD) d = 1; else if (k == 0xFC) d = 2; else if (k == 0xFA) d = 3; else if (k == 0xFB) d = 4; else if (k == 0xFE) d = 5;
		if (d < 0) continue;
		dx = kDx[d]; dy = kDy[d];
		const int x = _w.disk.mapX + dx, y = _w.disk.mapY + dy;
		if (!_w.map->inBounds(x, y)) continue;
		return true;
	}
}

bool Game::pickMapCell(int &x, int &y) {
	print("\rWhich way?\r");
	int cx = _w.disk.mapX, cy = _w.disk.mapY;
	_w.map->cell(cx, cy).flags |= 0x40;
	bool ok = false;
	for (;;) {
		redraw();
		const int k = readKey();
		_w.map->cell(cx, cy).flags &= 0xBF;
		if (k == 0xFF) break;
		if (k == 0xFE) { ok = true; break; }
		int d = -1;
		if (k == 0xFD) d = 1; else if (k == 0xFC) d = 2; else if (k == 0xFA) d = 3; else if (k == 0xFB) d = 4;
		if (d > 0 && _w.map->inBounds(cx + kDx[d], cy + kDy[d])) { cx += kDx[d]; cy += kDy[d]; }
		_w.map->cell(cx, cy).flags |= 0x40;
	}
	_w.map->cell(cx, cy).flags &= 0xBF;
	x = cx; y = cy;
	return ok;
}

bool Game::chooseCommandTarget(int slot) {
	if (_npcMode) return true;
	Member &m = _w.party(slot);
	int x, y;
	if (m.b[0x144] == 0x0B && (_w.item(m.b[0x147]).flags & 1)) {
		if (!pickMapCell(x, y)) return false;
	} else {
		int dx, dy;
		if (!askDirection(dx, dy)) return false;
		x = _w.disk.mapX + dx; y = _w.disk.mapY + dy;
	}
	_targetX[slot] = x; _targetY[slot] = y;
	return true;
}

void Game::useItemScriptOnCell(int slot, int event, bool inCombat) {
	const int x = _targetX[slot], y = _targetY[slot];
	if (!_w.map->inBounds(x, y)) return;
	Member &m = _w.party(slot);
	ScriptCall c;
	c.cellScript = _w.map->cell(x, y).script;
	c.event = event;
	c.member = slot;
	c.arg8 = m.b[0x145];
	const ScriptResult r = _vm.run(c);
	if (!r.ran) {
		print("\rThat has no effect.\r");
		return;
	}
	handleScriptResult(r);
	if (!inCombat) endOfTurn();
}

void Game::useWeaponOnCell(int slot, bool inCombat) {
	Member &m = _w.party(slot);
	const int x = _targetX[slot], y = _targetY[slot];
	if (!_w.map->inBounds(x, y)) return;
	MapCell &cell = _w.map->cell(x, y);
	const int item = m.b[0x147];
	const WeaponDef &wd = _w.weaponOf(item);
	const FireMode &fm = wd.modes.at(std::min<size_t>(m.b[0x63 + 6 * m.b[0x145]], wd.modes.size() - 1));
	// KEH rollWeaponVsObject
	int result;
	if (cell.script == 0xFFFF || !_w.scripts || !_w.scripts->has(cell.script) || !(_w.scripts->block(cell.script).eventMask & 0x10)) result = 5;
	else if (cell.w4 == 0) result = 4;
	else {
		const int armor = cell.b3;
		if (armor * 1 >= fm.dmgMax) result = 4;
		else if (100 < _w.rng.range(1, 100)) result = 3;
		else {
			int absorb = _w.rng.range(armor * 1, armor * 2);
			int dmg = std::max(0, _w.rng.range(fm.dmgMin, fm.dmgMax) - absorb);
			cell.w4 = uint16_t(std::max(0, cell.w4 - dmg));
			result = dmg == 0 ? 0 : (cell.w4 == 0 ? 1 : 2);
		}
	}
	const std::string verb = wd.verb();
	auto usesLine = [&]() { print("\r" + m.name() + " " + verb + "..."); };
	switch (result) {
	case 0: case 3:
		usesLine();
		print("\rand misses!\r");
		useUpAmmoFor(slot, m.b[0x145]);
		endOfTurn();
		break;
	case 1: case 2: {
		usesLine();
		ScriptCall c;
		c.cellScript = cell.script;
		c.event = 4;
		c.member = slot;
		c.arg8 = m.b[0x145];
		c.argA = result == 1 ? 1 : 0;
		const ScriptResult r = _vm.run(c);
		if (r.ran) {
			handleScriptResult(r);
			useUpAmmoFor(slot, m.b[0x145]);
			if (!inCombat) endOfTurn();
		} else {
			print("\rYou decide that it wouldn't work.\r");
		}
		break;
	}
	default:
		print("\rYou decide that it wouldn't work.\r");
		break;
	}
}

void Game::useCommandOnCell(int slot, bool inCombat) {
	Member &m = _w.party(slot);
	switch (m.b[0x144]) {
	case 0x0A: useItemScriptOnCell(slot, 3, inCombat); break;
	case 0x0B:
		if (_w.item(m.b[0x147]).flags & 1) useWeaponOnCell(slot, inCombat);
		else useItemScriptOnCell(slot, 2, inCombat);
		break;
	case 0x0E: endOfTurn(); break;
	default: break;
	}
}

void Game::afterRosterScreen(int result) {
	_sheetActive = false;
	if (result < 0) { redraw(); return; }
	_w.lootFlag = false;
	useCommandOnCell(result, false);
	if (_w.lootFlag) lootCell(_w.disk.mapX, _w.disk.mapY);
	redraw();
}

int Game::characterInfo(int partyNum, bool inCombat) {
	const bool npc = inCombat || _npcMode;
	int member = -1;
	_sheetActive = true;
	_sheet = SheetState{};
	int result = -1;
	auto finish = [&](int r) { _sheetActive = false; result = r; };
	while (_sheetActive && !_quit) {
		if (member != partyNum) { member = partyNum; _sheet = SheetState{}; }
		_sheetMember = partyNum;
		Member &m = _w.party(partyNum);
		const int total = m.itemCount();
		const auto active = learnedSkills(_w, m, false), passive = learnedSkills(_w, m, true);
		const int key = readKey();
		auto pageKeys = [&](int k, int leftTo, int rightTo) {
			if (k == 0xFA) { _sheet.page = rightTo; return true; }
			if (k == 0xFB) { _sheet.page = leftTo; return true; }
			return false;
		};
		auto fkey = [&](int k) {
			const int s = k - 0x3B;
			if (s < 0 || s >= _w.partyCount()) return false;
			partyNum = s;
			return true;
		};
		if (key == 0xFF) { finish(-1); break; }
		if (_sheet.page == 0) {
			m.b[0x144] = 0x0A;
			static const char kLetters[] = "SIDWACL";
			const char *p = key < 128 && key > 0 ? std::strchr(kLetters, key) : nullptr;
			if (key == 'V') {
				if (m.b[0x58]) {
					_sheet.showConditions = true;
					readKey();
					_sheet.showConditions = false;
				}
			} else if (p && *p) {
				m.b[0x145] = uint8_t(p - kLetters);
				if (!m.hp() || m.hp() < 1) { message(m.name() + " is incapacitated."); continue; }
				finish(chooseCommandTarget(partyNum) ? partyNum : -1);
			} else if (key == 0xFC) {
				const int shown = std::min(8, total - _sheet.invOffset);
				if (total - _sheet.invOffset > shown) _sheet.invOffset += 8;
			} else if (key == 0xFD) {
				if (_sheet.invOffset >= 8) _sheet.invOffset -= 8;
			} else if (key >= 0x3B && key <= 0x3F) {
				fkey(key);
			} else if (pageKeys(key, 2, 1)) {
			} else if (key >= '1' && key <= '8') {
				const int shown = std::min(8, total - _sheet.invOffset);
				const int n = key - '0';
				if (n > shown) continue;
				const int slot = _sheet.invOffset + n - 1;
				m.b[0x145] = uint8_t(slot);
				if (_npcMode) { finish(partyNum); break; }
				if (!itemActionMenu(_w, *this, partyNum, slot, inCombat)) continue;
				if (inCombat && (m.b[0x144] == 0x0D || m.b[0x144] == 0x0C)) { finish(partyNum); break; }
				if (m.b[0x144] == 6) { finish(partyNum); break; }
				finish(chooseCommandTarget(partyNum) ? partyNum : -1);
			}
		} else if (_sheet.page == 1) {
			if (key >= 0x3B && key <= 0x3F) { fkey(key); continue; }
			if (pageKeys(key, 0, 2)) continue;
			if (key == 0xFC) { if (int(active.size()) - _sheet.activeOffset > 8) _sheet.activeOffset += 8; continue; }
			if (key == 0xFD) { if (_sheet.activeOffset >= 8) _sheet.activeOffset -= 8; continue; }
			if (key >= '1' && key <= '8') {
				const int n = key - '0';
				if (n > std::min(8, int(active.size()) - _sheet.activeOffset)) continue;
				const int skill = active[size_t(_sheet.activeOffset + n - 1)];
				if (m.hp() < 1) { message(m.name() + " is incapacitated."); continue; }
				m.b[0x145] = uint8_t(skill);
				if (skill == 0 || skill == 0x0B || skill == 4) {
					if (_npcMode) continue;
					if (doctorTreat(_w, *this, partyNum, true, !inCombat)) finish(partyNum);
					continue;
				}
				m.b[0x144] = 0x0A;
				m.b[0x145] = uint8_t(skill + 0x0C);
				if (!inCombat) { finish(chooseCommandTarget(partyNum) ? partyNum : -1); continue; }
				if (m.b[0x145] == 0x1A || m.b[0x145] == 0x19) { finish(partyNum); continue; }
				finish(chooseCommandTarget(partyNum) ? partyNum : -1);
			}
		} else {
			if (key >= 0x3B && key <= 0x3F) { fkey(key); continue; }
			pageKeys(key, 1, 0);
		}
	}
	_sheetActive = false;
	return result;
}

void Game::reorderRoster() {
	if (_w.partyCount() < 2) return;
	std::vector<int> remaining, chosen;
	for (int i = 0; i < _w.partyCount(); ++i) remaining.push_back(i);
	while (!remaining.empty()) {
		std::vector<std::string> names;
		for (int i : remaining) names.push_back(_w.party(i).name());
		const int pick = pickFromList("Pick a Player:", names);
		if (pick < 0) break;
		chosen.push_back(remaining[size_t(pick)]);
		remaining.erase(remaining.begin() + pick);
	}
	for (int i : remaining) chosen.push_back(i);
	uint8_t ids[5];
	for (size_t i = 0; i < chosen.size(); ++i) ids[i] = _w.disk.partyIds[chosen[i]];
	for (size_t i = 0; i < chosen.size(); ++i) _w.disk.partyIds[i] = ids[i];
	_dialogOpen = false;
}

void Game::banishMember() {
	if (_w.partyCount() == 1) { message("Hey! I'm all you've got!!"); return; }
	std::vector<std::string> names;
	for (int i = 0; i < _w.partyCount(); ++i) names.push_back(_w.party(i).name());
	const int pick = pickFromList("Banish whom:", names);
	if (pick < 0) return;
	if (yesNo("Really banish\r" + _w.party(pick).name() + "?")) _w.removeMemberFromParty(pick);
	_dialogOpen = false;
}

int Game::rosterScreen() {
	_rosterActive = true;
	int result = -1;
	while (!_quit) {
		const int k = readKey();
		if (k == 0xFF || k == 'L') break;
		if (k >= 0x3B && k <= 0x3F) {
			const int slot = k - 0x3B;
			if (slot >= _w.partyCount()) continue;
			_rosterActive = false;
			result = characterInfo(slot, false);
			break;
		}
		if (k == 'R') reorderRoster();
		else if (k == 'B') banishMember();
	}
	_rosterActive = false;
	return result;
}

void Game::quitMenu() {
	for (;;) {
		openDialog();
		_dialog = "Your choice?\r\r\r1. Quit (without saving)\r3. Quit (save game)\r5. Just save!\r\rN)evermind";
		redraw();
		int k = readKey();
		if (k >= 'a' && k <= 'z') k -= 32;
		if (k == 0xFF || k == 'N') break;
		if (k != '1' && k != '3' && k != '5') continue;
		if (k != '1') {
			if (_saveDir.empty() || !saveGame(_w, _saveDir, _saveSlot)) message("Unable to save the game.");
		}
		if (k != '5') _quit = true;
		break;
	}
	_dialogOpen = false;
	redraw();
}

void Game::lootCell(int x, int y) {
	if (!_w.map->inBounds(x, y) || !(_w.map->cell(x, y).flags & 0x80)) return;
	int member = 0;
	for (int i = 0; i < _w.partyCount(); ++i)
		if (World::conState(_w.party(i)) < 1) { member = i; break; }
	bool emptied = false;
	for (;;) {
		auto &items = _w.map->groundItems();
		std::vector<size_t> here;
		for (size_t i = 0; i < items.size(); ++i)
			if (items[i].x == x && items[i].y == y) here.push_back(i);
		if (here.empty()) { emptied = true; break; }
		std::vector<std::string> names;
		for (size_t i : here) {
			const GroundItem &g = items[i];
			names.push_back(std::to_string(g.qty) + (g.item == 1 ? " dollars" : " " + _w.item(g.item).name));
		}
		const int pick = pickFromList("You find these items:", names);
		if (pick < 0) break;
		GroundItem &g = items[here[size_t(pick)]];
		Member &m = _w.party(member);
		bool remove = false;
		if (g.item == 1) {
			_w.disk.cash += g.qty;
			remove = true;
		} else {
			applyItemEffect(_w, m, _w.item(g.item), false);
			if (_w.isUnique(g.item)) {
				_w.disk.uniqueItems |= uint8_t(1u << _w.item(g.item).uniqueBit);
				remove = true;
			} else if (m.itemCount() < 32) {
				_w.initItemSlot(m, m.itemCount(), g.item);
				++m.itemCount();
				if (--g.qty == 0) remove = true;
			} else {
				message(m.name() + "\rhas too much to carry\ralready.");
			}
		}
		if (remove) items.erase(items.begin() + long(here[size_t(pick)]));
	}
	bool any = false;
	for (const GroundItem &g : _w.map->groundItems()) if (g.x == x && g.y == y) any = true;
	if (!any) {
		_w.map->cell(x, y).flags &= 0x7F;
		if (emptied) runScriptChain(_w.map->cell(x, y).script, 5);
	}
}

bool Game::movePartyStep(int dir, bool forced) {
	World &w = _w;
	_dialogOpen = false;
	bool mutagenic = false;
	for (int i = 0; i < w.partyCount(); ++i)
		if (w.party(i).hp() > 0 && w.party(i).b[0x57] != 0) mutagenic = true;
	if (!forced && mutagenic && w.rng.range(1, 100) <= 10) dir = w.rng.range(1, 4);
	const int nx = w.disk.mapX + kDx[dir], ny = w.disk.mapY + kDy[dir];
	if (!w.map->inBounds(nx, ny)) {
		static const char *kOffMap[4] = {"\rYou see miles of impassable ocean and you decide to turn back.\r", "\rThere are roads around here somewhere!\r",
		                                 "\rTry using the road, friend.\r", "\r            hic!\r"};
		openDialog();
		_dialog += kOffMap[std::min<int>(w.map->offMapMsg(), 3)];
		redraw();
		if (!forced) endOfTurn();
		return !_quit;
	}
	const int px = w.disk.mapX, py = w.disk.mapY;
	const int curMap = w.mapNo;
	runScriptChain(w.map->cell(px, py).script, 1);        // event 1: leaving the cell
	if (_quit) return false;
	if (w.mapNo != curMap || w.disk.mapX != px || w.disk.mapY != py) {   // a script moved us
		if (!forced) endOfTurn();
		lootCell(w.disk.mapX, w.disk.mapY);
		redraw();
		return !_quit;
	}

	// monster on the target cell?
	for (size_t i = 0; i < w.map->monsters().size(); ++i) {
		Monster &m = w.map->monsters()[i];
		if (m.x() == nx && m.y() == ny) {
			m.raw[0x50] &= 0x7F;
			runScriptChain(m.script(), kEvMonster, int(i));
			if (!forced) endOfTurn();
			redraw();
			return !_quit;
		}
	}
	const bool passable = w.isCellPassable(nx, ny);
	if (passable) w.setPartyPosition(nx, ny);
	passTime();
	runScriptChain(w.map->cell(nx, ny).script, kEvEnter);      // event 0: stepping onto / bumping into the cell
	if (!_quit && !forced) endOfTurn();
	lootCell(w.disk.mapX, w.disk.mapY);
	redraw();
	return !_quit;
}

void Game::run() {
	redraw();
	while (!_quit) {
		const int k = _platform.waitKey();
		if (k < 0) break;
		_dialogOpen = false;
		switch (k) {
		case 0xFD: movePartyStep(1); break;
		case 0xFC: movePartyStep(2); break;
		case 0xFA: movePartyStep(3); break;
		case 0xFB: movePartyStep(4); break;
		case 0xFF: passTime(); endOfTurn(); redraw(); break;   // ESC / idle: let time pass
		case 'E': {   // engage nearby enemies
			Combat cb(_w, *this);
			if (cb.buildCombatants(true) > 0) {
				cb.commands = defaultCombatCommands();
				if (cb.run(true).wiped) { print("\rYour party has been defeated.\r"); _quit = true; }
			}
			redraw();
			break;
		}
		case 0x3B: case 0x3C: case 0x3D: case 0x3E: case 0x3F:
			if (k - 0x3B < _w.partyCount()) afterRosterScreen(characterInfo(k - 0x3B, false));
			break;
		case 'A': afterRosterScreen(rosterScreen()); break;
		case 'Q': quitMenu(); break;
		default: redraw(); break;
		}
	}
}

} // namespace fod

namespace fod {

void Game::useCommandInCombat(int slot) {
	_targetX[slot] = _w.disk.mapX; _targetY[slot] = _w.disk.mapY;
	_w.lootFlag = false;
	useCommandOnCell(slot, true);
}

} // namespace fod

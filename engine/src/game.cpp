#include "fod/game.h"
#include "fod/combat.h"
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
		m.u8(0x49) = uint8_t(i + 1);
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
		d.partyIds[i] = uint8_t(i);
	}
	d.partyCount = 4;
	return d;
}

Game::Game(const GameData &data, const Assets &assets, Platform &platform, Disk1 disk)
    : _data(data), _assets(assets), _platform(platform), _w(data, std::move(disk)), _vm(_w, *this) {
	_w.loadMap(_w.disk.maps[_w.disk.mapIndex]);
}

// ---------------------------------------------------------------- drawing
void Game::redraw() {
	renderMainScreen(_screen, _assets, _w, {});
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
	cb.commands = [this](Combat &cc, int slot, Combatant &rec) {
		print("\r" + _w.party(slot).name() + ", choose: A)ttack E)vade R)un\r");
		const int k = readKey();
		if (k == 'E') { rec.command = 1; rec.speed = uint8_t(rec.speed * 2); return Combat::GroupAction::None; }
		if (k == 'R') {
			print("G)et closer  F)lee\r");
			const int k2 = readKey();
			if (k2 == 'G') return Combat::GroupAction::GetCloser;
			if (k2 == 'F') return Combat::GroupAction::Flee;
		}
		cc.autoCommand(slot, rec);
		return Combat::GroupAction::None;
	};
	const Combat::Outcome o = cb.run(false);
	if (o.wiped) { print("\rYour party has been defeated.\r"); _quit = true; }
	redraw();
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
	cb.commands = [this](Combat &c, int slot, Combatant &rec) {
		print("\r" + _w.party(slot).name() + ", choose: A)ttack E)vade R)un\r");
		const int k = readKey();
		if (k == 'E') {
			rec.command = 1;
			rec.speed = uint8_t(rec.speed * 2);
			return Combat::GroupAction::None;
		}
		if (k == 'R') {
			print("G)et closer  F)lee\r");
			const int k2 = readKey();
			if (k2 == 'G') return Combat::GroupAction::GetCloser;
			if (k2 == 'F') return Combat::GroupAction::Flee;
		}
		c.autoCommand(slot, rec);
		return Combat::GroupAction::None;
	};
	const Combat::Outcome o = cb.run(false);
	if (o.wiped) {
		print("\rYour party has been defeated.\r");
		_quit = true;
	}
}

bool Game::movePartyStep(int dir) {
	World &w = _w;
	_dialogOpen = false;
	const int nx = w.disk.mapX + kDx[dir], ny = w.disk.mapY + kDy[dir];
	if (!w.map->inBounds(nx, ny)) {
		openDialog();
		_dialog += "You see miles of impassable ocean and you decide to turn back.";
		redraw();
		return !_quit;
	}
	const int px = w.disk.mapX, py = w.disk.mapY;
	const int curMap = w.mapNo;
	runScriptChain(w.map->cell(px, py).script, 1);        // event 1: leaving the cell
	if (_quit) return false;
	if (w.mapNo != curMap || w.disk.mapX != px || w.disk.mapY != py) { redraw(); return true; }  // script moved us

	// monster on the target cell?
	for (size_t i = 0; i < w.map->monsters().size(); ++i) {
		Monster &m = w.map->monsters()[i];
		if (m.x() == nx && m.y() == ny) {
			m.raw[0x50] &= 0x7F;
			runScriptChain(m.script(), kEvMonster, int(i));
			passTime();
			redraw();
			return !_quit;
		}
	}
	bool passable = w.map->cell(nx, ny).passable();
	for (const auto &g : w.map->groups())
		if ((g.raw[9] & 0xF) && g.x() == nx && g.y() == ny) passable = false;
	for (const auto &m : w.map->monsters())
		if (m.x() == nx && m.y() == ny) passable = false;
	if (passable) w.setPartyPosition(nx, ny);
	passTime();
	runScriptChain(w.map->cell(nx, ny).script, kEvEnter);      // event 0: stepping onto / bumping into the cell
	if (!_quit && w.mapNo == curMap) endOfTurn();
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
		case 'Q': _quit = true; break;
		default: redraw(); break;
		}
	}
}

} // namespace fod

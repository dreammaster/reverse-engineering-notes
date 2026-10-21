// Port of FOD.EXE: loadDisk1 (new roster), the title picture and the party creation menu.
#include "fod/creator.h"
#include "fod/lzss.h"
#include <algorithm>
#include <cstdarg>
#include <cstdio>
#include <ctime>

namespace fod {

namespace {

std::string fmt(const char *f, ...) __attribute__((format(printf, 1, 2)));
std::string fmt(const char *f, ...) {
	char buf[128];
	va_list ap;
	va_start(ap, f);
	std::vsnprintf(buf, sizeof buf, f, ap);
	va_end(ap);
	return buf;
}

const Win kMain{0, 14, 39, 19}, kSkills{14, 1, 38, 12}, kParty{1, 21, 38, 23}, kPortrait{1, 1, 12, 11};

void centered(Screen &s, const Assets &a, const Win &w, int y, const std::string &text) {
	winWrite(s, a, w, std::max(0, (w.width() - int(text.size())) / 2), y, text);
}

class Creator {
public:
	Creator(const GameData &d, const Assets &a, Platform &p, Disk1 &disk) : _d(d), _a(a), _p(p), _disk(disk) {}

	bool run() {
		// title picture, any key
		const Bytes pic = lzssDecode(readFile(_d.dir + "/TPICT"));
		for (int y = 0; y < 200; ++y)
			for (int x = 0; x < 160 && size_t(y) * 160 + size_t(x) < pic.size(); ++x) {
				_s.put(x * 2, y, pic[size_t(y) * 160 + size_t(x)] >> 4);
				_s.put(x * 2 + 1, y, pic[size_t(y) * 160 + size_t(x)] & 15);
			}
		_p.present(_s);
		if (_p.waitKey() < 0) return false;

		int current = 0;
		for (;;) {
			_mode = Mode::Menu;
			draw(current);
			int k = _p.waitKey();
			if (k < 0) return false;
			if (k >= 'a' && k <= 'z') k -= 32;
			const int count = _disk.partyCount;
			if (k == 'A') { if (count < 3) { const int n = addMember(); if (n < 0) return false; if (n >= 0x100) continue; current = n; } }
			else if (k == 'R') {
				if (count == 0) { notice("There's no one to remove!"); continue; }
				current = removeMember(current);
			} else if (k == 'E' || (k >= 0x3B && k <= 0x3D)) {
				if (k >= 0x3B) { if (k - 0x3B >= count) continue; current = k - 0x3B; }
				if (count == 0) { notice("There's no one to edit!"); continue; }
				if (!editMember(current)) return false;
			} else if (k == 'P') {
				if (count == 0) {
					_mode = Mode::Notice;
					_notice1 = "It's tough out there!";
					_notice2 = "You should take somebody with you.";
					draw(current);
					if (_p.waitKey() < 0) return false;
					continue;
				}
				return true;
			}
		}
	}

private:
	enum class Mode { Menu, Choose, Edit, Remove, Notice };

	const Profession &prof() const { return _d.archetypes.professions().at(size_t(_profession)); }

	void notice(const std::string &text) {
		_mode = Mode::Notice;
		_notice1.clear();
		_notice2 = text;
		draw(0);
		_p.waitKey();
	}

	// ---------------------------------------------------------------- drawing
	void drawSkills() {
		const Win &w = kSkills;
		const Profession &pr = prof();
		centered(_s, _a, w, 0, "Skills");
		centered(_s, _a, w, 1, "Active      Passive");
		int ra = 0, rp = 0;
		for (int i = 0; i < 16; ++i) {
			if (pr.raw[0x38 + size_t(i)]) winWrite(_s, _a, w, 0, 3 + ra++, fmt("%1d %-10.10s", pr.raw[0x38 + size_t(i)], skillName(i).c_str()));
			if (pr.raw[0x48 + size_t(i)]) winWrite(_s, _a, w, 13, 3 + rp++, fmt("%1d %-10.10s", pr.raw[0x48 + size_t(i)], skillName(16 + i).c_str()));
		}
	}

	std::string skillName(int skill) const {
		const size_t o = skill < 16 ? 0x126 + 0x16 * size_t(skill) : 0x286 + 0x16 * size_t(skill - 16);
		const char *p = reinterpret_cast<const char *>(_d.hdspct.data() + o);
		return std::string(p, strnlen(p, 20));
	}

	void drawParty(int highlight, bool showHighlight) {
		for (int i = 0; i < _disk.partyCount; ++i) {
			const Member &m = _disk.roster[size_t(i)];
			winWrite(_s, _a, kParty, 0, i, fmt("F%1d>", i + 1), showHighlight && highlight == i);
			const Profession &pr = _d.archetypes.professions().at(m.profession());
			winWrite(_s, _a, kParty, 3, i, fmt("%-16.16s - %-16.16s", m.name().c_str(), pr.name.c_str()));
		}
	}

	void drawEdit(int member, int hilite) {
		const Member &m = _disk.roster[size_t(member)];
		const auto &profs = _d.archetypes.professions();
		for (int i = 0; i < 5 && i < int(profs.size()); ++i) winWrite(_s, _a, kMain, 0, 1 + i, fmt("%1d)%-13.13s", i + 1, profs[size_t(i)].name.c_str()));
		winWrite(_s, _a, kMain, 19, 0, "Attribute Pts: <-");
		static const char *kLabel[7] = {"ST:", "IQ:", "DX:", "WP:", "AP:", "CH:", "LK:"};
		static const int kX[7] = {21, 21, 21, 21, 30, 30, 30}, kY[7] = {2, 3, 4, 5, 2, 3, 4};
		for (int i = 0; i < 7; ++i) {
			winWrite(_s, _a, kMain, kX[i], kY[i], fmt("%3s", kLabel[i]));
			winWrite(_s, _a, kMain, kX[i] + 3, kY[i], fmt("%2u", m.attr(i)), hilite == i);
		}
		winWrite(_s, _a, kMain, 29, 5, "Sex:");
		static const char *kSex[4] = {"Other", "Male", "Female", "Other"};
		winWrite(_s, _a, kMain, 34, 5, kSex[m.b[0x50] & 3], hilite == 7);
		winWrite(_s, _a, kSkills, 6, 11, fmt("Con:%d", m.hp()));
		winWrite(_s, _a, kMain, 36, 0, fmt("%2.2d->", m.b[0x23]));
	}

	void draw(int current, int hilite = 0, bool editing = false) {
		_s.clear();
		drawBorders(_s, _a, 5);
		(void)editing;
		switch (_mode) {
		case Mode::Menu:
			centered(_s, _a, kPortrait, 10, "Welcome");
			centered(_s, _a, kMain, 1, "Choose a function:");
			winWrite(_s, _a, kMain, 5, 3, "A)dd member      R)emove member");
			winWrite(_s, _a, kMain, 5, 4, "E)dit member     P)lay the game");
			drawParty(current, false);
			break;
		case Mode::Choose: {
			centered(_s, _a, kMain, 1, "Choose a Profession:");
			const auto &profs = _d.archetypes.professions();
			for (int i = 0; i < 3 && i < int(profs.size()); ++i) winWrite(_s, _a, kMain, 4, 3 + i, fmt("%1d)%-13.13s", i + 1, profs[size_t(i)].name.c_str()));
			for (int i = 0; i < 2 && i + 3 < int(profs.size()); ++i) winWrite(_s, _a, kMain, 23, 3 + i, fmt("%1d)%-13.13s", i + 4, profs[size_t(i + 3)].name.c_str()));
			drawParty(current, false);
			break;
		}
		case Mode::Edit:
			centered(_s, _a, kPortrait, 10, prof().name);
			drawSkills();
			drawEdit(current, hilite);
			drawParty(current, true);
			// the name being typed is shown in the party window by the caller
			break;
		case Mode::Remove:
			centered(_s, _a, kMain, 1, "Really remove this member?");
			centered(_s, _a, kMain, 3, "Y)es  N)o");
			drawParty(current, false);
			break;
		case Mode::Notice:
			if (!_notice1.empty()) centered(_s, _a, kMain, 1, _notice1);
			centered(_s, _a, kMain, _notice1.empty() ? 2 : 3, _notice2);
			drawParty(current, false);
			break;
		}
		_p.present(_s);
	}

	// ---------------------------------------------------------------- member operations
	void createMember(int slot) {
		const Profession &pr = prof();
		Member &m = _disk.roster[size_t(slot)];
		for (int i = 0; i < 12; ++i) m.b[0x18 + size_t(i)] = 0;
		for (int i = 0; i < 7; ++i) m.b[0x18 + size_t(i)] = pr.raw[0x14 + size_t(i)];
		m.b[0x23] = pr.raw[0x33];
		for (int i = 0; i < 16; ++i) {
			m.b[0x24 + size_t(i)] = pr.raw[0x38 + size_t(i)];
			m.b[0x124 + size_t(i)] = 0;
			m.b[0x34 + size_t(i)] = pr.raw[0x48 + size_t(i)];
			m.b[0x134 + size_t(i)] = 0;
		}
		m.b[0x58] = 0;
		m.b[0x48] = uint8_t(_profession);
		m.b[0x51] = pr.raw[0x7B];
		m.b[0x50] = (m.b[0x50] & 0xFC) | 1;
	}

	void rollHitPoints(int slot) {
		const Profession &pr = prof();
		Member &m = _disk.roster[size_t(slot)];
		const int lo = pr.raw[0x70] | (pr.raw[0x71] << 8), hi = pr.raw[0x72] | (pr.raw[0x73] << 8);
		const int hp = _rng.range(lo, hi);
		m.setHpMax(hp); m.setHp(hp); m.setU16(0x122, uint16_t(hp));
	}

	// returns the new slot, -1 on window close, 0x100 if cancelled
	int addMember() {
		_mode = Mode::Choose;
		draw(0);
		int choice = -1;
		for (;;) {
			const int k = _p.waitKey();
			if (k < 0) return -1;
			if (k == 0xFF) return 0x100;
			if (k >= '1' && k <= '5' && k - '1' < int(_d.archetypes.professions().size())) { choice = k - '1'; break; }
		}
		_profession = choice;
		const int slot = _disk.partyCount++;
		createMember(slot);
		rollHitPoints(slot);
		std::memset(_disk.roster[size_t(slot)].b.data(), 0, 24);
		std::strcpy(reinterpret_cast<char *>(_disk.roster[size_t(slot)].b.data()), "Ojnab Bob");
		return editMember(slot) ? slot : -1;
	}

	int removeMember(int current) {
		_mode = Mode::Remove;
		draw(current);
		int k = _p.waitKey();
		if (k >= 'a' && k <= 'z') k -= 32;
		if (k == 'Y') {
			for (int i = current; i < _disk.partyCount - 1; ++i) _disk.roster[size_t(i)] = _disk.roster[size_t(i) + 1];
			--_disk.partyCount;
			_disk.roster[size_t(_disk.partyCount)] = Disk1::newGame().roster[0];
			if (current == _disk.partyCount && current > 0) --current;
		}
		return current;
	}

	bool editMember(int member) {
		int hilite = 0;
		_profession = _disk.roster[size_t(member)].profession();
		std::string name = _disk.roster[size_t(member)].name();
		bool editing = true;
		_mode = Mode::Edit;
		while (editing) {
			_profession = _disk.roster[size_t(member)].profession();
			draw(member, hilite, true);
			// the name line (with a cursor) replaces the party row for the member being edited
			{
				const Win row = kParty;
				winClear(_s, Win{row.x0 + 3, row.y0 + member, row.x1, row.y0 + member});
				winWrite(_s, _a, row, 3, member, name + ((_blink++ & 4) ? " " : "\x1b"));
				_p.present(_s);
			}
			const int k = _p.waitKey();
			if (k < 0) return false;
			Member &m = _disk.roster[size_t(member)];
			const Profession &pr = prof();
			if (k == 0xFD) hilite = (hilite + 7) % 8;
			else if (k == 0xFC) hilite = (hilite + 1) % 8;
			else if (k == 0xFA) {   // increase
				if (hilite < 7) {
					if (m.b[0x23] != 0 && pr.raw[0x20 + size_t(hilite)] > m.attr(hilite)) { ++m.attr(hilite); --m.b[0x23]; }
				} else {
					m.b[0x50] = uint8_t(m.b[0x50] + 1);
					if (m.b[0x50] > 2) m.b[0x50] = 1;
				}
			} else if (k == 0xFB) {   // decrease
				if (hilite < 7) {
					if (m.b[0x23] < pr.raw[0x33] && pr.raw[0x14 + size_t(hilite)] < m.attr(hilite)) { --m.attr(hilite); ++m.b[0x23]; }
				} else {
					m.b[0x50] = uint8_t(((m.b[0x50] + 2) & 1) + 1);
				}
			} else if (k == 0xFF) {
				if (!name.empty()) { std::memset(m.b.data(), 0, 24); std::memcpy(m.b.data(), name.data(), std::min<size_t>(name.size(), 12)); editing = false; }
			} else if (k == 8) {
				if (!name.empty()) name.pop_back();
			} else if (k >= '1' && k <= '5') {
				const int np = k - '1';
				if (np < int(_d.archetypes.professions().size()) && np != _profession) {
					_profession = np;
					const std::string keep = name;
					createMember(member);
					(void)keep;
				}
				rollHitPoints(member);
			} else if (k >= 0x3B && k <= 0x3D) {
				const int to = k - 0x3B;
				if (to < _disk.partyCount) {
					std::memset(m.b.data(), 0, 24);
					std::memcpy(m.b.data(), name.data(), std::min<size_t>(name.size(), 12));
					return editMember(to);
				}
			} else if ((k >= 'A' && k <= 'Z') || (k >= 'a' && k <= 'z') || k == ' ' || (k > '5' && k < 127)) {
				if (name.size() < 12) name += char(k);
			}
		}
		return true;
	}

	const GameData &_d;
	const Assets &_a;
	Platform &_p;
	Disk1 &_disk;
	Screen _s;
	Mode _mode = Mode::Menu;
	int _profession = 0;
	std::string _notice1, _notice2;
	int _blink = 0;
	Rng _rng{uint32_t(std::time(nullptr))};
};

} // namespace

Disk1 emptyRoster() {
	Disk1 d = Disk1::newGame();
	for (auto &m : d.roster) std::memset(m.b.data(), 0, 24);
	d.partyCount = 0;
	return d;
}

void finishNewParty(Disk1 &disk) {
	disk.saveCounter = 1;
	for (int i = 0; i < disk.partyCount; ++i) {
		disk.roster[size_t(i)].b[0x49] = uint8_t(i - 6);
		disk.partyIds[i] = uint8_t(i);
		disk.tail[size_t(uint8_t(i - 6))] = 1;
	}
}

bool runPartyCreator(const GameData &data, const Assets &assets, Platform &platform, Disk1 &disk) {
	Creator c(data, assets, platform, disk);
	if (!c.run()) return false;
	finishNewParty(disk);
	return true;
}

} // namespace fod

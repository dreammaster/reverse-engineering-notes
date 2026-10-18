// play_tool EXE GAMEDIR [script]: a text front end for the transpiled game logic - type commands, read the original's responses.
//   look | l | look at fishing pole | take stick | put stick on pole | talk to shella | give X to Y ...   (verb names from the game's own vocabulary)
//   inv | here | room | rooms | goto ROOM | shot FILE.png | reset | state | quit
// With a script file the lines are executed instead of reading the keyboard (and echoed).
#include "engine.h"
#include "render.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace shannara;

struct Verb {
	std::string name, prep;
	int flags, id;
};

static std::string lower(std::string s) {
	for (char &c : s)
		c = (char)tolower((unsigned char)c);
	return s;
}

class Player {
public:
	Player(const std::string &exe, const std::string &dir) : _e(exe, dir), _picDir(getenv("SHAN_PICS") ? getenv("SHAN_PICS") : dir + "/../game_out/pics") {
		Machine &m = _e.machine();
		for (uint32_t a = 0x1DAA0; a + 13 <= 0x1DE98; a += 13) {
			uint32_t np = m.r32(a), pp = m.r32(a + 4);
			if (!np || np >= 0x100000)
				break;
			_verbs.push_back({m.cstr(np), pp ? m.cstr(pp) : "", m.r8(a + 8), (int)m.r32(a + 9)});
		}
		int n = m.r16(0x217A0);
		for (int i = 1; i <= n; i++)
			_names.push_back(objectName(i));
		_e.snapshot(); // the start state, for "reset"
		// longest verb names first so "look at" wins over "look"
		_order.resize(_verbs.size());
		for (size_t i = 0; i < _order.size(); i++)
			_order[i] = i;
		std::sort(_order.begin(), _order.end(), [&](size_t a, size_t b) { return _verbs[a].name.size() > _verbs[b].name.size(); });
	}

	std::string objectName(int obj) {
		uint32_t p = _e.callFunction(0x19FB4, (uint32_t)obj); // object_get_name?
		return p ? _e.machine().cstr(p) : std::string();
	}

	int room() { return _e.machine().r16(0xADE96); }

	std::vector<int> children(int parent) {
		std::vector<int> out;
		for (int slot = 0; slot < 3; slot++) {
			int c = (int)(_e.callFunction(0x1A348, (uint32_t)parent, (uint32_t)slot) & 0xFFFF); // object_get_list_head?
			for (int guard = 0; c && guard < 500; guard++) {
				out.push_back(c);
				c = (int)(_e.callFunction(0x1A49C, (uint32_t)c) & 0xFFFF); // object_get_next_sibling?
			}
		}
		return out;
	}

	void list(const char *title, int parent) {
		printf("%s:", title);
		for (int o : children(parent))
			printf(" %s(%d)", _names[o - 1].c_str(), o);
		printf("\n");
	}

	// find an object by name or number: prefer objects here / carried
	int findObject(const std::string &what) {
		if (what.empty())
			return 0;
		if (isdigit((unsigned char)what[0]))
			return atoi(what.c_str());
		std::string w = lower(what);
		std::vector<int> scope = children(room());
		for (int o : children(3))
			scope.push_back(o);
		scope.push_back(3);
		for (int o : scope)
			if (lower(_names[o - 1]) == w)
				return o;
		for (size_t i = 0; i < _names.size(); i++)
			if (lower(_names[i]) == w)
				return (int)i + 1;
		for (int o : scope)
			if (lower(_names[o - 1]).find(w) != std::string::npos)
				return o;
		return 0;
	}

	void run(const std::string &lineIn) {
		std::string line = lower(lineIn);
		while (!line.empty() && isspace((unsigned char)line.back()))
			line.pop_back();
		if (line.empty())
			return;
		if (line == "inv" || line == "i") { list("carrying", 3); return; }
		if (line == "here") { list(("in " + _names[room() - 1]).c_str(), room()); return; }
		if (line == "room") { printf("room %d: %s\n", room(), _names[room() - 1].c_str()); return; }
		if (line == "state") { printf("state hash %016llx, score %u\n", (unsigned long long)_e.stateHash(), _e.machine().r32(0xADEAC)); return; }
		if (line == "l" || line == "look around") line = "look";
		if (line.rfind("goto ", 0) == 0) {
			int r = atoi(line.c_str() + 5);
			try { _e.callFunction(0x1EE10, (uint32_t)r); } catch (const std::exception &x) { printf("[%s]\n", x.what()); }
			printf("%s\n", _e.takeText().c_str());
			return;
		}
		if (line.rfind("shot ", 0) == 0) { // shot FILE.png: draw the current room (background + the overlays the game says are visible)
			try {
				uint64_t h0 = _e.regionHash();
				RenderInfo ri;
				Image img = renderRoom(_e, _picDir, &ri);
				writePng(lineIn.substr(5), img);
				printf("room %d, picture 0x%04X, %dx%d, %d of %d overlay entries drawn%s\n", ri.room, ri.picId, img.w, img.h, ri.drawn, ri.entries, h0 == _e.regionHash() ? "" : " (WARNING: the draw phase changed the saved state)");
			} catch (const std::exception &x) { printf("[%s]\n", x.what()); }
			_e.takeText();
			return;
		}
		if (line == "reset") { _e.restore(); printf("(back at the start state)\n"); return; }
		if (line == "verbs") {
			for (auto &v : _verbs)
				printf("%s%s%s | ", v.name.c_str(), v.prep.empty() ? "" : " ", v.prep.c_str());
			printf("\n");
			return;
		}
		for (size_t k : _order) {
			const Verb &v = _verbs[k];
			std::string vn = lower(v.name);
			if (line.compare(0, vn.size(), vn) != 0 || (line.size() > vn.size() && line[vn.size()] != ' '))
				continue;
			std::string rest = line.size() > vn.size() ? line.substr(vn.size() + 1) : "";
			std::string dobj = rest, iobj;
			if (v.flags >= 2 && !v.prep.empty()) {
				size_t p = rest.find(" " + lower(v.prep) + " ");
				if (p != std::string::npos) {
					dobj = rest.substr(0, p);
					iobj = rest.substr(p + v.prep.size() + 2);
				}
			}
			int d = v.flags >= 1 ? findObject(dobj) : 0, i = v.flags >= 2 ? findObject(iobj) : 0;
			if (v.flags >= 1 && !d) { printf("(no such object: '%s')\n", dobj.c_str()); return; }
			try {
				_e.command(v.id, d, i);
			} catch (const std::exception &x) {
				printf("[engine exception: %s]\n", x.what());
			}
			printf("%s\n", _e.takeText().c_str());
			return;
		}
		printf("(unknown command; 'verbs' lists the vocabulary)\n");
	}

private:
	Engine _e;
	std::string _picDir;
	std::vector<Verb> _verbs;
	std::vector<std::string> _names;
	std::vector<size_t> _order;
};

int main(int argc, char **argv) {
	if (argc < 3) {
		fprintf(stderr, "usage: play_tool EXE GAMEDIR [script]\n");
		return 2;
	}
	try {
		Player p(argv[1], argv[2]);
		p.run("room");
		p.run("look");
		std::istream *in = &std::cin;
		std::ifstream script;
		bool echo = false;
		if (argc >= 4) {
			script.open(argv[3]);
			in = &script;
			echo = true;
		}
		std::string line;
		while ((echo ? true : (printf("> "), fflush(stdout), true)) && std::getline(*in, line)) {
			if (line == "quit" || line == "q")
				break;
			if (echo)
				printf("> %s\n", line.c_str());
			p.run(line);
		}
	} catch (const std::exception &x) {
		fprintf(stderr, "error: %s\n", x.what());
		return 1;
	}
	return 0;
}

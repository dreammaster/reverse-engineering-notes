#include "xanth/interface.h"

#include <cstdio>
#include <cstring>

#include "xanth/verbs.h"

namespace Xanth {

Interface::Interface(Game &game, Display &display) : _game(game), _display(display), _files(NULL), _quit(false), _selected(-1), _verb(0), _dobj(0) {
	_verbIds = initialVerbs();
}

void Interface::start() {
	std::vector<std::string> names;
	for (size_t i = 0; i < _verbIds.size(); i++) {
		const VerbInfo *v = findVerb(_verbIds[i]);
		names.push_back(v ? v->name : "?");
	}
	_display.setVerbs(names);
	_game.beginGame();
}

/** The short form of an object in the sentence line: its alternative name with an article ("the computer"), a proper name as it is. */
std::string Interface::shortName(int obj) {
	std::string alt = _game.objectNames.name(_game.state.objects.altNameOffset(obj));
	if (alt.empty() || _game.state.objects.hasAttr(obj, 2))
		return _game.print_obj(2, obj);
	return "the " + alt;
}

void Interface::move(int x, int y) {
	_display.setMouse(x, y);
	int obj = _display.inventoryObjectAt(x, y);
	if (!obj)
		obj = _display.objectAt(x, y);
	// the line at the bottom of the picture: the sentence so far ("Look at the computer"), or just the name of the thing under the pointer
	std::string name;
	if (obj) {
		name = _game.objectNames.name(_game.state.objects.altNameOffset(obj));
		if (name.empty())
			name = _game.objectName(obj);
	}
	if (_verb) {
		const VerbInfo *info = findVerb(_verb);
		std::string line = info ? info->name : "";
		if (_dobj) {
			line += " " + shortName(_dobj);
			if (info && info->preposition)
				line += std::string(" ") + info->preposition;
		}
		if (obj && info && info->objects > 0)
			line += " " + shortName(obj);
		name = line;
	}
	_display.setHoverName(name);
}

void Interface::click(int x, int y) {
	move(x, y);
	int v = _display.verbAt(x, y);
	if (v >= 0) {
		_selected = v;
		_verb = _verbIds[v];
		_dobj = 0;
		_display.selectVerb(v);
		const VerbInfo *info = findVerb(_verb);
		if (info && info->objects == 0)       // a verb without objects ("Look") is a complete sentence
			execute(0, 0);
		move(x, y);                           // the sentence line shows the verb
		return;
	}
	if (x >= 4 && x <= 24 && y >= 168 && y <= 181) {      // the WAIT button
		_verb = 80;
		_display.pressButton(0);
		execute(0, 0);
		return;
	}
	if (x >= 25 && x <= 45 && y >= 168 && y <= 181) {      // the disk button: the game menu
		_display.pressButton(1);
		diskMenu();
		return;
	}
	int arrow = _display.compassAt(x, y);
	if (arrow >= 0) {
		_verb = 0x51;                               // walk: the direction code is the arrow number + 1
		execute(arrow + 1, 0);
		return;
	}
	if (_display.scrollInventory(x, y))
		return;
	int obj = _display.inventoryObjectAt(x, y);
	if (!obj)
		obj = _display.objectAt(x, y, true);
	if (!obj || !_verb)
		return;
	const VerbInfo *info = findVerb(_verb);
	bool two = info && info->objects == 2;
	if (two && !_dobj) {
		_dobj = obj;
		return;
	}
	execute(two ? _dobj : obj, two ? obj : 0);
}

std::string Interface::slotName(int slot) const {
	char b[24];
	snprintf(b, sizeof(b), "XANTH_%02d.SAV", slot);
	return b;
}

void Interface::diskMenu() {
	static const char *kItems[] = {"Help", "Save", "Restore", "Restart", "Music/Voice", "Status/Score", "Quit", "Cancel"};
	int choice = _display.popupMenu(std::vector<std::string>(kItems, kItems + 8));
	if (getenv("XANTH_TRACE"))
		fprintf(stderr, "disk menu: %d\n", choice);
	switch (choice) {
	case 1:
		saveGame();
		break;
	case 2:
		restoreGame();
		break;
	case 3:
		_game.newGame();
		start();
		break;
	case 5:
		_display.beginCommand();
		_game.Font_WriteMessage(" You have amassed %d of 1000 points,\n giving you the rank of .",(int)_game.state.v.score);
		break;
	case 6:
		_quit = true;
		break;
	default:
		break;
	}
}

// Saved games: a slot is a file with a 32-byte description followed by the game state in the original's save format.
void Interface::saveGame() {
	if (!_files)
		return;
	std::vector<std::string> slots;
	for (int i = 1; i <= 8; i++) {
		Common::Bytes d;
		std::string label = std::to_string(i) + ": ";
		if (_files->loadSaved(slotName(i), d) && d.size() >= 32)
			label += std::string((const char *)&d[0], strnlen((const char *)&d[0], 32));
		else
			label += "(empty)";
		slots.push_back(label);
	}
	int slot = _display.popupMenu(slots) + 1;
	if (slot <= 0)
		return;
	Common::Bytes data(32, 0), state;
	char title[32];
	snprintf(title, sizeof(title), "Turn %d, room %d", (int)_game.state.v.turn_counter, (int)_game.state.v.current_room);
	memcpy(&data[0], title, strlen(title));
	_game.state.save(state);
	data.insert(data.end(), state.begin(), state.end());
	_files->save(slotName(slot), data);
}

void Interface::restoreGame() {
	if (!_files)
		return;
	std::vector<std::string> slots;
	std::vector<int> numbers;
	for (int i = 1; i <= 8; i++) {
		Common::Bytes d;
		if (_files->loadSaved(slotName(i), d) && d.size() >= 32) {
			slots.push_back(std::to_string(i) + ": " + std::string((const char *)&d[0], strnlen((const char *)&d[0], 32)));
			numbers.push_back(i);
		}
	}
	if (slots.empty())
		return;
	int pick = _display.popupMenu(slots);
	if (getenv("XANTH_TRACE"))
		fprintf(stderr, "restore: %d slots, pick %d\n", (int)slots.size(), pick);
	if (pick < 0)
		return;
	Common::Bytes d;
	if (!_files->loadSaved(slotName(numbers[pick]), d))
		return;
	Common::Bytes state(d.begin() + 32, d.end());
	if (_game.state.load(state)) {              // as after a restore in the original: the screen is drawn again and the room told (phase 0xA)
		_game.RoomItem_Draw(0x10);
		_game.call_object_handler(_game.state.v.current_room, 0xA);
	}
}

void Interface::execute(int dobj, int iobj) {
	_display.beginCommand();
	_display.selectVerb(-1);
	_game.executeCommand(_verb, dobj, iobj);
	_verb = 0;
	_dobj = 0;
	_selected = -1;
	move(_display.mouseX(), _display.mouseY());
}

} // namespace Xanth

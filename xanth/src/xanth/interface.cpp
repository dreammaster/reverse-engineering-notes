#include "xanth/interface.h"

#include <cstdio>
#include <cstring>

#include "xanth/verbs.h"

namespace Xanth {

Interface::Interface(Game &game, Display &display) : _game(game), _display(display), _files(NULL), _quit(false), _canUndo(false), _fromDefault(false), _pendingObj(0), _selected(-1), _verb(0), _dobj(0) {
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
		if (!obj && _pendingObj && !_dobj)
			obj = _pendingObj;
		if (obj && info && info->objects > 0)
			line += " " + shortName(obj);
		name = line;
	}
	_display.setHoverName(name);
}

/** The verbs listed for a thing (the list its record points to in the original's table): its own, usually the special ones like "Push". */
std::vector<int> Interface::verbsOf(int obj) {
	std::vector<int> out;
	if (!_game.state.objects.valid(obj))
		return out;
	const byte *r = _game.state.objects.record(obj);
	int type = _game.state.objects.type(obj), at = type == 2 ? 0x18 : type == 4 ? 0x0A : (type == 3 || type == 5) ? 0x1C : -1;
	if (at < 0)
		return out;
	unsigned idx = r[at] | (r[at + 1] << 8);
	if (!idx)
		return out;
	for (int i = 0; i < 16; i++) {
		int w = _game.exe16(0x3000, (int)(0x4624 + idx * 2 + 2 + i * 2));
		if (!w)
			break;
		out.push_back(w);
	}
	return out;
}

/** The verb a thing gets when no verb is chosen (build_command_line?): TAKE for something portable that is not carried, else its own default verb,
 *  else LOOK_AT (0x45 for what is alive or one of the two special things). */
int Interface::defaultVerb(int obj) {
	if (_game.object_has_attr(obj, 0xC) && !_game.object_parent_is(obj, 1))
		return 0x44;
	int v = _game.object_get_default_verb(obj);
	if (v)
		return v;
	return (!_game.object_has_attr(obj, 0x17) && obj != 0x182 && obj != 0x183) ? 0x13 : 0x45;
}

void Interface::showObjectVerbs(int obj) {
	_pendingObj = obj;
	_fromDefault = true;
	_extraIds = verbsOf(obj);
	std::vector<std::string> names;
	for (size_t i = 0; i < _extraIds.size(); i++) {
		const VerbInfo *v = findVerb(_extraIds[i]);
		names.push_back(v ? v->name : "?");
	}
	_display.setExtraVerbs(names);
	_verb = 0;
	int def = defaultVerb(obj);                          // the thing's usual verb is chosen already
	for (size_t i = 0; i < _extraIds.size(); i++)
		if (_extraIds[i] == def) {
			_verb = def;
			_display.selectVerb(100 + (int)i);
		}
	for (size_t i = 0; i < _verbIds.size() && !_verb; i++)
		if (_verbIds[i] == def) {
			_verb = def;
			_display.selectVerb((int)i);
		}
}

void Interface::click(int x, int y) {
	move(x, y);
	int v = _display.verbAt(x, y);
	if (v >= 0) {
		int id = v < 100 ? _verbIds[v] : _extraIds[v - 100];
		if (id == _verb && !_fromDefault) {       // the verb that is chosen is clicked again: not chosen any more
			_verb = 0;
			_dobj = 0;
			_selected = -1;
			_display.selectVerb(-1);
			move(x, y);
			return;
		}
		_fromDefault = false;
		_selected = v;
		_verb = id;
		int first = _dobj ? _dobj : _pendingObj;       // the thing clicked last, before this verb was chosen
		_dobj = 0;
		_display.selectVerb(v);
		const VerbInfo *info = findVerb(_verb);
		if (info && info->objects == 0 && _verb == 38 && first) {      // "Look" with a thing already clicked is "Look at" it
			_verb = 19;
			execute(first, 0);
		} else if (info && info->objects == 0) {       // a verb without objects ("Look") is a complete sentence
			execute(0, 0);
		} else if (first) {                      // a thing was clicked first: it is the (first) object
			if (info && info->objects == 2)
				_dobj = first;
			else
				execute(first, 0);
		}
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
	if (x >= 25 && x <= 45 && y >= 182 && y <= 195) {      // UNDO
		undo();
		_display.pressButton(3);
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
	if (!obj) {                                  // a click on nothing: what was chosen so far is dropped (sub_1923E)
		if (_verb || _dobj || _pendingObj) {
			_verb = 0;
			_dobj = 0;
			_pendingObj = 0;
			_fromDefault = false;
			_selected = -1;
			_extraIds.clear();
			_display.setExtraVerbs(std::vector<std::string>());
			_display.selectVerb(-1);
			move(x, y);
		}
		return;
	}
	if (!_verb) {                                // no verb yet: the thing's own verbs are listed
		showObjectVerbs(obj);
		move(x, y);
		return;
	}
	const VerbInfo *info = findVerb(_verb);
	bool two = info && info->objects == 2;
	if (two && !_dobj) {
		_dobj = obj;
		_pendingObj = obj;
		move(x, y);
		return;
	}
	execute(two ? _dobj : obj, two ? obj : 0);
}

std::string Interface::slotName(int slot) const {
	char b[24];
	snprintf(b, sizeof(b), "XANTH_%02d.SAV", slot);
	return b;
}

void Interface::undo() {
	_display.beginCommand();
	if (!_canUndo) {
		_game.Font_WriteMessage("[You can't use \"undo\" after that.]");
		return;
	}
	if (_game.state.load(_undoState)) {
		_canUndo = false;
		_game.RoomItem_Draw(0x10);
		_game.Font_WriteMessage(" [Undone.]");
	}
}

void Interface::doubleClick(int x, int y) {
	int obj = _display.inventoryObjectAt(x, y);
	if (!obj)
		obj = _display.objectAt(x, y);
	if (!obj)
		return;
	_verb = defaultVerb(obj);
	execute(obj, 0);
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
	_game.state.save(_undoState);               // every command can be undone: the state before it is kept
	_canUndo = true;
	_display.beginCommand();
	_display.selectVerb(-1);
	_game.executeCommand(_verb, dobj, iobj);
	_verb = 0;
	_dobj = 0;
	_selected = -1;
	_pendingObj = 0;
	_fromDefault = false;
	_extraIds.clear();
	_display.setExtraVerbs(std::vector<std::string>());
	move(_display.mouseX(), _display.mouseY());
}

} // namespace Xanth

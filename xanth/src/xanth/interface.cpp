#include "xanth/interface.h"

#include "xanth/verbs.h"

namespace Xanth {

Interface::Interface(Game &game, Display &display) : _game(game), _display(display), _selected(-1), _verb(0), _dobj(0) {
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

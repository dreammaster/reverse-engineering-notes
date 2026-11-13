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

void Interface::click(int x, int y) {
	int v = _display.verbAt(x, y);
	if (v >= 0) {
		_selected = v;
		_verb = _verbIds[v];
		_dobj = 0;
		_display.selectVerb(v);
		return;
	}
	int arrow = _display.compassAt(x, y);
	if (arrow >= 0) {
		_verb = 0x51;                               // walk: the direction code is the arrow number + 1
		execute(arrow + 1, 0);
		return;
	}
	int obj = _display.objectAt(x, y);
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
}

} // namespace Xanth

#include "xanth/game.h"

#include "xanth/logic/logic.h"

namespace Xanth {

// Verbs and attributes that the core itself needs (the full tables are in xanth/logic/commands.h and the object attribute table).
enum {
	PHASE_ACTOR_ROOM = 1,     // handler phases of run_command: room and player, then the actor
	PHASE_ACTOR = 2,
	PHASE_IOBJ_PARENT = 3,
	PHASE_IOBJ = 4,
	PHASE_DOBJ_PARENT = 5,
	PHASE_DOBJ = 6,
	PHASE_ROOM_TICK = 0x1B,   // every turn of advance_time
	PHASE_LEAVE = 0xF,        // go_to_room: the room is left, entered (0xD), described (0xE)
	PHASE_ENTER = 0xD,
	PHASE_ENTERED = 0xE,
	PHASE_FOLLOWER = 0x1A
};
enum {
	ATTR_VISITED = 5,
	ATTR_HOLDS_ACTOR = 0x10   // the actor's location is a vehicle / container the actor is carried in
};

static const uint8 kExeDseg[] = {
#include "xanth/logic/exe_dseg.inc"
};

static const uint8 kExeTables[] = {
#include "xanth/logic/exe_tables.inc"
};

Game::Game(Ui &ui_, const Legend::MessageFile &messages_, const ObjectNames &names)
    : ui(ui_), messages(messages_), objectNames(names), _textNext(0), _seed(1) {
	newGame();
}

void Game::newGame() {
	state.reset();
	memset(_dseg, 0, sizeof(_dseg));
	memcpy(_dseg, kExeDseg, sizeof(kExeDseg));
	memcpy(_tables, kExeTables, sizeof(_tables));
	rt = Runtime();
	rt.object_count = (uint16)state.objects.count();
	rt.player_obj = 1;
	rt.actor_obj = 1;
	rt.word_85222 = 1;
}

// ------------------------------------------------------------------------------------------------------------------------------ the turn
int Game::call_object_handler(int obj, int phase) {
	if (obj <= 0 || obj > rt.object_count)
		return 0;
	unsigned idx = (unsigned)state.objects.handlerIndex(obj);
	if (idx == 0 || idx > 0x1C0)
		return 0;
	HandlerFn fn = kObjectHandlers[idx - 1];
	return fn ? fn(*this, phase) : 0;
}

int Game::exec_word_handler(int verb) {
	for (unsigned i = 0; i < kVerbHandlerCount; i++)
		if (kVerbHandlers[i].verb == verb)
			return kVerbHandlers[i].fn(*this, 0);
	return 0;
}

static int typeOf(const ObjectModel &m, int obj) {
	return obj ? m.type(obj) : 0;
}

int Game::run_command(int verb, int dobj, int iobj) {
	Vars &v = state.v;
	rt.run_command_depth++;
	if ((int16)rt.run_command_depth > 1 && verb == rt.selectedCommand && dobj == rt.dobj && iobj == rt.iobj) {
		rt.unk_8AC1C++;       // the same command run from inside itself
		if ((int16)rt.unk_8AC1C > 4) {
			Font_WriteMessage("[Warning: infinite perform recursion terminated]");
			return 2;        // (the original does not undo the depth counter here)
		}
	} else {
		rt.unk_8AC1C = 0;
	}
	int savedVerb = rt.selectedCommand, savedDobj = rt.dobj, savedIobj = rt.iobj;
	rt.selectedCommand = (uint16)verb;
	rt.dobj = (uint16)dobj;
	rt.dobj_type = (uint16)typeOf(state.objects, dobj);
	rt.iobj = (uint16)iobj;
	rt.iobj_type = (uint16)typeOf(state.objects, iobj);

	int result = call_object_handler(v.current_room, PHASE_ACTOR_ROOM);
	if (!result && rt.actor_obj != rt.player_obj)
		result = call_object_handler(rt.player_obj, PHASE_ACTOR_ROOM);
	if (!result) {
		int where = object_get_location(rt.actor_obj);
		if (where != v.current_room)
			result = call_object_handler(where, PHASE_ACTOR_ROOM);
	}
	if (!result)
		result = call_object_handler(rt.actor_obj, PHASE_ACTOR);
	if (!result)
		result = sub_1DF24(*this, rt.selectedCommand);
	if (!result && rt.iobj != result) {
		int where = object_get_location(rt.iobj);
		if (where != 0)
			result = call_object_handler(where, PHASE_IOBJ_PARENT);
		if (!result)
			result = call_object_handler(rt.iobj, PHASE_IOBJ);
	}
	if (!result) {
		if (rt.dobj != 0) {
			int where = object_get_location(rt.dobj);
			if (where != 0)
				result = call_object_handler(where, PHASE_DOBJ_PARENT);
			if (!result)
				result = call_object_handler(rt.dobj, PHASE_DOBJ);
		}
		if (!result)
			result = exec_word_handler(rt.selectedCommand);
	}
	rt.selectedCommand = (uint16)savedVerb;
	rt.dobj = (uint16)savedDobj;
	rt.iobj = (uint16)savedIobj;
	rt.run_command_depth--;
	return result;
}

void Game::executeCommand(int verb, int dobj, int iobj) {
	Vars &v = state.v;
	rt.actor_obj = rt.player_obj;
	rt.byte_850AB = 0;
	rt.selectedCommand = (uint16)verb;
	rt.dobj = (uint16)dobj;
	rt.iobj = (uint16)iobj;
	rt.dobj_type = (uint16)typeOf(state.objects, dobj);
	rt.iobj_type = (uint16)typeOf(state.objects, iobj);
	run_command(rt.selectedCommand, rt.dobj, rt.iobj);
	call_object_handler(rt.player_obj, 7);
	call_object_handler(v.current_room, 7);
	advance_time(rt.word_85222, 0);
	rt.word_85222 = 1;
	v.turn_counter++;
}

int Game::go_to_room(int room) {
	return ::Xanth::go_to_room(*this, room);
}

// ------------------------------------------------------------------------------------------------------------------------------ events
// The event list is the saved one (state.events); an event is an id and a countdown in turns (0xFFFF = every turn). `rt.event_index` is the entry being processed,
// cancel_event and advance_time keep it valid when entries are removed.
int Game::schedule_event(int id, int delay) {
	std::vector<ScheduledEvent> &ev = state.events;
	size_t at = 0;
	while (at < ev.size() && (ev[at].a & 0xFF) != (id & 0xFF))
		at++;
	if (at >= 16) {
		Font_WriteMessage("[ERROR: Queue overflow]");
		return 0;
	}
	if (at == ev.size()) {
		ScheduledEvent e;
		e.a = 0;
		e.b = 0;
		ev.push_back(e);
	}
	ev[at].a = (uint16)(id & 0xFF);
	ev[at].b = (uint16)delay;
	return 0;
}

int Game::cancel_event(int id) {
	std::vector<ScheduledEvent> &ev = state.events;
	for (size_t i = 0; i < ev.size(); i++) {
		if ((ev[i].a & 0xFF) == (id & 0xFF)) {
			ev.erase(ev.begin() + i);
			if ((int)i <= (int16)rt.event_index)
				rt.event_index--;
			break;
		}
	}
	return 0;
}

int Game::event_pending(int id) {
	for (size_t i = 0; i < state.events.size(); i++)
		if ((state.events[i].a & 0xFF) == (id & 0xFF))
			return 1;
	return 0;
}

int Game::advance_time(int mayInterrupt, int turns) {
	std::vector<ScheduledEvent> &ev = state.events;
	if (rt.byte_850AB) {
		rt.byte_850AB = 0;
		return 0;
	}
	uint16 savedIndex = rt.event_index;
	int done = 0;
	if ((int16)turns > 0) {
		for (;;) {
			int fired = 0;
			if (call_object_handler(state.v.current_room, PHASE_ROOM_TICK))
				fired = 1;
			rt.event_index = 0;
			if (!ev.empty()) {
				for (;;) {
					unsigned at = rt.event_index;
					if ((int16)ev[at].b > 0) {
						ev[at].b--;
					} else {
						int id = ev[at].a & 0xFF;
						int before = (int16)ev[at].b;
						if (run_event(*this, id))
							fired++;
						if ((int16)rt.word_85222 > 1) {
							if (turns < (int16)rt.word_85222)
								turns = (int16)rt.word_85222;
							rt.word_85222 = 1;
							mayInterrupt = 0;
						}
						if ((int16)rt.event_index >= 0 && (unsigned)(int16)rt.event_index < ev.size() && (int)(ev[rt.event_index].a & 0xFF) == id) {
							unsigned now = rt.event_index;
							if (ev[now].b == 0 && before == 0) {
								ev.erase(ev.begin() + now);          // the event is over
								rt.event_index--;
							} else if (ev[now].b != 0xFFFF && before <= 0) {
								rt.event_index--;                    // it was set again: look at the same entry once more
							}
						}
					}
					rt.event_index++;
					if ((int16)rt.event_index >= (int)ev.size())
						break;
				}
			}
			sub_1DBB7(1);
			if (mayInterrupt && done + 1 < (int16)turns && fired) {
				if (rt.byte_850AA == 0)
					break;
				RoomItem_Draw(0x20);
				if (!ui.askYesNo("Do you want to continue waiting?"))
					break;
			}
			done++;
			if (done >= (int16)turns)
				break;
		}
	}
	rt.event_index = savedIndex;
	return 0;
}

int Game::add_score(int points, int quiet, int silent) {
	if (points == 0)
		return 0;
	state.v.score += (uint16)points;
	Font_WriteMessage(MSG(7, 2), quiet ? " " : nullptr, points, points > 1 ? "s" : nullptr, silent ? 0x0B : 0x20);
	return 0;
}

// ------------------------------------------------------------------------------------------------------------------------------ text
char *Game::keep(const std::string &s) {
	std::vector<char> &slot = _text[_textNext++ & 7];
	slot.assign(s.begin(), s.end());
	slot.push_back(0);
	return &slot[0];
}

std::string Game::objectName(int obj) {
	for (unsigned i = 0; i < kNameCallbackCount; i++) {
		if (kNameCallbacks[i].obj == obj) {
			TextRef t = kNameCallbacks[i].fn(*this, 0x10, obj);
			if (t.isMessage())
				return messages.string(t.id);
			if (t.text)
				return t.text;
			break;
		}
	}
	return objectNames.name(state.objects.nameOffset(obj));
}

namespace {
class GameNames : public NameProvider {
public:
	explicit GameNames(Game &g) : _g(g) {}
	virtual std::string objectName(int obj) { return _g.objectName(obj); }

private:
	Game &_g;
};
} // namespace

char *Game::print_obj(int style, int obj, const char *verb) {
	if (!state.objects.valid(obj))
		return nullptr;
	GameNames names(*this);
	GrammarContext ctx(state.objects, names, rt.player_obj, rt.player_obj);
	std::string s = formatObject(ctx, (unsigned)style, obj, verb ? verb : "");
	state.objects.setAttr(obj, 6);
	if (style & PO_OUTPUT) {
		ui.printText(s);
		return nullptr;
	}
	return keep(s);
}

char *Game::ResourceText_GetStr(uint32 id) {
	return keep(messages.string(id));
}

std::string Game::format(const std::string &fmt, const FmtArg *args) {
	std::string out;
	size_t n = 0;
	for (size_t i = 0; i < fmt.size(); i++) {
		if (fmt[i] != '%' || i + 1 >= fmt.size()) {
			out.push_back(fmt[i]);
			continue;
		}
		char k = fmt[++i];
		if (k == 's') {
			if (args[n].text)
				out += args[n].text;
			else if (args[n].id)
				out += messages.string(args[n].id);
			n++;
		} else if (k == 'd') {
			out += std::to_string((int)(int16)args[n++].number);
		} else if (k == 'u') {
			out += std::to_string((unsigned)(uint16)args[n++].number);
		} else if (k == 'c') {
			out.push_back((char)args[n++].number);
		} else {
			out.push_back('%');
			out.push_back(k);
		}
	}
	return out;
}

int Game::writeMessage(const std::string &fmt, const FmtArg *args) {
	return ui.printText(format(fmt, args));
}


uint8 &Game::exe8(int seg, int off) {
	unsigned lin = (unsigned)((seg & 0xFFFF) * 16 + (off & 0xFFFF));
	if ((seg & 0xFFFF) == 0x84FC)
		return _dseg[off & 0xFFFF];
	if (lin >= 0x31550 && lin + 1 < 0x34000)
		return _tables[lin - 0x31550];
	_scratch[0] = _scratch[1] = 0;
	return _scratch[0];
}

int Game::_rand() {
	_seed = _seed * 214013u + 2531011u;
	return (int)((_seed >> 16) & 0x7FFF);
}

} // namespace Xanth

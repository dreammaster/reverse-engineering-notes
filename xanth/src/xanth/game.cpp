#include <cstdlib>
#include <cstdio>
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

static const uint8 kExeOverlay[] = {
#include "xanth/logic/exe_overlay.inc"
};

static const struct {
	unsigned lin, size, at;
} kExeOverlayRanges[] = {
#include "xanth/logic/exe_overlay_ranges.inc"
};

Game::Game(Ui &ui_, const Legend::MessageFile &messages_, const ObjectNames &names)
    : ui(ui_), messages(messages_), objectNames(names), _textNext(0), _seed(1) {
	newGame();
}

void Game::newGame() {
	state.reset();
	memset(_dseg, 0, sizeof(_dseg));
	memcpy(_dseg, kExeDseg, sizeof(kExeDseg));
	_seed = kExeDseg[0x5812] | (kExeDseg[0x5813] << 8) | ((unsigned)kExeDseg[0x5814] << 16) | ((unsigned)kExeDseg[0x5815] << 24);   // the C runtime's seed (word_8A7D2/4)
	memcpy(_tables, kExeTables, sizeof(_tables));
	memcpy(_overlay, kExeOverlay, sizeof(_overlay));
	rt = Runtime();
	rt.object_count = (uint16)state.objects.count();
	rt.player_obj = 1;
	rt.actor_obj = 1;
	rt.word_85222 = 1;
	// the segment words of the overlay data (data segments of the overlays that the logic reads tables from): the exe keeps the overlay loader's placeholder values here, the
	// tables lie where the flat image has them (found by their contents; the oracle does not know them either)
	rt.word_8AA64 = rt.word_8AA66 = 0x48F7;       // conversation texts
	rt.word_8AA44 = 0x4483;                       // the weekday names
	rt.word_8AA46 = 0x4488;
	rt.word_8AA76 = 0x4AC0;
	rt.word_8AA7C = 0x4F11;
	rt.word_8AA9C = 0x66B9;
	rt.word_8AA86 = 0x55B8;
	rt.word_8AA8A = 0x5E29;
	rt.word_8AA90 = rt.word_8AA92 = 0x6134;
	rt.word_8AA98 = 0x630E;
	rt.word_8AA9A = 0x6449;
	rt.word_8AAEA = 0x7634;
	rt.word_8AAEE = 0x7969;
	rt.word_8AAF4 = 0x7CF0;                       // the books of the mantle
}

// ------------------------------------------------------------------------------------------------------------------------------ conversations
uint8 *Game::Conversation_GetEntry(int convId, int subId) {
	unsigned id = (unsigned)convId & 0xFFFF;
	if (id >= 0x8000) {
		id &= 0x7FFF;
		if (id >= kConvMenuCount)
			return nullptr;
		if ((int16)subId < 0)
			return &state.v.Conversation_ENTRIES3[id];
		if ((int)kConvMenus[id].count > subId)
			return &state.v.Conversation_ENTRIES2[kConvMenus[id].first + subId];
		return &state.v.Conversation_ENTRIES3[id];
	}
	return id < kConvLineCount ? &state.v.Conversation_ENTRIES1[id] : nullptr;
}

int Game::conversation_set_enabled(int convId, int subId, int on) {
	uint8 *e = Conversation_GetEntry(convId, subId);
	if (e)
		*e = on ? (*e | 1) : (*e & 0xFE);
	return 0;
}

std::string Game::textOf(const ConvEntry &e) {
	TextRef t = e.text ? e.text(*this) : TextRef::message(e.msg);
	return std::string(ResourceText_GetStr(t));
}

int Game::show_compendium(int convNum, int picNum) {
	ui.conversationBegin(picNum);
	unsigned convId = (unsigned)convNum & 0xFFFF;
	int lastMenu = -1, nextChoice = -1;                 // var_8C / var_8E: the menu shown last and the entry after the one chosen there
	for (int steps = 0; convId != 0xFFFF && steps < 60; steps++) {
		if (getenv("XANTH_TRACE_CONV"))
			fprintf(stderr, "conv %X\n", convId);
		if (convId >= 0x8000) {
			unsigned m = convId & 0x7FFF;
			if (m >= kConvMenuCount)
				break;
			const ConvMenu &menu = kConvMenus[m];
			std::vector<int> shown;                       // the options that are enabled
			std::vector<std::string> texts;
			for (unsigned j = 0; j < menu.count; j++) {
				uint8 *flag = Conversation_GetEntry((int)convId, (int)j);
				if (flag && (*flag & 1)) {
					shown.push_back((int)j);
					texts.push_back(textOf(kConvOptions[menu.first + j]));
				}
			}
			if (getenv("XANTH_TRACE_CONV"))
				fprintf(stderr, " menu %u: %d of %u shown\n", m, (int)shown.size(), menu.count);
			int initial = 0;
			if (lastMenu == (int)convId) {
				for (size_t k = 0; k < shown.size(); k++)
					if (shown[k] == nextChoice)
						initial = (int)k;
			} else {
				lastMenu = (int)convId;
			}
			int sel = shown.empty() ? 0 : ui.conversationMenu(texts, initial);
			if (shown.empty() || sel < 0 || sel >= (int)shown.size())
				break;
			nextChoice = sel + 1 < (int)shown.size() ? shown[sel + 1] : 0xFFFF;
			int j = shown[sel];
			if (menu.callback)
				menu.callback(*this, (int)convId, j);
			convId = kConvOptions[menu.first + j].next;
		} else {
			if (convId >= kConvLineCount)
				break;
			const ConvEntry &line = kConvLines[convId];
			uint8 *flag = Conversation_GetEntry((int)convId, 0);
			if (flag && (*flag & 1))
				ui.conversationLine(textOf(line), line.next == 0xFFFF || line.next < 0x8000);
			convId = line.next;
		}
	}
	ui.conversationEnd();
	if (rt.byte_86CC0)
		RoomItem_Draw(0x10);
	rt.byte_86CC0 = 1;
	return 0;
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

int Game::verbListOffset(int obj) {
	if (!state.objects.valid(obj))
		return 0;
	const byte *r = state.objects.record(obj);
	int type = state.objects.type(obj), at = type == 2 ? 0x18 : type == 4 ? 0x0A : (type == 3 || type == 5) ? 0x1C : -1;
	if (at < 0)
		return 0;
	unsigned idx = r[at] | (r[at + 1] << 8);
	return idx ? (int)(0x4624 + idx * 2 + 2) : 0;
}

int Game::object_list_append(int obj) {
	int off = verbListOffset(obj);
	if (off)
		for (; exe16(0x3000, off) != 0; off += 2)
			exe16(0x3000, off) = 0;
	return 0;
}

int Game::sub_1DBB7(int minutes) {
	Vars &v = state.v;
	unsigned t = (unsigned)(uint16)(v.time_of_day_minutes + minutes);
	v.day_count = (uint16)(v.day_count + t / 0x5A0);
	v.time_of_day_minutes = (uint16)(t % 0x5A0);
	return 0;
}

// sub_312CF (sub_508C0): the companion selection screen after Grundy's talk: the picture of the four candidates, each of whom plays an animation; the player clicks one, the others
// go back (the animations backwards), and the game goes on in the first room of Xanth with the companion (room 0x5B), or without when the player gave up (0x5A).
int Game::sub_312CF() {
	Vars &v = state.v;
	static const struct {
		int obj, pic, count;
	} kCandidates[4] = {{6, 0x34A, 0x18}, {0xB, 0x332, 0x18}, {5, 0x31A, 0x18}, {0xA, 0x302, 0x18}};
	call_object_handler(v.current_room, 0xF);
	v.previous_room = v.current_room;
	v.current_room = 0x5A;
	set_room_music(0x5A);
	other("game_over_screen", 0x301);
	for (int i = 0; i < 4; i++)
		other("sub_29F77", kCandidates[i].pic, kCandidates[i].count, 0);
	int ids[4] = {kCandidates[0].obj, kCandidates[1].obj, kCandidates[2].obj, kCandidates[3].obj};
	v.companion_obj = (uint16)ui.chooseCompanion(ids, 4);
	v.word_85252 = v.companion_obj;
	for (int i = 0; i < 4; i++)
		if (kCandidates[i].obj != v.companion_obj)
			other("sub_29F77", kCandidates[i].pic, kCandidates[i].count, 1);
	wait_ticks_or_click(0x7D0);
	int room = v.companion_obj < 1 ? 0x5A : 0x5B;
	v.current_room = (uint16)room;
	other("sub_1EED9");
	v.current_room = 0x5A;
	return go_to_room(room);
}

// sub_1D99F: the sword's story goes one step on (word_8526A counts them): a pause, the property of the hilt (object 0x8B) from a table of the program, the hilt redrawn and the
// text of the step (a table of far pointers to texts, one of them a message) shown.
int Game::sub_1D99F() {
	Vars &v = state.v;
	wputchar(0x0B);
	wait_ticks_or_click(0xFA0);
	v.word_8526A++;
	unsigned n = v.word_8526A;
	object_set_prop_a(0x8B, exe16(0x84FC, (0x404 + 2 * n) & 0xFFFF));
	RoomItem_Draw(0x20);
	Font_WriteMessage(TextRef::fromWords(0x44, 0xF000), exeText(0x3263, (0x7AC + 4 * n) & 0xFFFF));
	return 0;
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

void Game::beginGame() {
	Vars &v = state.v;
	object_set_prop_a(0x56, 0x128);              // the CD-ROM version
	object_clear_attr(0x31, 0xE);
	object_set_default_verb(0x31, 0x2C);
	schedule_event(1, 7);
	schedule_event(3, 0xC);
	v.light_room = v.current_room;
	rt.byte_86CC0 = 1;                           // (the intro has shown a conversation already)
	RoomItem_Draw(0x10);
	set_room_music(v.current_room);
	call_object_handler(v.current_room, 0x19);
	call_object_handler(v.current_room, 9);
	wputchar(0xB);
	object_set_attr(v.current_room, 6);
	object_set_attr(v.current_room, 5);
}

void Game::executeCommand(int verb, int dobj, int iobj) {
	Vars &v = state.v;
	rt.actor_obj = rt.player_obj;
	rt.byte_850AB = 0;
	if (verb == 0x51) {                          // walking: the direction goes to byte_8BE1C
		rt.byte_8BE1C = (uint8)dobj;
		dobj = 0;
	}
	rt.selectedCommand = (uint16)verb;
	rt.dobj = (uint16)dobj;
	rt.iobj = (uint16)iobj;
	rt.dobj_type = (uint16)typeOf(state.objects, dobj);
	rt.iobj_type = (uint16)typeOf(state.objects, iobj);
	rt.run_command_depth = 0;
	run_command(rt.selectedCommand, rt.dobj, rt.iobj);
	call_object_handler(rt.player_obj, 7);
	call_object_handler(v.current_room, 7);
	advance_time(0, rt.word_85222);
	rt.word_85222 = 1;
	v.turn_counter++;
	rt.byte_850AB = 0;
	RoomItem_Draw(0x20);
	ui.putChar(0x0B);                            // (marks the end of the turn's text)
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
	if (getenv("XANTH_TRACE_TURN")) {
		fprintf(stderr, "advance_time(%d, %d) skip=%d:", mayInterrupt, turns, rt.byte_850AB);
		for (size_t i = 0; i < ev.size(); i++)
			fprintf(stderr, " [%u in %d]", ev[i].a, (int16)ev[i].b);
		fprintf(stderr, "\n");
	}
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
						if (run_event(id))
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
	if (s.size() > (1u << 20)) {                      // (a text of the original is never this long: something is reading past the end of its string)
		fprintf(stderr, "keep: a text of %zu bytes\n", s.size());
		abort();
	}
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
	objectName(obj);                  // (the name callback of a thing sets its "proper name" attribute, which the article depends on)
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

char *Game::farPtr(int seg, int off) {
	if ((seg & 0xFF00) == 0xF000)
		return ResourceText_GetStr((((uint32)seg & 0xFFFF) << 16) | ((uint32)off & 0xFFFF));
	unsigned lin = (unsigned)((seg & 0xFFFF) * 16 + (off & 0xFFFF));
	if ((seg & 0xFFFF) == 0x84FC || (lin >= 0x31550 && lin + 1 < 0x35400))
		return (char *)&exe8(seg, off);
	// memory of the original that is not modelled (the logic copies blocks to and from it): a scratch area big enough for any such copy
	if (_farScratch.empty())
		_farScratch.assign(0x20000, 0);
	return (char *)&_farScratch[off & 0xFFFF];
}

char *Game::ResourceText_GetStr(uint32 id) {
	if ((id >> 24) != 0xF0)             // not a message id but a far pointer: text of the original's data segment, or memory that is not modelled (empty)
		return farPtr((int)(id >> 16), (int)(id & 0xFFFF));
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
				out += (args[n].id >> 24) == 0xF0 ? messages.string(args[n].id) : std::string(farPtr((int)(args[n].id >> 16), (int)(args[n].id & 0xFFFF)));
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


// the saved regions that lie in the data segment (offset -> index + 1 into kVarRegions): the logic reaches some of them by address, they are kept in the saved state
static const std::vector<uint8> &dsegRegions() {
	static std::vector<uint8> map;
	if (map.empty()) {
		map.assign(0x10000, 0);
		for (unsigned i = 0; i < ARRAYSIZE(kVarRegions); i++)
			if (kVarRegions[i].address >= 0x84FC0 && kVarRegions[i].address + kVarRegions[i].size <= 0x84FC0 + 0x10000)
				for (unsigned k = 0; k < kVarRegions[i].size; k++)
					map[kVarRegions[i].address - 0x84FC0 + k] = (uint8)(i + 1);
	}
	return map;
}

uint8 &Game::exe8(int seg, int off) {
	unsigned lin = (unsigned)((seg & 0xFFFF) * 16 + (off & 0xFFFF));
	if ((seg & 0xFFFF) == 0x84FC) {
		unsigned r = dsegRegions()[off & 0xFFFF];
		if (r) {
			const VarRegion &rg = kVarRegions[r - 1];
			return reinterpret_cast<uint8 *>(&state.v)[rg.offset + (lin - rg.address)];
		}
		return _dseg[off & 0xFFFF];
	}
	for (unsigned i = 0; i < ARRAYSIZE(kExeOverlayRanges); i++)                    // (the tables of the overlays' data segments: read only)
		if (lin >= kExeOverlayRanges[i].lin && lin < kExeOverlayRanges[i].lin + kExeOverlayRanges[i].size) {
			return _overlay[kExeOverlayRanges[i].at + (lin - kExeOverlayRanges[i].lin)];
		}
	if (lin >= 0x34628 && lin < 0x34628 + sizeof(state.v.unk_34628))          // (the verb lists of the objects: saved with the game)
		return state.v.unk_34628[lin - 0x34628];
	if (lin >= 0x31550 && lin + 1 < 0x35400)
		return _tables[lin - 0x31550];
	_scratch[0] = _scratch[1] = 0;
	return _scratch[0];
}

void Game::walkContents(int root, bool visibleOnly, const std::function<bool(int)> &visit) {
	walkContentsEx(root, visibleOnly, [&](int obj, int, int) { return visit(obj); });
}

void Game::walkContentsEx(int root, bool visibleOnly, const std::function<bool(int, int, int)> &visit) {
	struct Frame {
		int obj, slot, kind;
	};
	std::vector<Frame> stack;
	int cur = root, slot = 0, kind = -1, childKind = 0;
	while (cur != 0) {
		if (visit(cur, (int)stack.size(), kind))
			return;
		for (;;) {
			int head = 0;
			int slots = state.objects.valid(cur) ? (state.objects.type(cur) == OT_CONTAINER ? 2 : state.objects.type(cur) == OT_CHARACTER ? 3 : 1) : 0;
			for (; slot < slots; slot++) {
				if (!visibleOnly || sub_1DA1E(*this, cur, slot)) {
					head = state.objects.listHead(cur, slot);
					if (head)
						break;
				}
			}
			if (head) {
				if (cur == root && state.objects.type(root) == OT_ROOM)
					childKind = 0;
				else if (cur == rt.actor_obj)
					childKind = 1;
				else if (kind == 0)
					childKind = 2;
				else if (kind == 1)
					childKind = 3;
				Frame f = {cur, slot, kind};
				stack.push_back(f);
				cur = head;
				slot = 0;
				kind = childKind;
				break;                          // visit the first child
			}
			if (stack.empty()) {
				cur = 0;
				break;
			}
			int sibling = state.objects.nextSibling(cur);
			if (sibling) {
				cur = sibling;
				slot = 0;
				break;                          // visit the next sibling
			}
			Frame f = stack.back();             // back to the parent, on to its next slot
			stack.pop_back();
			cur = f.obj;
			slot = f.slot + 1;
			kind = f.kind;
		}
	}
}

int Game::sub_1AC45(int id) {
	for (size_t i = 0; i < state.events.size(); i++)
		if ((state.events[i].a & 0xFF) == (id & 0xFF))
			return state.events[i].b;
	return 0x7FFF;
}

int Game::run_event(int id) {
	if ((int16)id <= 0 || id > 0x2D)
		return 0;
	unsigned idx = (unsigned)id - 1;
	if (idx < kEventFunctionCount && kEventFunctions[idx])
		return kEventFunctions[idx](*this, 0);
	return 0;
}

int Game::sub_1FB15(int index, int phase) {
	if ((unsigned)index < kExitFunctionCount && kExitFunctions[index])
		return kExitFunctions[index](*this, phase);
	return 0;
}

int Game::exitField(int room, int offset, int size) const {
	if (state.objects.type(room) != OT_ROOM)
		return 0;
	const byte *r = state.objects.record(room);
	return size == 1 ? r[offset] : (r[offset] | (r[offset + 1] << 8));
}

int Game::_rand() {
	_seed = _seed * 214013u + 2531011u;
	return (int)((_seed >> 16) & 0x7FFF);
}

} // namespace Xanth

/* The game: state, the command core and the engine functions that the object handlers, exits, events and conversations call.
 *
 * The functions keep the names of the original (object_has_attr, go_to_room, Font_WriteMessage ...) so that the generated logic in xanth/logic reads like the
 * source it was derived from; a later pass can rename them. Everything that needs a screen, mouse, sound card or timer goes through `Ui` (ui.h).
 */
#ifndef XANTH_GAME_H
#define XANTH_GAME_H

#include "legend/messages.h"
#include "xanth/gamestate.h"
#include "xanth/grammar.h"
#include "xanth/objectnames.h"
#include "xanth/ui.h"

namespace Xanth {

/** The variables of the original that are not part of a save game: the command being run, the event queue's cursor, option flags and many one-off values. */
struct Runtime {
#include "xanth/logic/runtime_vars.inc"
	Runtime() {
		memset(this, 0, sizeof(*this));
		Runtime &rt = *this;
#include "xanth/logic/runtime_init.inc"
	}
};

/** A 16-bit word of the original: a number, or - where the original passes a near pointer - a pointer to text. The generated logic uses it for parameters. */
struct Word {
	intptr_t v;
	Word(int x = 0) : v(x) {}
	Word(unsigned x) : v((int)x) {}
	Word(intptr_t x) : v(x) {}
	Word(const char *p) : v((intptr_t)p) {}
	template <class R, class... A> Word(R (*f)(A...)) : v((intptr_t)f) {}
	operator int() const { return (int)v; }
};

/** What the conversation text routines return: a message id (the original returns 0xF0BBIIII in dx:ax) or a pointer to a string it built. */
struct TextRef {
	uint32 id;
	const char *text;
	TextRef() : id(0), text(nullptr) {}
	TextRef(const char *s) : id(0), text(s) {}
	TextRef(uint32 i) : id(i), text(nullptr) {}
	static TextRef message(uint32 i) { TextRef t; t.id = i; return t; }
	static TextRef fromWords(Word lo, Word hi) { TextRef t; t.id = ((uint32)(uint16)(int)hi << 16) | (uint16)(int)lo; return t; }
	bool isMessage() const { return id != 0; }
	operator char *() const { return const_cast<char *>(text); }
	operator Word() const { return Word(text ? (intptr_t)text : (intptr_t)id); }
};

/** An argument of a formatted message: a number for %d / %u / %c or a string for %s. */
struct FmtArg {
	bool isString;
	int number;
	const char *text;
	uint32 id;                 // a message id passed where a string is expected
	FmtArg() : isString(false), number(0), text(nullptr), id(0) {}
	FmtArg(int n) : isString(false), number(n), text(nullptr), id(0) {}
	FmtArg(unsigned n) : isString((n >> 24) == 0xF0), number((int)n), text(nullptr), id((n >> 24) == 0xF0 ? n : 0) {}     // a message id passed for %s
	FmtArg(intptr_t n) : isString(false), number((int)n), text(nullptr), id(0) {}
	FmtArg(const char *s) : isString(true), number(0), text(s), id(0) {}
	FmtArg(const TextRef &t) : isString(true), number(0), text(t.text), id(t.id) {}
};

/** Message id as the original passes it: 0xF0BBIIII = block BB, index IIII of the message file. */
#define MSG(block, index) ((::uint32)(0xF0000000u | ((::uint32)(block) << 16) | (::uint32)(index)))

class Game {
public:
	Game(Ui &ui, const Legend::MessageFile &messages, const ObjectNames &names);

	GameState state;
	Runtime rt;
	Ui &ui;
	const Legend::MessageFile &messages;
	const ObjectNames &objectNames;

	/** The current name of an object: the callback of the 49 objects that have one (their name depends on what the player knows), else the name in the record. */
	std::string objectName(int obj);

	/** A new game: the state of the original after its start-up. */
	void newGame();

	// ---- the turn (execute_command and what it calls)
	/** Runs the command that the interface built (selected verb, direct and indirect object). Returns after the timed events have run. */
	void executeCommand(int verb, int dobj, int iobj);
	int run_command(int verb, int dobj, int iobj);
	int call_object_handler(int obj, int phase);
	int exec_word_handler(int verb);
	int go_to_room(int room);
	int advance_time(int mayInterrupt, int turns);
	int schedule_event(int id, int delay);
	int cancel_event(int id);
	int event_pending(int id);
	int add_score(int points, int quiet, int silent);

	// ---- objects (the original's names)
	int object_has_attr(int obj, int attr) { return state.objects.hasAttr(obj, attr) ? 1 : 0; }
	int object_set_attr(int obj, int attr) { state.objects.setAttr(obj, attr); return 0; }
	int object_clear_attr(int obj, int attr) { state.objects.clearAttr(obj, attr); return 0; }
	int object_parent_is(int obj, int parent) { return state.objects.parentIs(obj, parent) ? 1 : 0; }
	int object_move(int obj, int parent, int slot) { state.objects.move(obj, parent, slot); return 0; }
	int object_get_location(int obj) { return state.objects.location(obj); }
	int object_get_slot(int obj) { return state.objects.slot(obj); }
	int object_get_list_head(int parent, int slot) { return state.objects.listHead(parent, slot); }
	int object_get_next_sibling(int obj) { return state.objects.nextSibling(obj); }
	int object_get_points(int obj) { return state.objects.points(obj); }
	int object_set_points(int obj, int v) { state.objects.setPoints(obj, v); return 0; }
	int object_get_prop_a(int obj) { return state.objects.propA(obj); }
	int object_set_prop_a(int obj, int v) { state.objects.setPropA(obj, v); return 0; }
	int object_get_default_verb(int obj) { return state.objects.defaultVerb(obj); }
	int object_set_default_verb(int obj, int v) { state.objects.setDefaultVerb(obj, v); return 0; }
	int object_is_at(int obj, int parent, int slot) { return state.objects.isAt(obj, parent, slot) ? 1 : 0; }
	int object_is_within(int obj, int ancestor) { return state.objects.isWithin(obj, ancestor) ? 1 : 0; }

	// ---- text
	/** The text of a reference to an object (print_obj); valid until a few more calls have been made. */
	char *print_obj(int style, int obj, const char *verb = nullptr);
	/** A message of the message file as text (ResourceText_GetStr); valid until a few more calls have been made. */
	char *ResourceText_GetStr(uint32 id);
	char *ResourceText_GetStr(TextRef t) { return t.isMessage() ? ResourceText_GetStr(t.id) : const_cast<char *>(t.text ? t.text : ""); }
	/** Prints a message or a format string with arguments (Font_WriteMessage). */
	template <class... A> int Font_WriteMessage(uint32 msg, A... args) {
		FmtArg a[] = {FmtArg(args)..., FmtArg()};
		return writeMessage(messages.string(msg), a);
	}
	template <class... A> int Font_WriteMessage(TextRef t, A... args) {
		FmtArg a[] = {FmtArg(args)..., FmtArg()};
		return writeMessage(t.isMessage() ? messages.string(t.id) : std::string(t.text ? t.text : ""), a);
	}
	template <class... A> int Font_WriteMessage(const char *format, A... args) {
		FmtArg a[] = {FmtArg(args)..., FmtArg()};
		return writeMessage(format, a);
	}
	template <class... A> int show_text_box(uint32 msg, A... args) {
		FmtArg a[] = {FmtArg(args)..., FmtArg()};
		return ui.showTextBox(format(messages.string(msg), a));
	}
	int RoomItem_Draw(int what) { return other("RoomItem_Draw", what); }
	int set_room_music(int room) { return other("set_room_music", room); }
	int sub_1DBB7(int a) { return other("sub_1DBB7", a); }
	/** sprintf into one of the original's static buffers (the C runtime's, with %s / %d / %u / %c). */
	template <class... A> int _sprintf(char *dest, const char *fmt, A... args) {
		FmtArg a[] = {FmtArg(args)..., FmtArg()};
		std::string s = format(fmt, a);
		memcpy(dest, s.c_str(), s.size() + 1);
		return (int)s.size();
	}
	template <class... A> int show_text_box(TextRef t, A... args) {
		FmtArg a[] = {FmtArg(args)..., FmtArg()};
		return ui.showTextBox(format(t.isMessage() ? messages.string(t.id) : std::string(t.text ? t.text : ""), a));
	}
	int wputchar(int ch) { return ui.putChar(ch); }
	int wait_ticks_or_click(int ticks) { return ui.waitTicksOrClick(ticks); }
	int realsound_play(const char *file) { return ui.playSound(file); }
	int Font_WriteString(const char *text) { return ui.printText(text); }

	/** A routine of the original that only draws or animates; see Ui::other. */
	int other(const char *name, Word a0 = 0, Word a1 = 0, Word a2 = 0, Word a3 = 0, Word a4 = 0) {
		intptr_t args[5] = {a0.v, a1.v, a2.v, a3.v, a4.v};
		return ui.other(name, args, 5);
	}

#include "xanth/logic/engine_other.inc"

	/** Placeholders for the logic that indexes the original's data segment by pointer (see the function list in logic/manual_review.txt); they read a private copy. */
	/** Byte / word of the original's static tables at segment:offset (the logic indexes them through segment variables); 0x84FC is the data segment placeholder. */
	uint8 &exe8(int seg, int off);
	uint16 &exe16(int seg, int off) { return *reinterpret_cast<uint16 *>(&exe8(seg, off)); }
	/** The far pointer / message id stored as a dword in one of the static tables. */
	TextRef exeText(int seg, int off) { return TextRef::fromWords(exe16(seg, off), exe16(seg, off + 2)); }
	uint16 &dseg16(int off) { return *reinterpret_cast<uint16 *>(&_dseg[off & 0xFFFE]); }
	uint8 &dseg8(int off) { return _dseg[off & 0xFFFF]; }

	// ---- random numbers (the C runtime's generator of the original)
	int _rand();
	/** random_percent: true with the given percent chance. */
	int random_percent(int percent) { return (_rand() % 100) < percent ? 1 : 0; }

private:
	std::string format(const std::string &fmt, const FmtArg *args);
	int writeMessage(const std::string &fmt, const FmtArg *args);

	std::vector<char> _text[8];     // results of print_obj / ResourceText_GetStr
	unsigned _textNext;
	char *keep(const std::string &s);
	unsigned _seed;
	uint8 _dseg[0x10000];
	uint8 _tables[0x34000 - 0x31550];
	uint8 _scratch[2];
};

} // namespace Xanth

#endif

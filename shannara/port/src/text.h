// print_obj: the engine's sentence-building helper (0x1C0E8, ~2,500 call sites). Turns an object into the words that fill a "%s" in a
// message: article / pronoun / possessive / reflexive, the object's name, an optional trailing period and a verb conjugated for the
// subject ("take" -> "takes" for Shella, "take" for "you"). A direct port, differentially tested against the original (tests/text_fuzz.py).
#pragma once
#include "objectdat.h"
#include "world.h"

namespace shannara {

class Text {
public:
	// flags (low byte of print_obj's first argument)
	enum Flags { kFormMask = 7, kCapital = 0x10, kPeriod = 0x20, kVerb = 0x40, kPrintNow = 0x80 };
	// forms: 0 nothing, 1 name only, 2 "the"/none + name, 3 "a/an/some"/none + name, 4 subject pronoun, 5 possessive, 6 object pronoun, 7 reflexive

	Text(const LeExe &exe, const ObjectDat &names, World &world);

	// Returns false where the original returns NULL (invalid object, or the 0x80 "print immediately" mode, in which `printed` receives
	// the text instead). Marks the object as mentioned (attribute 6) exactly like the original.
	bool printObj(uint32_t flags, int obj, const char *verb, std::string &out, std::string *printed = nullptr);
	// the object's name as object_get_name? returns it ("" when it has none)
	std::string name(int obj) const;
	bool hasName(int obj) const;

private:
	bool isPlayer(int obj) const { return obj == _playerA || obj == _playerB; }
	bool plural(int obj) const { return _w.hasAttr(obj, 3) && !_w.hasAttr(obj, 1); }

	const ObjectDat &_names;
	World &_w;
	int _playerA, _playerB; // word_ADEB0 / word_ADEB2: the player characters ("you")
	int _specialObject;     // the one object with a computed name (0xAFF50 table)
};

} // namespace shannara

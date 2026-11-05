/* The English grammar helper of the game: `print_obj` of the original. It builds the text of a reference to an object - an article or pronoun, the object's name, and
 * optionally a verb phrase conjugated to agree with the object - for the messages the handlers print ("You see %s here.", "%s fall%s", ...).
 *
 * style = flags + kind:
 *   kind (style & 7)   0 nothing (only the verb phrase)      1 name                      2 article + name: "the switch" (proper names get none)
 *                      3 indefinite: "a" / "an" / "some" + name                           4 subject pronoun (you, they, she, he, it)
 *                      5 possessive (your, their, her, his, its)                          6 object pronoun (you, them, her, him, it)
 *                      7 reflexive (yourself, themselves, herself, himself, itself)
 *   flags              0x10 capitalise the first letter (otherwise the text starts with a space)     0x20 append a full stop
 *                      0x40 append the verb phrase passed in, conjugated for a singular third person ("are" -> "is", "do" -> "does", "look" -> "looks", "watch" -> "watches")
 *                      0x80 print the result as a message instead of returning it
 *
 * The attributes that matter (see xanth_overview.md section 12): 1 and 3 mark plural / mass things ("some", "they"), 2 proper names (no article), 4 words that start with
 * a vowel sound ("an"), 22 female characters; type 5 objects are characters.
 */
#ifndef XANTH_GRAMMAR_H
#define XANTH_GRAMMAR_H

#include "xanth/objectnames.h"
#include "xanth/objects.h"

namespace Xanth {

enum PrintObjFlags {
	PO_NONE = 0,
	PO_NAME = 1,
	PO_THE = 2,
	PO_A = 3,
	PO_SUBJECT = 4,
	PO_POSSESSIVE = 5,
	PO_OBJECT_PRONOUN = 6,
	PO_REFLEXIVE = 7,
	PO_CAPITAL = 0x10,
	PO_PERIOD = 0x20,
	PO_AGREE = 0x40,
	PO_OUTPUT = 0x80
};

/** Where an object's current name comes from. 49 objects have a name callback in the original (object handler phase 0x10: the name depends on what the player has
 * learned - an unknown character is "tiny man" until introduced - and the callback also sets or clears attribute 2, proper name); the game implements this
 * interface with those callbacks. */
class NameProvider {
public:
	virtual ~NameProvider() {}
	virtual std::string objectName(int obj) = 0;
};

/** The names stored in the object records (OBJECT.DAT), without any callback. */
class RecordNames : public NameProvider {
public:
	RecordNames(const ObjectModel &objects, const ObjectNames &names) : _objects(objects), _names(names) {}
	virtual std::string objectName(int obj) { return _names.name(_objects.nameOffset(obj)); }

private:
	const ObjectModel &_objects;
	const ObjectNames &_names;
};

struct GrammarContext {
	const ObjectModel &objects;
	NameProvider &names;
	int playerObj;       // the player character: always "you"
	int secondPersonObj; // word_85228 of the original: a second object that is addressed as "you" (always 1, the player, in the game)
	GrammarContext(const ObjectModel &o, NameProvider &n, int player, int second) : objects(o), names(n), playerObj(player), secondPersonObj(second) {}
};

/** The text print_obj builds (at most 126 characters). Returns an empty string for an invalid object. `verb` is used when PO_AGREE is set. */
std::string formatObject(const GrammarContext &ctx, unsigned style, int obj, const std::string &verb);

} // namespace Xanth

#endif

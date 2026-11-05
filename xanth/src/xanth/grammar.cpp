#include "xanth/grammar.h"

namespace Xanth {

// Attributes used by the grammar (numbers as in the original's attribute table).
enum {
	ATTR_PLURAL = 1,       // "some", "they"
	ATTR_PROPER = 2,       // proper name: no article
	ATTR_MASS = 3,         // mass noun: "some"; with ATTR_PLURAL cleared it is still referred to as "they"
	ATTR_VOWEL = 4,        // the name starts with a vowel sound: "an"
	ATTR_FEMALE = 0x16
};

static const size_t kMaxResult = 126;

/* The English words of the grammar are part of the engine (they are in the original's data segment, not in the message files). */
static const char *const kThe = "the", *const kSome = "some", *const kAn = "an", *const kA = "a";

struct PronounSet {
	const char *you, *they, *she, *he, *it;
};
static const PronounSet kSubject = {"you", "they", "she", "he", "it"};
static const PronounSet kPossessive = {"your", "their", "her", "his", "its"};
static const PronounSet kObjectPronoun = {"you", "them", "her", "him", "it"};
static const PronounSet kReflexive = {"yourself", "themselves", "herself", "himself", "itself"};

/** Verbs whose third person singular is irregular: the plural (infinitive) form and its singular. */
struct IrregularVerb {
	const char *plural, *singular;
};
static const IrregularVerb kIrregularVerbs[] = {{"are", "is"}, {"were", "was"}, {"have", "has"}, {"do", "does"}, {"go", "goes"}};

/** Verbs ending like this get "es" or "ies" (the original's table: the two-letter ending is replaced by the replacement, but only the ending's own letters
 * count - "ss" is replaced by "sses", "ry" by "ies", which makes carry -> "caries": a quirk of the original that is kept). */
struct SuffixRule {
	const char *ending;
	const char *replacement; // replaces the ending
};
static const SuffixRule kSuffixRules[] = {{"ry", "ies"}, {"ly", "ies"}, {"ss", "sses"}, {"zz", "zzes"}, {"sh", "shes"}, {"ch", "ches"}};

static bool endsWith(const std::string &s, const char *tail) {
	size_t n = strlen(tail);
	return s.size() >= n && s.compare(s.size() - n, n, tail) == 0;
}

/** The verb phrase in the third person singular ("are" -> "is", "look" -> "looks"). */
static std::string singularVerb(const std::string &verb) {
	for (size_t i = 0; i < ARRAYSIZE(kIrregularVerbs); i++) {
		const char *pl = kIrregularVerbs[i].plural;
		size_t n = strlen(pl);
		if (endsWith(verb, pl) && (verb.size() == n || verb[verb.size() - n - 1] == ' '))
			return verb.substr(0, verb.size() - n) + kIrregularVerbs[i].singular;
	}
	if (verb.size() > 2) {
		for (size_t i = 0; i < ARRAYSIZE(kSuffixRules); i++)
			if (endsWith(verb, kSuffixRules[i].ending))
				return verb.substr(0, verb.size() - 2) + kSuffixRules[i].replacement;
	}
	return verb + "s";
}

std::string formatObject(const GrammarContext &ctx, unsigned style, int obj, const std::string &verb) {
	const ObjectModel &om = ctx.objects;
	if (!om.valid(obj))
		return std::string();

	const bool isYou = obj == ctx.playerObj || obj == ctx.secondPersonObj;
	// plural reference: "they", "them" - mass nouns that are not marked plural as well
	const bool plural = om.hasAttr(obj, ATTR_MASS) && !om.hasAttr(obj, ATTR_PLURAL);

	std::string s;

	std::string phrase;      // article or pronoun
	bool withName = false;   // followed by the name
	switch (style & 7) {
	case PO_NAME:
		withName = true;
		break;
	case PO_THE:
		if (!om.hasAttr(obj, ATTR_PROPER))
			phrase = kThe;
		withName = true;
		break;
	case PO_A:
		if (!om.hasAttr(obj, ATTR_PROPER)) {
			if (om.hasAttr(obj, ATTR_MASS) || om.hasAttr(obj, ATTR_PLURAL))
				phrase = kSome;
			else if (om.hasAttr(obj, ATTR_VOWEL))
				phrase = kAn;
			else
				phrase = kA;
		}
		withName = true;
		break;
	case PO_SUBJECT:
	case PO_POSSESSIVE:
	case PO_OBJECT_PRONOUN:
	case PO_REFLEXIVE: {
		const PronounSet &p = (style & 7) == PO_SUBJECT ? kSubject : (style & 7) == PO_POSSESSIVE ? kPossessive : (style & 7) == PO_OBJECT_PRONOUN ? kObjectPronoun : kReflexive;
		// the reflexive form of the player only exists for the player character itself
		const bool you = (style & 7) == PO_REFLEXIVE ? obj == ctx.playerObj : isYou;
		if (you)
			phrase = p.you;
		else if (plural)
			phrase = p.they;
		else if (om.type(obj) == OT_CHARACTER)
			phrase = om.hasAttr(obj, ATTR_FEMALE) ? p.she : p.he;
		else
			phrase = p.it;
		break;
	}
	default:
		break;
	}

	// without PO_CAPITAL a phrase starts with a space, so that it can follow other text
	if (!phrase.empty() || withName) {
		if (!(style & PO_CAPITAL))
			s = " ";
	}
	const size_t start = s.size();
	if (!phrase.empty()) {
		s += phrase;
		if (withName)
			s += ' ';
	}
	if (withName) {
		std::string name = ctx.names.objectName(obj);
		s += name;
	}
	if (s.size() > start && (style & PO_CAPITAL) && s[start] >= 'a' && s[start] <= 'z')
		s[start] = (char)(s[start] - 'a' + 'A');

	if ((style & PO_AGREE) && !verb.empty()) {
		s += ' ';
		if (isYou || plural)
			s += verb;
		else
			s += singularVerb(verb);
	}
	if (style & PO_PERIOD)
		s += '.';
	if (s.size() > kMaxResult)
		s.resize(kMaxResult);
	return s;
}

} // namespace Xanth

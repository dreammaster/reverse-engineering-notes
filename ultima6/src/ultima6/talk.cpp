#include "ultima6/talk.h"

#include "files/converse.h"
#include "ultima6/session.h"
#include "ultima6/stats_view.h"

#include <algorithm>
#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

// Offsets into ObjList::globals (objlist +0x1BF1 on)
enum {
	G_QUEST = 0x1BF1 - 0x1BF1,
	G_KARMA = 0x1BF9 - 0x1BF1,
	G_GARGISH = 0x1C5F - 0x1BF1,
	G_GENDER = 0x1C71 - 0x1BF1
};

Talk::Talk(World &world, TextWindow &scroll, BorlandRandom &rng, const GameClock &clock, int actor)
	: _world(world), _scroll(scroll), _rng(rng), _clock(clock), _actor(actor), _portrait(actor) {
}

bool Talk::load(Bytes &script) const {
	ConverseScript s;
	if (!_world.script(_actor, s))
		return false;
	script = s.bytes();
	return true;
}

bool Talk::canTalk(const World &world, int actor) {
	ConverseScript s;
	return world.script(actor, s);
}

std::string Talk::_nameOfActor(int actor) const {
	ConverseScript s;
	if (!_world.script(actor, s))
		return "";
	std::string n = s.name();
	std::replace(n.begin(), n.end(), '_', '.');
	return n;
}

bool Talk::start() {
	Bytes script;
	if (!load(script))
		return false;
	_conv.reset(new Conversation(script, _actor, *this));
	const ObjList &a = _world.actors;
	_conv->setVar(Conversation::VAR_SEX, a.globals[G_GENDER]);
	_conv->setVar(Conversation::VAR_KARMA, a.globals[G_KARMA]);
	_conv->setVar(Conversation::VAR_GARGF, a.globals[G_GARGISH]);
	_conv->setVar(Conversation::VAR_PARTYLIVE, a.partySize > 0 ? a.partySize - 1 : 0);
	_conv->setVar(Conversation::VAR_PARTYALL, a.partySize > 0 ? a.partySize - 1 : 0);
	_conv->setVar(Conversation::VAR_HP, a.hp[1]);
	_conv->setSVar(Conversation::VAR_PARTYLIVE, _nameOfActor(_actor)); // slot 0x17 doubles as the speaker's name
	_conv->setSVar(0x19, playerName());
	_conv->setVar(Conversation::VAR_QUESTF, a.globals[G_QUEST]);
	_conv->setVar(Conversation::VAR_WORKTYPE, a.worktype[_actor]);
	advance();
	return true;
}

void Talk::advance() {
	_state = _conv->run();
}

bool Talk::key(int k) {
	if (!_conv)
		return false;
	switch (_state) {
	case Conversation::NEED_KEY:
		_conv->giveKey();
		advance();
		break;
	case Conversation::NEED_CHAR: {
		const std::string &ok = _conv->allowed();
		if (k >= 32 && k < 127 && (ok.empty() || ok.find((char)k) != std::string::npos || ok.find((char)std::toupper(k)) != std::string::npos ||
								   ok.find((char)std::tolower(k)) != std::string::npos)) {
			_scroll.print(std::string(1, (char)k));
			_conv->giveChar(k);
			advance();
		}
		break;
	}
	case Conversation::NEED_LINE:
		if (k == KEY_ENTER) {
			std::string line = _typed;
			_typed.clear();
			_conv->giveLine(line);
			advance();
		} else if (k == KEY_BACKSPACE) {
			if (!_typed.empty()) {
				_typed.erase(_typed.size() - 1);
				_scroll.print("\b");
			}
		} else if (k >= 32 && k < 127 && _typed.size() < 30) {
			_typed += (char)k;
			_scroll.print(std::string(1, (char)k));
		}
		break;
	case Conversation::ENDED:
		break;
	}
	return _state != Conversation::ENDED;
}

// ---- ConversationHost ----

void Talk::print(const std::string &text) {
	_scroll.print(text);
}

std::string Talk::playerName() {
	return (const char *)_world.actors.partyNames[0];
}

std::string Talk::genderTitle() {
	return _world.actors.globals[G_GENDER] ? "milady" : "milord";
}

// APPROX(8): not the original's exact behaviour, see docs/approximations.md
std::string Talk::timeOfDay() {
	return _clock.hour < 12 ? "morning" : _clock.hour < 18 ? "afternoon" : "evening";
}

int Talk::random(int lo, int hi) {
	return lo + _rng.next() % (hi - lo + 1);
}

std::string Talk::scriptName(int npc) {
	return _nameOfActor(npc);
}

int Talk::rosterIndex(int actor) const {
	const ObjList &a = _world.actors;
	for (int i = 0; i < a.partySize; ++i)
		if (a.partyRoster[i] == actor)
			return i;
	return -1;
}

std::string Talk::partyName(int npc, bool &in) {
	int i = rosterIndex(npc);
	in = i >= 0;
	if (i < 0 || i >= ObjList::PARTY_NAMES)
		return "";
	return (const char *)_world.actors.partyNames[i];
}

int Talk::talkFlags(int npc) {
	return npc >= 0 && npc < ObjList::ACTORS ? _world.actors.talkFlags[npc] : 0;
}

void Talk::setTalkFlags(int npc, int flags) {
	if (npc >= 0 && npc < ObjList::ACTORS)
		_world.actors.talkFlags[npc] = (uint8_t)flags;
}

bool Talk::inParty(int npc) {
	return rosterIndex(npc) >= 0;
}

int Talk::partyMember(int index) {
	const ObjList &a = _world.actors;
	return index >= 0 && index < a.partySize ? a.partyRoster[index] : 0;
}

// APPROX(33): not the original's exact behaviour, see docs/approximations.md
int Talk::joinParty(int npc) {
	ObjList &a = _world.actors;
	if (rosterIndex(npc) >= 0)
		return 3;
	if (a.partySize >= ObjList::PARTY_NAMES || a.partySize >= 8)
		return 2;
	int i = a.partySize++;
	a.partyRoster[i] = (uint8_t)npc;
	std::string n = _nameOfActor(npc);
	std::memset(a.partyNames[i], 0, ObjList::NAME_SIZE);
	std::memcpy(a.partyNames[i], n.data(), std::min<size_t>(n.size(), ObjList::NAME_SIZE - 1));
	a.status[npc] |= 0x80; // in the party (actors' status array)
	return 0;
}

int Talk::leaveParty(int npc) {
	ObjList &a = _world.actors;
	int i = rosterIndex(npc);
	if (i < 0)
		return 2;
	for (int j = i; j + 1 < a.partySize; ++j) {
		a.partyRoster[j] = a.partyRoster[j + 1];
		std::memcpy(a.partyNames[j], a.partyNames[j + 1], ObjList::NAME_SIZE);
	}
	--a.partySize;
	a.status[npc] &= ~0x80;
	return 0;
}

bool Talk::npcNearby(int npc) {
	int a = _world.actorObject(_actor), b = _world.actorObject(npc);
	if (a < 0 || b < 0)
		return false;
	const World::Obj &p = _world.objects[a], &q = _world.objects[b];
	return p.z == q.z && std::abs(p.x - q.x) < 6 && std::abs(p.y - q.y) < 6;
}

bool Talk::wounded(int npc) {
	return npc >= 0 && npc < ObjList::ACTORS && _world.actors.hp[npc] < ActorStats::maxHealth(_world, npc);
}

bool Talk::poisoned(int npc) {
	return npc >= 0 && npc < ObjList::ACTORS && (_world.actors.status[npc] & 0x08);
}

void Talk::heal(int npc) {
	if (npc >= 0 && npc < ObjList::ACTORS)
		_world.actors.hp[npc] = (uint8_t)ActorStats::maxHealth(_world, npc);
}

void Talk::cure(int npc) {
	if (npc >= 0 && npc < ObjList::ACTORS)
		_world.actors.status[npc] &= ~0x08;
}

void Talk::resurrect(int) {
	// bringing a dead party member back needs the corpse bookkeeping of the object code (not ported yet)
}

void Talk::setWorktype(int npc, int worktype) {
	if (npc >= 0 && npc < ObjList::ACTORS)
		_world.actors.worktype[npc] = (uint8_t)worktype;
}

int Talk::adjustStat(int npc, int stat, int delta) {
	if (npc < 0 || npc >= ObjList::ACTORS)
		return 0;
	ObjList &a = _world.actors;
	switch (stat) {
	case 0:
		a.exp[npc] = (uint16_t)(a.exp[npc] + delta);
		return a.exp[npc];
	case 1:
		a.level[npc] = (uint8_t)(a.level[npc] + delta);
		return a.level[npc];
	case 2:
		a.strength[npc] = (uint8_t)(a.strength[npc] + delta);
		return a.strength[npc];
	case 3:
		a.intelligence[npc] = (uint8_t)(a.intelligence[npc] + delta);
		return a.intelligence[npc];
	default:
		a.dexterity[npc] = (uint8_t)(a.dexterity[npc] + delta);
		return a.dexterity[npc];
	}
}

bool Talk::hasObject(int npc, int obj, int quality, bool checkQuality) {
	std::vector<int> inv;
	_world.inventory(npc, inv);
	for (int i : inv) {
		const World::Obj &o = _world.objects[i];
		if (o.type == obj && (!checkQuality || o.quality == quality))
			return true;
	}
	return false;
}

int Talk::countObject(int npc, int obj) {
	std::vector<int> inv;
	_world.inventory(npc, inv);
	int n = 0;
	for (int i : inv)
		if (_world.objects[i].type == obj)
			n += _world.quantityOf(_world.objects[i]);
	return n;
}

void Talk::newObject(int npc, int obj, int quality, int quantity) {
	_world.addToInventory(npc, obj, quality, quantity);
}

void Talk::deleteObject(int npc, int obj, int quality, int quantity) {
	_world.removeFromInventory(npc, obj, quality, quantity);
}

void Talk::giveObject(int obj, int quality, int from, int to) {
	if (_world.removeFromInventory(from, obj, quality, 1) > 0)
		_world.addToInventory(to, obj, quality, 1);
}

int Talk::whoHasObject(int obj, int quality) {
	const ObjList &a = _world.actors;
	for (int i = 0; i < a.partySize; ++i)
		if (hasObject(a.partyRoster[i], obj, quality, quality != 0))
			return a.partyRoster[i];
	return -1;
}

// conv_read_value case 0x9A: strength * 200 less ten times what is carried (sub_1DA30 / sub_1D998), or 0
int Talk::maxCarry(int npc) {
	if (npc < 0 || npc >= ObjList::ACTORS)
		return 0;
	int most = _world.actors.strength[npc] * 200, have = _world.carriedWeight(npc) * 10;
	return most > have ? most - have : 0;
}

// case 0x9B: quantity times the block 3 weight; ten times that unless the type is one of the nine reduced ones (gold and reagents)
int Talk::objectWeight(int obj, int quantity) {
	int w = _world.flags.block3[obj & (TileFlags::BLOCK3 - 1)] * quantity;
	bool reduced = obj == 0x58 || (obj >= 0x41 && obj <= 0x48);
	return reduced ? w : w * 10;
}

bool Talk::horsed(int npc) {
	int k = _world.actorObject(npc);
	return k >= 0 && _world.objects[k].type == 0x1AF; // a horse with rider (type 431)
}

void Talk::mount(int npc) {
	int k = _world.actorObject(npc);
	if (k >= 0) {
		_world.objects[k].type = 0x1AF;
		_world.objects[k].frame = 0;
	}
}

void Talk::addKarma(int n) {
	uint8_t &k = _world.actors.globals[G_KARMA];
	k = (uint8_t)std::min(99, k + n);
}

void Talk::subtractKarma(int n) {
	uint8_t &k = _world.actors.globals[G_KARMA];
	k = (uint8_t)std::max(0, k - n);
}

void Talk::sleep() {
	// the party sleeps until sunrise; the clock handling for it is part of the rest command
}

} // namespace Ultima6
} // namespace Ultima6

// Martian Dreams' time hazards (GAME.EXE sub_22B27 = the time step of the game, called with the minutes that pass): the thin air of Mars (the status bit 0x40 of the object, "gasps for air!",
// the tanks of oxygen 0x83 that last an hour for a charge), the cold of the nights (the clothes' warmth, ds:0CFA / ds:0D3E, against the hour: "is freezing!", "feels warmer!"), the lamps and
// candles in hand that burn down (ds:0D8A: 0x6E 0x70 0x72 0x74 0x76), the ice that melts, the radium that poisons whoever carries it outside a lead container (0x8B), the psychic powers that fade,
// the slow clearing of poison, paralysis and sleep, the street lamps that go on by night and the daylight (byte_4112B).
//
// APPROX(97): the objects of the four types of the table at cs:18A4 (sub_13EDC, sub_3F825, sub_228E0, sub_22AA7), the scenes of the ambient noise (sub_1F289 while the machines are dead),
// sub_17E5C (bit 4 of the object status), sub_22780, the party member that follows (word_4E6FA) and the redraws are left out; the original calls the routine with the minutes that passed, this
// is called every minute; "the view" (sub_270D2) is a square of 40 round the party; the damage of the cold and the poison is `Combat::applyDamage`, whose messages are Ultima VI's.

#include "ultima6/session.h"
#include "ultima6/stats_view.h"

#include "ultima6/describe.h"

#include <algorithm>
#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

namespace {
// ds:0CFA / ds:0D3E: the warmth of the things worn (sub_2020C)
struct Warmth {
	uint16_t type;
	uint8_t value;
};
const Warmth WARMTH[34] = {{1, 1},   {2, 2},   {3, 3},   {4, 3},   {5, 2},   {6, 1},   {7, 1},   {8, 2},    {9, 1},    {0xA, 1},  {0xB, 2},  {0xE, 4},
						   {0xD, 3}, {0x11, 2}, {0x12, 3}, {0x13, 3}, {0x14, 4}, {0x15, 4}, {0x16, 3}, {0x17, 5}, {0x5A, 12}, {0x19, 2}, {0x1A, 2}, {0x1B, 2},
						   {0x1C, 3}, {0x1D, 3}, {0x1E, 3}, {0x21, 1}, {0x22, 1}, {0x23, 1}, {0x24, 2}, {0x25, 3}, {0x26, 1}, {0x27, 3}};
} // namespace

// sub_2020C: the warmth of a thing worn (the last of the table that matches, as the original counts down)
int Game::mdWarmth(int type) const {
	for (int i = 33; i >= 0; --i)
		if (WARMTH[i].type == type)
			return WARMTH[i].value;
	return 0;
}

// sub_270D2 (0 = in view): the person is on the party's level within 20 squares of the party
bool Game::mdInView(int actor) const {
	int k = _world.actorObject(actor);
	if (k < 0)
		return false;
	const World::Obj &o = _world.objects[k];
	return mdInViewAt(o.x, o.y, o.z);
}

bool Game::mdInViewAt(int ox, int oy, int oz) const {
	int mask = z == 0 ? 0x3FF : 0xFF;
	auto gap = [mask](int d) {
		d &= mask;
		if (d > (mask >> 1))
			d -= mask + 1;
		return std::abs(d);
	};
	return oz == z && gap(ox - x) <= 20 && gap(oy - y) <= 20;
}

// sub_2BC7E: the first thing of the type among what the person carries, -1 if there is none
int Game::mdFindCarried(int actor, int type) const {
	std::vector<int> inv;
	_world.inventory(actor, inv, true);
	for (int i : inv)
		if (_world.objects[i].type == type)
			return i;
	return -1;
}

// What a person feels of the air (the bit 0x40 of the object's status): "gasps for air!" and "breathes easier!"
void Game::mdGasp(int actor, bool gasping) {
	int k = _world.actorObject(actor);
	if (k < 0)
		return;
	World::Obj &o = _world.objects[k];
	if (gasping && !(o.status & 0x40)) {
		o.status |= 0x40;
		message(mdMemberName(actor) + " gasps for air!\n");
	} else if (!gasping && (o.status & 0x40)) {
		o.status &= 0xBF;
		message(mdMemberName(actor) + " breathes easier!\n");
	}
}

// The light of the day (byte_4112B, 0-7, the radius in squares the party sees): 0 below ground or at night, rising through the evening and morning (from 6:40 to 7:20 and 17:20 to 18:00 by
// steps of ten minutes: 2 + minutes / 10), 7 by day; a lamp or candle in hand gives its tile's light (1-3) when the day gives less than 4.
int Game::mdLight() const {
	int di = gameClock.hour * 60 + gameClock.minute, light;
	if (z > 0 || di < 0x190 || di > 0x438)
		light = 0;
	else if (di < 0x1B8)
		light = (di - 0x190) / 10 + 2;
	else if (di > 0x410)
		light = (0x438 - di) / 10 + 2;
	else
		light = 7;
	if (light < 4) {
		int lamp = 0;
		const ObjList &al = _world.actors;
		for (int m = 0; m < al.partySize; ++m) {
			std::vector<int> inv;
			_world.inventory(al.partyRoster[m], inv, false);
			for (int i : inv) {
				const World::Obj &o = _world.objects[i];
				if ((o.status & 0x18) != 0x18 || (o.type != 0x6E && o.type != 0x70 && o.type != 0x72 && o.type != 0x74 && o.type != 0x76))
					continue;
				lamp = std::max<int>(lamp, _world.flags.flags2[(_world.baseTile.firstTile[o.type] + o.frame) & (TileFlags::TILES - 1)] & 3);
			}
		}
		if (lamp)
			light = lamp;
	}
	return light;
}

// sub_22382 (a thing hurts a person) and sub_22780 (what is on the square of a person): the first thing on the person's square that is not the person has its tile's flag 8 of the first
// set of flags (a hazard) unless a thing before it has the flag 4 of the third (a cover); the types do what the table at cs:068C says: 0xD1 hurts from half the person's hit points to half the
// most (fire, not the mechanical man), 0x17D 1-10 twice, 0xD7 (the sound 0x15) 1-20 twice, 0xA8 1-20 and 0x1CF a trap of the quality's kind (20 "It's hot here!" 1-20, 1 "Fire!", 2 "Frost!",
// 3 "Fire bomb!", 4 "Fragmentation bomb!", 5 "Steam jet!", 6 "Heat Ray!", 7 "Freeze Ray!", 8 "Acid!": 1 to the hit points less two; not at all when the body has fewer than 2). The creature that
// has the bits 8 and 10 of its flags is not hurt. APPROX(97): the bombs and rays are not drawn (sub_1F08A, sub_1E453), the kind of damage (fire, cold) is not kept, sub_17D85 is not read.
void Game::mdHazard(int actor) {
	ObjList &al = _world.actors;
	int k = _world.actorObject(actor);
	if (k < 0 || _world.objects[k].deleted || (al.status[actor] & 0x10) || !mdInViewAt(_world.objects[k].x, _world.objects[k].y, _world.objects[k].z))
		return;
	const World::Obj &me = _world.objects[k];
	const int cls = MovementClasses::flags(MovementClasses::classOf(me.type)); // the class flags (seg058:01C6): 0x100 and 0x400 together make a creature proof against the squares
	if ((cls & 0x100) && (cls & 0x400))
		return;
	std::vector<int> here;
	_world.objectsAt(me.x, me.y, me.z, here);
	for (int h : here) {
		const World::Obj &o = _world.objects[h];
		if (h == k || o.deleted)
			continue;
		int tile = (_world.baseTile.firstTile[o.type] + o.frame) & (TileFlags::TILES - 1);
		if (_world.flags.flags3[tile] & 4)
			return;
		if (!(_world.flags.flags1[tile] & 8))
			continue;
		int hp = al.hp[actor], most = ActorStats::maxHealth(_world, actor), di = 0;
		switch (o.type) {
		case 0xD1:
			di = actor == 6 ? 0 : seRoll(hp / 2, most / 2);
			break;
		case 0x17D:
			di = seRoll(1, 10) + seRoll(1, 10);
			break;
		case 0xD7:
			mdSfx(0x15);
			di = seRoll(1, 0x14) + seRoll(1, 0x14);
			break;
		case 0xA8:
			di = seRoll(1, 0x14);
			break;
		case 0x1CF: {
			int q = o.quality ? o.quality : seRoll(1, 8);
			if (q == 0x14) {
				message("It's hot here!\n");
				di = seRoll(1, 0x14);
				break;
			}
			if (al.hp[0] < 2)
				break;
			static const char *const NAMES[8] = {"Fire!", "Frost!", "Fire bomb!", "Fragmentation bomb!", "Steam jet!", "Heat Ray!", "Freeze Ray!", "Acid!"};
			static const int SOUNDS[8] = {9, 0x0A, 0x0B, 0x0B, 0x19, 0x0A, 0x0A, 0x1A};
			if (q >= 1 && q <= 8) {
				mdSfx(SOUNDS[q - 1]);
				message(std::string(NAMES[q - 1]) + "\n");
			}
			di = seRoll(1, std::max(1, hp - 2));
			break;
		}
		default:
			break;
		}
		if (di > 0)
			_combat.applyDamage(actor, actor, di);
		return;
	}
}

// sub_22B27(1): one minute of the hazards (the clock itself is the engine's)
void Game::mdMinute() {
	ObjList &al = _world.actors;
	mdThings();
	mdBarges();
	mdSegments();
	const bool airless = (al.talkFlags[0x10] & 0x20) != 0;
	auto bare = [&](const World::Obj &t) { // the name without its article
		std::string n = Describe::object(_world, t);
		for (const char *art : {"a ", "an ", "the "})
			if (n.compare(0, std::strlen(art), art) == 0)
				return n.substr(std::strlen(art));
		return n;
	};
	for (int s = 0; s < ObjList::ACTORS; ++s) {
		int k = _world.actorObject(s);
		if (k < 0 || _world.objects[k].deleted)
			continue;
		World::Obj &me = _world.objects[k];
		if (s <= 0xF) { // the psychic numbers stay within 10
			for (int w = 0; w < 3; ++w)
				if (mdPsychic(s, w) > 10)
					mdSetPsychic(s, w, 10);
		}
		if (!_mdDreaming && (al.status[s] & 0x80) && !(al.status[s] & 0x10)) {
			if (s == 0)
				continue;
			int warmth = 0;
			if (!(al.status[s] & 4) && mdInView(s))
				mdGasp(s, s != 6 && airless && mdFindCarried(s, 0x83) < 0);
			std::vector<int> inv;
			_world.inventory(s, inv, false);
			for (int i : inv) {
				World::Obj &o = _world.objects[i];
				if ((o.status & 0x18) != 0x18)
					continue;
				warmth += mdWarmth(o.type);
				bool fuel = o.type == 0x6E || o.type == 0x70 || o.type == 0x72 || o.type == 0x74 || o.type == 0x76;
				if (!fuel || seRoll(0, 1) == 0)
					continue;
				if (o.quality > 1) { // burns down a minute
					--o.quality;
				} else if (o.type == 0x74 || o.type == 0x76) {
					o.type = (uint16_t)(o.type - 1); // an oil lamp is out of fuel (0x73, 0x75)
					o.quality = 0;
					message("The " + bare(o) + " is out of fuel.\n");
				} else {
					message(bare(o) + " went out.\n");
					_world.deleteObject(i);
				}
			}
			// the cold: at night (before 4 or from 22 o'clock) 1 (warm enough: 2 with little warmth); in the twilight (to 6 and from 18) 1 only with little warmth
			int severity = 0, h = gameClock.hour;
			if (h <= 3 || h >= 0x16)
				severity = (warmth < 10 ? 1 : 0) + 1;
			else if (h <= 6 || h >= 0x12)
				severity = warmth < 10 ? 1 : 0;
			if (s == 6)
				severity = 0;
			if (z != 0 || _mdSleeping) // (byte_41BEE: nobody feels the cold while a member dreams until dawn)
				severity = 0;
			if (severity) {
				if (!(al.status[s] & 1)) {
					message(mdMemberName(s) + " is freezing!\n");
					al.status[s] |= 1;
				}
				if (seRoll(0, 1)) {
					message(mdMemberName(s) + " is freezing!\n");
					_combat.applyDamage(s, s, seRoll(1, 2));
					if (severity > 1)
						_combat.applyDamage(s, s, seRoll(1, 2));
				} else if (severity > 1) {
					message(mdMemberName(s) + " is freezing!\n");
					_combat.applyDamage(s, s, seRoll(1, 2));
				}
			} else if (al.status[s] & 1) {
				message(mdMemberName(s) + " feels warmer!\n");
				al.status[s] &= 0xFE;
			}
		}
		// what happens to everybody each minute
		mdHazard(s);
		if ((me.status & 0x80) && !_combatMode)
			me.status &= 0x7F;
		if ((al.status[s] & 8) && seRoll(0, 0x19) == 0)
			al.status[s] &= 0xF7; // the poison wears off
		if ((al.status[s] & 2) && (s == 6 || seRoll(0, 3) == 0)) { // paralysis ends for a roll of 1-30 within the strength
			int strength = (int)al.strength[s] - ((me.status & 0x40) ? 3 : 0) - (s <= 0xF && mdPsychic(s, 0) ? 3 : 0) + ((me.status & 0x80) ? 3 : 0);
			strength = std::max(1, std::min(30, strength));
			if (s == 6 || seRoll(1, 0x1E) <= strength)
				al.status[s] &= 0xFD;
		}
		if ((al.status[s] & 4) && al.worktype[s] != 0x91 && mdInView(s) && seRoll(0, 0x14) == 0) { // wakes
			al.status[s] &= 0xFB;
			me.type = al.baseObjFrame[s] & 0x3FF;
			me.frame = (uint8_t)(al.baseObjFrame[s] >> 10);
		}
		if ((al.status[s] & 8) && s != 6 && seRoll(0, 7) == 0)
			_combat.applyDamage(s, s, 1);
		if (s <= 7) { // the psychic powers fade now and then: one chance in four times the intelligence
			int intel = std::max(1, (int)al.intelligence[s] - (((me.status & 0x40) || ((me.status & 0x80) && s != 1)) ? 3 : 0));
			for (int w = 0; w < 3; ++w) {
				if (mdPsychic(s, w) == 0 || seRoll(1, intel * 4) != 1)
					continue;
				mdSetPsychic(s, w, mdPsychic(s, w) - 1);
				if (mdPsychic(s, w) == 0 && (al.status[s] & 0x80) && ((s == 0) == _mdDreaming)) {
					mdSfx(0x32);
					message("A psychic power fades...\n");
				}
			}
		}
		// the ice melts: the first stage a tenth of the time, the pieces (0x100) too (only the party's packs are searched: the others are not looked at)
		if (!(al.status[s] & 0x80))
			continue;
		std::vector<int> inv;
		_world.inventory(s, inv, true);
		for (int i : inv) {
			World::Obj &o = _world.objects[i];
			if (o.type == 0xA0 && o.frame > 1 && seRoll(0, 99) < 10) {
				message("Some ice has melted.\n");
				o.frame = 1;
			} else if (o.type == 0x100 && seRoll(0, 99) < 10) {
				message("Some ice has melted.\n");
				_world.deleteObject(i);
			}
		}
		// radium (0x1C1, 0x1C0) poisons whoever has it outside a lead container (0x8B)
		if (s != 6) {
			for (int radium : {0x1C1, 0x1C0}) {
				if (radium == 0x1C1 && (al.status[s] & 8))
					continue;
				for (int i : inv) {
					if (_world.objects[i].deleted || _world.objects[i].type != radium)
						continue;
					bool shielded = false;
					int cur = i;
					while (_world.objects[cur].location() == ObjRecord::IN_CONTAINER) {
						int up = _world.objects[cur].parent;
						if (up < 0)
							break;
						if (_world.objects[up].type == 0x8B) {
							shielded = true;
							break;
						}
						cur = up;
					}
					if (!shielded) {
						message("`" + mdMemberName(s) + " is poisoned!\n");
						al.status[s] |= 8;
					}
					break;
				}
			}
		}
	}
}

// The barges of the canals (sub_13EDC, sub_13F41, sub_140F9, sub_141C7, sub_14378, sub_1449B), run once a minute after the things in view: once the story has come to the barge (flags 0x76 bits 0x20
// and 0x40; or in a dream) the lever (0x1A2, a frame below 4 saying north, east, south or west; 4 and more is "center") of each barge in view (16 at most) pulls its barge one square that way: the barge
// is the lever, whose quantity is the number of its pieces with itself, and the pieces (0x1A1) of the same number (quality) within 6 squares in view. It stops (the lever goes to "center") when a piece
// would leave the water (terrain 0x10-0x17, 0xC0-0xC3) or meet anything but a piece of itself; it waits when some but not all of the party are aboard (not in solo mode); with 412 things aboard "The barge
// is riding low in the water.", with 512 "The barge is scraping the bottom of the canal." and it does not move. Everything on the squares of the pieces (what is not hung, a thing of type below 0x1CD)
// goes along, and the view follows the leader.
void Game::mdBarges() {
	ObjList &al = _world.actors;
	if (!(((al.talkFlags[0x76] & 0x60) == 0x60) || _mdDreaming))
		return;
	std::vector<int> helms;
	for (size_t i = 0x101; i < _world.objects.size() && helms.size() < 16; ++i) {
		const World::Obj &o = _world.objects[i];
		if (!o.deleted && o.onMap() && o.type == 0x1A2 && o.frame < 4 && mdInViewAt(o.x, o.y, o.z))
			helms.push_back((int)i);
	}
	static const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};
	bool leaderMoved = false;
	for (size_t h = helms.size(); h-- > 0;) {
		int hk = helms[h];
		int number = _world.objects[hk].quality, hz = _world.objects[hk].z;
		std::vector<int> parts{hk};
		mdEach(_world.objects[hk].x - 6, _world.objects[hk].y - 6, _world.objects[hk].x + 6, _world.objects[hk].y + 6, [&](int i) {
			const World::Obj &o = _world.objects[i];
			if (o.type == 0x1A1 && o.quality == number && mdInViewAt(o.x, o.y, o.z) && parts.size() < 0x20)
				parts.push_back(i);
			return false;
		}, hz);
		int expected = _world.objects[hk].quantity;
		if (expected != (int)parts.size()) {
			if (expected < (int)parts.size())
				message("Found " + std::to_string(parts.size()) + " barge pieces when expecting " + std::to_string(expected) + "!\n");
			continue;
		}
		int dx = DX[_world.objects[hk].frame], dy = DY[_world.objects[hk].frame];
		auto stop = [&]() { _world.objects[hk].frame += 4; };
		// sub_141C7: may the barge go?
		int aboard = 0;
		bool go = true;
		for (int p : parts) {
			std::vector<int> here;
			_world.objectsAt(_world.objects[p].x, _world.objects[p].y, hz, here);
			for (int q : here) {
				int a = _world.objects[q].actor;
				if (!_world.objects[q].deleted && a >= 0 && a < 0x100 && (al.status[a] & 0x80))
					++aboard;
			}
		}
		for (int p : parts) {
			int tx = (_world.objects[p].x + dx) & 0x3FF, ty = (_world.objects[p].y + dy) & 0x3FF;
			if (!mdInViewAt(tx, ty, hz)) {
				go = false;
				break;
			}
			int tile = _world.map.tile(hz, tx & (hz == 0 ? 0x3FF : 0xFF), ty & (hz == 0 ? 0x3FF : 0xFF)) & 0xFF;
			if (!((tile >= 0x10 && tile <= 0x17) || (tile >= 0xC0 && tile <= 0xC3))) {
				stop();
				go = false;
				break;
			}
			bool own = false, other = false, blocked = false;
			std::vector<int> there;
			_world.objectsAt(tx, ty, hz, there);
			for (int q : there) {
				const World::Obj &o = _world.objects[q];
				if (o.deleted)
					continue;
				if (o.type == 0x1A1 || o.type == 0x1A2) {
					if (o.quality == number)
						own = true;
					else
						blocked = true;
				} else if (o.type < 0x1CD) {
					other = true;
				}
			}
			if (blocked || (other && !own)) {
				stop();
				go = false;
				break;
			}
		}
		if (go && _solo < 0 && aboard > 0 && al.partySize > aboard)
			go = false;
		if (!go)
			continue;
		// sub_14378: the load
		std::vector<int> cargo;
		for (int p : parts) {
			std::vector<int> here;
			_world.objectsAt(_world.objects[p].x, _world.objects[p].y, hz, here);
			for (int q : here) {
				const World::Obj &o = _world.objects[q];
				int tile = (_world.baseTile.firstTile[o.type] + o.frame) & (TileFlags::TILES - 1);
				if (!o.deleted && cargo.size() < 0x200 && !(_world.flags.flags3[tile] & 4) && !(_world.flags.flags2[tile] & 0x10) && o.type < 0x1CD)
					cargo.push_back(q);
			}
		}
		if (cargo.size() >= 0x19C) {
			if (cargo.size() >= 0x200) {
				message("The barge is scraping the bottom of the canal.\n");
				continue;
			}
			message("The barge is riding low in the water.\n");
		}
		// sub_1449B: the pieces, then what is aboard, go one square
		for (int p : parts)
			_world.moveObject(p, (_world.objects[p].x + dx) & 0x3FF, (_world.objects[p].y + dy) & 0x3FF, hz);
		for (size_t c = cargo.size(); c-- > 0;) {
			int q = cargo[c];
			if (_world.objects[q].type == 0x1A1 || _world.objects[q].type == 0x1A2) {
				message("The barge shudders!\n");
				mdSfx(0xC);
			} else {
				_world.moveObject(q, (_world.objects[q].x + dx) & 0x3FF, (_world.objects[q].y + dy) & 0x3FF, hz);
			}
			if (q == _leaderObject)
				leaderMoved = true;
		}
	}
	if (leaderMoved && _leaderObject >= 0) {
		x = _world.objects[_leaderObject].x;
		y = _world.objects[_leaderObject].y;
	}
}

// sub_22B27's loop over the things in view (the types of the table at cs:18A4): a seed mound (0xFB with quality 0xF and a frame below 3) grows: the seed (0xFC) in the hole (0x101) under it
// keeps the day (quality) and month (quantity) it was planted; four days on (the month's 28 days; or when the month has come round) the mound takes its next frame, the seed's stamp is made
// anew, and at frame 3 the mound's quality is 0x10 (ripe); in a dream it grows by a roll of 1-2 on the stamp's quantity, a step above 12. A flame (0x9D) counts its quality down, takes the
// next lower frame with a quality of 10-15 when it is out and is gone at frame 0. The jets (0x1D0) are in mdJets.
void Game::mdThings() {
	for (size_t i = 0x101; i < _world.objects.size(); ++i) {
		World::Obj &o = _world.objects[i];
		if (o.deleted || !o.onMap() || (o.type != 0xFB && o.type != 0x9D && o.type != 0x1D0) || !mdInViewAt(o.x, o.y, o.z))
			continue;
		if (o.type == 0x1D0) {
			mdJets((int)i);
			continue;
		}
		if (o.type == 0x9D) {
			if (o.quality > 1) {
				--o.quality;
			} else if (o.frame != 0) {
				--o.frame;
				o.quality = (uint8_t)seRoll(10, 15);
			} else {
				_world.deleteObject((int)i);
			}
			continue;
		}
		if (o.quality != 0x0F || o.frame >= 3)
			continue;
		std::vector<int> here;
		_world.objectsAt(o.x, o.y, o.z, here);
		int hole = -1;
		for (int h : here)
			if (_world.objects[h].type == 0x101 && !_world.objects[h].deleted)
				hole = h;
		int seed = -1;
		for (size_t c = 0; c < _world.objects.size() && hole >= 0; ++c)
			if (!_world.objects[c].deleted && _world.objects[c].location() == ObjRecord::IN_CONTAINER && _world.objects[c].parent == hole && _world.objects[c].type == 0xFC)
				seed = (int)c;
		if (seed < 0)
			continue;
		World::Obj &sd = _world.objects[seed];
		int day = sd.quality, month = sd.quantity;
		auto grow = [&]() {
			++o.frame;
			if (o.frame >= 3)
				o.quality = 0x10;
		};
		if (_mdDreaming) {
			month += seRoll(1, 2);
			if (month > 0x0C) {
				grow();
				sd.quantity = 0;
			} else {
				sd.quantity = (uint8_t)month;
			}
			continue;
		}
		if (month == 0 || day == 0)
			continue;
		int now = gameClock.month * 28 + gameClock.day, planted = month * 28 + day + 3;
		if (now > planted || gameClock.month < month) {
			grow();
			sd.quality = (uint8_t)gameClock.day;
			sd.quantity = (uint8_t)gameClock.month;
		}
	}
}

// sub_55BA0 (trigger_jet): a steam jet (0x1D0) puts out a jet of steam as long as the high half of its quality, in the direction of the bits 5-7 of its quantity (the eight directions, north first,
// clockwise), and whoever stands in it is hurt for 10-50. The low half of the quality is the period: a jet with a period and a counter in the low half of the quantity "Nothing comes out.";
// else the counter is set to the period. APPROX(97): the things the jet hurts besides people (the types 0xB3, 0x184, 0x133, 0x12C, 0xD9 of sub_2043E) are not hurt, the animation is not drawn
void Game::mdJet(int jet) {
	World::Obj &o = _world.objects[jet];
	if (o.type != 0x1D0)
		return;
	int length = o.quality >> 4, period = o.quality & 0xF, counter = o.quantity & 0xF, dir = (o.quantity & 0xE0) >> 5;
	if (period > 0 && counter > 0) {
		message("Nothing comes out.\n");
		return;
	}
	if (period > 0)
		o.quantity = (uint8_t)((dir << 5) | period);
	static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
	int px = o.x, py = o.y, mask = o.z == 0 ? 0x3FF : 0xFF;
	mdSfx(0x19);
	for (int i = 0; i < length; ++i) {
		px = (px + DX[dir]) & mask;
		py = (py + DY[dir]) & mask;
		std::vector<int> here;
		_world.objectsAt(px, py, o.z, here);
		for (int h : here) {
			int a = _world.objects[h].actor;
			if (a >= 0 && !_world.objects[h].deleted && _world.actors.hp[a] > 0)
				_combat.applyDamage(a, a, seRoll(10, 50));
		}
		for (int h : here) { // the things that can be struck (sub_2043E) take 10-50 as well
			const World::Obj &b = _world.objects[h];
			if (!b.deleted && b.actor < 0 && b.quantity != 0 && (b.type == 0xB3 || b.type == 0x184 || b.type == 0x133 || b.type == 0x12C || b.type == 0xD9))
				mdHitThing(0, h, 0, seRoll(10, 50));
		}
	}
}

// sub_55D62: the jets (0x1D0) fire while the power underground is on: a jet with a period does so when its quantity is 0 (else the counter is cleared), one without a period every minute
// it has a quantity
void Game::mdJets(int jet) {
	World::Obj &o = _world.objects[jet];
	if (!(_world.actors.talkFlags[0x73] & 4) || o.type != 0x1D0)
		return;
	int period = o.quality & 0xF;
	int v = o.quantity != 0 ? 1 : 0;
	if (period) {
		if (v)
			o.quantity = (uint8_t)(o.quantity & 0xF0);
		else
			mdJet(jet);
	} else if (v) {
		mdJet(jet);
	}
}

// sub_18286: how hard a person is to hit: the dexterity, 3 less if he gasps (not below 1), 3 more with the bit 0x80 of his object status (not above 30), 1 if he sleeps
int Game::mdDefence(int actor) const {
	int k = _world.actorObject(actor);
	int d = _world.actors.dexterity[actor];
	int st = k >= 0 ? _world.objects[k].status : 0;
	if (st & 0x40)
		d = std::max(1, d - 3);
	if (st & 0x80)
		d = std::min(0x1E, d + 3);
	if (_world.actors.status[actor] & 4)
		d = 1;
	return d;
}

// sub_18A64 (the ray guns 0x2B, 0x2D, 0xF0, 0xF1; the effect numbers at ds:0705 ff.): the shot goes along the line to the target and whoever is on it is hit: a roll of 1-60 (1-45 for the
// freeze ray 0xF1) above his defence hits (else "%s dodges!" and the sound 3), a hit does the gun's damage (20, 20, 25, 10); the freeze ray freezes water (a 0xA0 of frame 1 goes to frame 3,
// "The water freezes.") and paralyses (the bit 2 of the status, "`%s paralyzed!") the kinds of creature that the bits 7 and 14 of the flags name ("It has no effect." on the others).
// APPROX(97): the flight is not drawn; sub_19088 is not read; the charges are not used.
void Game::mdRayShot(int attacker, int weapon, int tx, int ty) {
	static const int GUNS[4] = {0x2B, 0x2D, 0xF0, 0xF1}, DAMAGE[4] = {20, 20, 25, 10}, SOUND[4] = {8, 8, 10, 10};
	int idx = 0;
	for (int i = 0; i < 4; ++i)
		if (GUNS[i] == weapon)
			idx = i;
	int ka = _world.actorObject(attacker);
	if (ka < 0)
		return;
	mdSfx(SOUND[idx]);
	std::vector<std::pair<int, int>> path;
	_world.boltPath(z, _world.objects[ka].x, _world.objects[ka].y, tx, ty, 8, path);
	if (path.empty() || path.back() != std::make_pair(tx, ty))
		path.push_back(std::make_pair(tx, ty)); // (the target's own body blocks the line: the shot ends in it)
	for (const auto &sq : path) {
		std::vector<int> here;
		_world.objectsAt(sq.first, sq.second, z, here);
		for (int h : here) {
			World::Obj &o = _world.objects[h];
			if (o.deleted)
				continue;
			if (weapon == 0xF1 && o.actor < 0 && o.type == 0xA0 && o.frame == 1) {
				o.frame = 3;
				message("The water freezes.\n");
				continue;
			}
			int a = o.actor;
			if (a < 0 || a == attacker || _world.actors.hp[a] == 0)
				continue;
			if (seRoll(1, weapon == 0xF1 ? 0x2D : 0x3C) <= mdDefence(a)) {
				message(mdMemberName(a) + " dodges!\n");
				mdSfx(3);
				continue;
			}
			if (weapon == 0xF1) {
				const CreatureStats *cs = Combat::stats(o.type);
				const int cls = MovementClasses::flags(MovementClasses::classOf(o.type)); // the class flags (seg058:01C6)
				if (!cs || !(cls & ((1 << 14) | (1 << 7)))) {
					message("It has no effect.\n");
					continue;
				}
				_world.actors.status[a] |= 2;
				message("`" + mdMemberName(a) + " paralyzed!\n");
				if ((cls & (1 << 7)) && !(cls & (1 << 14)))
					mdStrike(attacker, a, DAMAGE[idx]);
				continue;
			}
			mdStrike(attacker, a, DAMAGE[idx]);
		}
	}
}

// sub_29CE2, the worktypes of Martian Dreams that have a routine of their own: 0x13 (sub_1C622) a thing that drifts (5 moves; in the maze dream 0x44 the creature 0x18E does it only half the turns)
// one step in a random direction and hurts whatever shares its square for 1-10 twice; 0x15 (sub_1D819, the child): walks up to the nearest member and hits one next to it for 6; 0x1A
// (sub_2AF67) turns the person into a plain thing of the same looks (its hit points the quantity, its quality kept) where it stands. APPROX(97): the others (0xD Stoker, 0x11, 0x12, 0x14,
// 0x16 - 0x19: sub_1D230, sub_1D7E6, sub_1C91A, sub_1CDC2, sub_1C710, sub_1D89E, sub_50FEC, sub_1CB44) act as the engine's plain fighters do.
// sub_1A45C, the part that matters: the damage of a blow (1 and 255 are as they are, else rolled from 1 to the damage) is cut by a roll from 1 to the armour worn (sub_189D2); nothing left
// and the victim is "grazed" (it wakes: sub_18F7D); else the hurt is the engine's `Combat::applyDamage`
bool Game::mdStrike(int attacker, int victim, int damage) {
	if (damage > 1 && damage < 0xFF)
		damage = seRoll(1, damage);
	if (victim >= 0 && victim < 0x100 && damage > 0 && damage < 0xFF) {
		int ac = _combat.armourClass(victim);
		if (ac > 0)
			damage -= seRoll(1, ac);
	}
	if (damage <= 0) {
		_world.actors.status[victim] &= 0xFD;
		message("`" + mdMemberName(victim) + " grazed.\n");
		return false;
	}
	return _combat.applyDamage(attacker, victim, damage);
}

bool Game::mdWorktype(int actor) {
	ObjList &al = _world.actors;
	int k = _world.actorObject(actor);
	if (k < 0 || _world.objects[k].deleted || actor >= 0x100)
		return false;
	int wt = al.worktype[actor];
	if ((wt < 0x99 || wt > 0x9E) && wt != 8 && wt != 0xA0 && wt != 4 && wt != 6 && wt != 7 && wt != 9 && wt != 0x1B && wt != 0x9F && wt != 0x0A && wt != 0x0B && wt != 0x0D && wt != 0x0E && wt != 0x18 && wt != 0x11 && wt != 0x12 && wt != 0x13 && wt != 0x14 && wt != 0x15 && wt != 0x16 && wt != 0x17 && wt != 0x19 && wt != 0x1A)
		return false;
	_world.objects.reserve(_world.objects.size() + 128); // the things made below must not move the object that `o` stands for
	World::Obj &o = _world.objects[k];
	// sub_1B69D: whom the creature goes for (mdTarget), with its distance; -1 if there is none
	auto nearest = [&](int &dist) {
		int best = mdTarget(actor);
		dist = 99;
		if (best >= 0) {
			int bk = _world.actorObject(best);
			dist = std::max(std::abs(_world.objects[bk].x - o.x), std::abs(_world.objects[bk].y - o.y));
		}
		return best;
	};
	auto remove = [&]() { // sub_2C902: the creature is taken out of the world
		al.status[actor] |= 0x10;
		_world.deleteObject(k);
	};
	auto facing = [&]() { o.frame = seRoll(0, 1) ? 3 : 7; }; // the random look of a 0x175, 0x16A or 0x182 (the bits above the type in its word)
	// sub_1B936, the fighter: it goes for whom `mdTarget` names; in reach of its weapon (the engine's choice of the best) it strikes (10 moves) when next to the target or when there is a line, else it
	// steps to one side (by chance to the square level with the target on either axis); out of reach it walks up; with no target it wanders (the engine's: false)
	auto fight = [&]() -> bool {
		int d, t = nearest(d);
		if (t < 0)
			return false;
		int tk = _world.actorObject(t), tx = _world.objects[tk].x, ty = _world.objects[tk].y;
		int weapon = _combat.weaponType(actor), range = Combat::rangeOf(weapon);
		if (Combat::distance(std::abs(tx - o.x), std::abs(ty - o.y)) <= range && d < 8) {
			if (d <= 1 || _creatures.lineOfFire(actor, tx, ty, range)) {
				_combat.attack(actor, t);
				_world.spendMoves(actor, 10);
				return true;
			}
			if (seRoll(0, 1))
				_creatures.walkTo(actor, o.x, ty);
			else
				_creatures.walkTo(actor, tx, o.y);
			return true;
		}
		_creatures.walkTo(actor, tx, ty);
		return true;
	};
	if (wt == 8 || wt == 0xA0)
		return fight();
	// sub_26441 and sub_26469: the square distance on the wrapped map
	auto gapOf = [&](int a, int b) {
		int d = (a - b) & 0x3FF;
		return d > 0x200 ? d - 0x400 : d;
	};
	auto memberNear = [&](int r) { // some member of the party with both distances below r
		for (int m = 0; m < al.partySize; ++m) {
			int mk = _world.actorObject(al.partyRoster[m]);
			if (mk >= 0 && std::abs(gapOf(_world.objects[mk].x, o.x)) < r && std::abs(gapOf(_world.objects[mk].y, o.y)) < r)
				return true;
		}
		return false;
	};
	// the aimless walk of sub_1C1E8 and sub_1C322: one time in two a step towards a square ten along one axis (x or y at random: x is 10, or 1014 when x + a roll of 0-1 is 0, the other
	// coordinate is the creature's own; the original's sum is taken as it is: the goal is an absolute square, not ten squares off)
	auto drift = [&]() {
		int gx = o.x, gy = o.y;
		if (seRoll(0, 1)) {
			gx = (((o.x + seRoll(0, 1)) == 0) ? -10 : 10) & 0x3FF;
		} else {
			gy = (((o.y + seRoll(0, 1)) == 0) ? -10 : 10) & 0x3FF;
		}
		_creatures.walkTo(actor, gx, gy);
	};
	if (wt == 7 || wt == 9 || wt == 0x1B || wt == 0x9F) { // sub_1C062, the one that runs away: more than seven squares from the party it does nothing (its moves are gone); with a target in view it makes
		// for the square as far from it as it is near (the walk costs nothing when it fails), without (and not of side 0x40) it wanders, of side 0x40 it comes to the party; one turn in four, if it
		// is not of the party, it gets a hit point back and, once it has a quarter of its strength, its own worktype again
		if (std::max(std::abs(gapOf(o.x, x)), std::abs(gapOf(o.y, y))) > 7) {
			_world.spendMoves(actor, al.moves[actor] ? al.moves[actor] : 1);
			return true;
		}
		int t = mdTarget(actor);
		if (t < 0)
			return false;
		int tk = _world.actorObject(t), dx = gapOf(_world.objects[tk].x, o.x), dy = gapOf(_world.objects[tk].y, o.y);
		if (std::abs(dx) < 8 && std::abs(dy) < 8) {
			int saved = al.moves[actor];
			if (!_creatures.walkTo(actor, (o.x - dx) & 0x3FF, (o.y - dy) & 0x3FF))
				al.moves[actor] = (uint8_t)saved;
		} else if ((al.status[actor] & 0x60) == 0x40) {
			_creatures.walkTo(actor, x, y);
		} else {
			return false;
		}
		if (!(al.status[actor] & 0x80) && seRoll(0, 3) == 0) {
			int most = ActorStats::maxHealth(_world, actor);
			if (al.hp[actor] < most)
				++al.hp[actor];
			if (most > 0 && al.hp[actor] * 4 / most > 0 && al.unknown12f1[actor])
				al.worktype[actor] = al.unknown12f1[actor];
		}
		return true;
	}
	if (wt == 0x0A) { // sub_1C1E8: with a member within three squares, one time in two it comes to the party; else it drifts or rests
		if (memberNear(3) && seRoll(0, 1) == 0) {
			_creatures.walkTo(actor, x, y);
			return true;
		}
		if (seRoll(0, 1))
			drift();
		else
			_world.spendMoves(actor, 5);
		return true;
	}
	if (wt == 0x0B) { // sub_1C322: with a member within three squares, one time in six it turns on the party (worktype 8, side 0x20; a 0x17A becomes the 0x17C with 5 more strength, 5 less dexterity,
		// moves and 7 less intelligence and 20 more hit points) and fights; else it drifts or rests
		if (memberNear(3) && seRoll(0, 5) == 0) {
			al.worktype[actor] = 8;
			al.status[actor] |= 0x20;
			if (o.type == 0x17A) {
				o.type = 0x17C;
				al.strength[actor] = (uint8_t)(al.strength[actor] + 5);
				al.dexterity[actor] = (uint8_t)(al.dexterity[actor] - 5);
				al.moves[actor] = (uint8_t)(al.moves[actor] - 5);
				al.intelligence[actor] = (uint8_t)(al.intelligence[actor] - 7);
				al.hp[actor] = (uint8_t)(al.hp[actor] + 0x14);
			}
			return fight();
		}
		if (seRoll(0, 1))
			drift();
		else
			_world.spendMoves(actor, 5);
		return true;
	}
	if (wt == 4) { // sub_1BB87, the ranged fighter: with a target within two squares it runs, the original's goal being the small square (x - his x, y - his y) of the map (a slip: it heads for the
		// north-west corner); without a ranged weapon (readied, range 3 or more: sub_1BA72 takes the one that reaches the target, else the one that hurts most) "has no ranged weapons!"; more than five
		// squares from the party it comes to it; else it shoots when in reach with a line, goes to one side when there is none and walks up when out of reach; without a target it wanders (the engine's)
		int d, t = nearest(d);
		int tx = 0, ty = 0;
		if (t >= 0) {
			int tk = _world.actorObject(t);
			tx = _world.objects[tk].x;
			ty = _world.objects[tk].y;
		}
		int best = -1, bestRange = 0, bestDamage = 0;
		std::vector<int> inv;
		_world.inventory(actor, inv, false);
		for (int i : inv) {
			const World::Obj &it = _world.objects[i];
			int range = Combat::rangeOf(it.type), dmg = std::max(0, Combat::weaponDamageOf(it.type));
			if (range < 3 || (it.status & 0x18) != 0x18)
				continue;
			if (bestRange < d) {
				if (range > bestRange) {
					bestRange = range;
					bestDamage = dmg;
					best = i;
				}
			} else if (dmg > bestDamage) {
				bestRange = range;
				bestDamage = dmg;
				best = i;
			}
		}
		if (t >= 0 && std::abs(gapOf(tx, o.x)) < 3 && std::abs(gapOf(ty, o.y)) < 3) {
			_creatures.walkTo(actor, (o.x - tx) & 0x3FF, (o.y - ty) & 0x3FF);
			return true;
		}
		if (best < 0) {
			message("\n" + _combat.nameOf(actor) + " has no ranged weapons!\n");
			_world.spendMoves(actor, 5);
			return true;
		}
		if (std::abs(gapOf(o.x, x)) > 5 || std::abs(gapOf(o.y, y)) > 4) {
			_creatures.walkTo(actor, x, y);
			return true;
		}
		if (t < 0)
			return false;
		if (d <= bestRange) {
			if (d <= 1 || _creatures.lineOfFire(actor, tx, ty, bestRange)) {
				_combat.attackWith(actor, t, _world.objects[best].type, false);
				_world.spendMoves(actor, 10);
				return true;
			}
			if (seRoll(0, 1))
				_creatures.walkTo(actor, o.x, ty);
			else
				_creatures.walkTo(actor, tx, o.y);
			return true;
		}
		_creatures.walkTo(actor, tx, ty);
		return true;
	}
	if (wt == 6) { // sub_1BDB5, the berserk fighter: it goes for the strongest within eight squares (mdTargetStrongest); in reach it strikes (10 moves) when next to it or with a line; otherwise it
		// walks up (or to one side when there is no line) and, when the step fails, fights as sub_1B936; 5 moves more
		int t = mdTargetStrongest(actor);
		if (t < 0) {
			if (!fight())
				return false;
			_world.spendMoves(actor, 5);
			return true;
		}
		int tk = _world.actorObject(t), tx = _world.objects[tk].x, ty = _world.objects[tk].y;
		int weapon = _combat.weaponType(actor), range = Combat::rangeOf(weapon);
		int d = std::max(std::abs(tx - o.x), std::abs(ty - o.y));
		int gx = tx, gy = ty;
		if (d < 8 && Combat::distance(std::abs(tx - o.x), std::abs(ty - o.y)) <= range) {
			if (d <= 1 || _creatures.lineOfFire(actor, tx, ty, range)) {
				_combat.attack(actor, t);
				_world.spendMoves(actor, 10);
				return true;
			}
			if (seRoll(0, 1))
				gx = o.x, gy = ty;
			else
				gx = tx, gy = o.y;
		}
		int saved = al.moves[actor];
		if (!_creatures.walkTo(actor, gx, gy)) {
			al.moves[actor] = (uint8_t)saved;
			fight();
		}
		_world.spendMoves(actor, 5);
		return true;
	}
	if (wt == 0x0E) { // sub_1C4E8, the part that matters: a 0x175 or 0x182 more than seven squares from the nearest member burrows (invisible, worktype 0x12); the fight itself is the engine's
		if (o.type != 0x175 && o.type != 0x182 && o.type != 0x16A)
			return false;
		facing();
		int d;
		if (nearest(d) >= 0 && d >= 8 && o.type != 0x16A) {
			o.status |= 2;
			al.worktype[actor] = 0x12;
			al.unknown12f1[actor] = 0x12;
			_world.spendMoves(actor, 5);
			return true;
		}
		return false;
	}
	if (wt == 0x12) { // sub_1C91A (the thing under the sand, 0x175 and others): it waits (2 moves) until a member of the party stands next to it; then "Suddenly, something bursts up through the sand!",
		// it comes up (the invisible bit goes) as a stationary fighter (worktype 0xE) and, if it is a 0x175, ten to thirty 0x16A (worktype 0x19, their quality the number of their parent)
		// rise at random places within six squares
		int d;
		if (nearest(d) < 0 || d > 1) {
			_world.spendMoves(actor, 2);
			return true;
		}
		message("Suddenly, something bursts up through the sand!\n");
		al.worktype[actor] = 0x0E;
		al.unknown12f1[actor] = 0x0E;
		o.status &= 0xFD;
		if (o.type == 0x175) {
			int n = seRoll(0, 10) + seRoll(0, 10) + 10;
			for (int i = 0; i < n; ++i)
				for (int t = 0; t < 8; ++t) {
					int nx = o.x + seRoll(0, 12) - 6, ny = o.y + seRoll(0, 12) - 6;
					// APPROX(97): the square is any free one for a creature (the original also lets one stand on another 0x16A)
					int c = _creatures.spawn(0x16A, 0x19, nx, ny, o.z, true);
					if (c >= 0) {
						int ck = _world.actorObject(c);
						if (ck >= 0) {
							_world.objects[ck].frame = 3;
							_world.objects[ck].quality = (uint8_t)actor;
						}
						break;
					}
				}
		}
		return false;
	}
	if (wt == 0x19) { // sub_1CB44 (the worm that burrows, 0x16A, shown as 0xFE under the sand): in reach it comes up and strikes; out of reach, unhurt, it burrows now and then; a wounded one
		// (under a quarter of its strength) always, and goes; under the sand it creeps to the member and surfaces by chance, and when it is more than eight squares from its parent it is gone
		int d, m = nearest(d);
		if (m < 0)
			return false;
		int mk = _world.actorObject(m);
		const CreatureStats *cs = Combat::stats(0x16A);
		int range = Combat::rangeOf(_combat.weaponType(actor));
		bool burrowed = (o.status & 2) != 0;
		auto surface = [&]() {
			o.status &= 0xFD;
			o.type = 0x16A;
			facing();
		};
		if (d <= range) {
			if (burrowed)
				surface();
			if (d > 1 && !_creatures.lineOfFire(actor, _world.objects[mk].x, _world.objects[mk].y, range)) {
				_world.spendMoves(actor, 5);
				return true;
			}
			_combat.attack(actor, m);
			_world.spendMoves(actor, 10);
			return true;
		}
		if (!burrowed) {
			_world.spendMoves(actor, 5);
			bool wounded = al.hp[actor] < (cs ? cs->hp : 30) / 4;
			if (!wounded && seRoll(1, 100) > 50)
				return true;
			o.type = 0xFE;
			o.frame = 0;
			if (wounded)
				remove();
			else
				o.status |= 2;
			return true;
		}
		int parent = o.quality;
		if (parent != 0) {
			int pk = _world.actorObject(parent);
			if (pk >= 0 && std::max(std::abs(_world.objects[pk].x - o.x), std::abs(_world.objects[pk].y - o.y)) > 8) {
				remove();
				return true;
			}
		}
		_creatures.walkTo(actor, _world.objects[mk].x, _world.objects[mk].y);
		if (seRoll(0, d * 2) <= 1)
			surface();
		return true;
	}
	if (wt == 0x14) { // sub_1CDC2 (the thief of oxium): it goes to the nearest member; next to a party member that carries oxium (0x83) it takes the stack (what it keeps is a random part of it): "Oxium stolen!",
		// 10 moves (afterwards, if a number from 0 to 50 is not above that part, it is quick: worktype 0x1B); a member without goes unrobbed: another next to it with oxium is robbed, else the first is struck
		int d, m = nearest(d);
		if (m < 0) {
			_world.spendMoves(actor, 5);
			return true;
		}
		int mk = _world.actorObject(m);
		if (d > 1) {
			_creatures.walkTo(actor, _world.objects[mk].x, _world.objects[mk].y);
			_world.spendMoves(actor, 5);
			return true;
		}
		int victim = m;
		if (mdFindCarried(m, 0x83) < 0) {
			for (int i = 0; i < al.partySize; ++i) {
				int a = al.partyRoster[i], ak = _world.actorObject(a);
				if (a == m || a == actor || ak < 0 || (al.status[a] & 0x10))
					continue;
				if (std::max(std::abs(_world.objects[ak].x - o.x), std::abs(_world.objects[ak].y - o.y)) <= 1 && mdFindCarried(a, 0x83) >= 0) {
					victim = a;
					break;
				}
			}
		}
		int item = mdFindCarried(victim, 0x83);
		if (item < 0) {
			_combat.attack(actor, m);
			_world.spendMoves(actor, 10);
			return true;
		}
		int had = _world.quantityOf(_world.objects[item]);
		_world.removeFromInventory(victim, 0x83, 0, had);
		int kept = seRoll(0, had);
		if (kept > 0)
			_world.addToInventory(actor, 0x83, 0, kept);
		message("Oxium stolen!\n");
		_world.spendMoves(actor, 10);
		if (seRoll(0, 0x32) <= kept) {
			al.worktype[actor] = 0x1B;
			al.unknown12f1[actor] = 0x1B;
		}
		return true;
	}
	if (wt == 0x11) { // sub_1D7E6: a 0x172 does nothing at all (its moves are spent); the others are the worm of sub_1CFF6
		if (o.type != 0x172)
			return false;
		_world.spendMoves(actor, al.moves[actor] ? al.moves[actor] : 1);
		return true;
	}
	if (wt == 0x17) { // sub_1D89E: a 0x198 with a 0x1A8 where it stands is taken out of the world; without one it turns into a 0x1A8. A 0x1A8 makes sure a 0x198 (frame 3) lies where it stands, steps one way
		// at random and takes the 0x198 along; 5 moves
		if (o.type == 0x198) {
			if (seFind(0x1A8, o.x, o.y) >= 0) {
				_world.spendMoves(actor, al.moves[actor] ? al.moves[actor] : 1);
				remove();
				return true;
			}
			o.type = 0x1A8;
			o.frame = 0;
		}
		if (o.type != 0x1A8)
			return false;
		int piece = seFind(0x198, o.x, o.y);
		if (piece < 0)
			piece = seMake(0x198, o.x, o.y, o.z, 0, 0);
		if (piece >= 0)
			_world.objects[piece].frame = 3;
		static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
		int dir = seRoll(0, 7);
		_creatures.walkTo(actor, o.x + DX[dir], o.y + DY[dir]);
		if (piece >= 0)
			_world.moveObject(piece, _world.objects[k].x, _world.objects[k].y, _world.objects[k].z);
		_world.spendMoves(actor, 5);
		return true;
	}
	if (wt == 0x0D) { // sub_1D230 (the creeping vine: the root 0x172 and the tip 0x174, each the quality of the other's actor; the tip leaves its trail 0x173 behind and, when it goes more than eight squares
		// from the root or is hemmed in, draws it in again; sub_1CFF6). A root without a tip grows one next to it, towards the nearest member, when that member is not next to it; the tip strikes a member
		// next to it (10 moves), or steps along the way to the member (its look: the facing in the movement flags, the frame of the trail the shape of its bend)
		if (o.type != 0x172 && o.type != 0x174)
			return false;
		static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
		MoveRules rules;
		rules.partyMayShuffle = false;
		int type = o.type, z = o.z, link = o.quality;
		auto setFlags = [&](int dir) { al.movementFlags[actor] = (uint8_t)((al.movementFlags[actor] & 0xF8) | dir); };
		auto spend = [&](int n) { _world.spendMoves(actor, n); };
		auto gone = [&]() { // the end of a vine whose other end is gone
			al.hp[actor] = 0;
			al.status[actor] |= 0x10;
			_world.deleteObject(k);
		};
		// sub_1CFF6 (flag 0): the tip draws in the trail behind it; the direction it faced is given back (+1 in its quantity), -1 if there was no trail to draw in
		auto retract = [&]() {
			int oldFacing = o.frame & 6, behind = (o.frame + 4) & 6;
			int sx = (o.x + DX[behind]) & 0x3FF, sy = (o.y + DY[behind]) & 0x3FF;
			int seg = -1;
			std::vector<int> here;
			_world.objectsAt(sx, sy, o.z, here);
			for (int h : here)
				if (!_world.objects[h].deleted && _world.objects[h].type == 0x173 && _world.objects[h].quality == actor) {
					seg = h;
					break;
				}
			if (seg < 0) {
				spend(5);
				return -1;
			}
			if (!mdInViewAt(_world.objects[seg].x, _world.objects[seg].y, _world.objects[seg].z))
				return -1;
			int sf = _world.objects[seg].frame, f = o.frame, nd;
			if (sf > 1) {
				int v = (sf - 1) * 3 - (1 << (f >> 1)), n = 0;
				while ((v >>= 1) != 0)
					n += 2;
				nd = (n + 4) & 6;
			} else {
				nd = f & 6;
			}
			_world.deleteObject(seg);
			_world.moveObject(k, sx, sy, o.z);
			o.frame = (uint8_t)(nd + ((f & 1) ? 0 : 1));
			setFlags(nd);
			return oldFacing;
		};
		auto backed = [&]() { // what the tip does after drawing in the trail: remembers the way it came (quantity = direction + 1, 0xFF when there was none)
			int d = retract();
			o.quantity = d >= 0 ? (uint8_t)(d + 1) : 0xFF;
		};
		if (link != 0) { // the other end is gone: so is this
			int lk = _world.actorObject(link);
			if ((al.status[link] & 0x10) || lk < 0 || _world.objects[lk].deleted) {
				gone();
				return true;
			}
		}
		if (!mdInView(actor) && type != 0x172) {
			spend(5);
			return true;
		}
		if (type == 0x174 && link != 0) {
			int lk = _world.actorObject(link);
			if (!mdInView(link) || std::abs(_world.objects[lk].x - o.x) > 8 || std::abs(_world.objects[lk].y - o.y) > 8) {
				backed();
				spend(5);
				return true;
			}
		}
		int d, m = nearest(d);
		if (m < 0) {
			spend(5);
			return true;
		}
		int mk = _world.actorObject(m), mx = _world.objects[mk].x, my = _world.objects[mk].y;
		int sx = (mx > o.x) - (mx < o.x), sy = (my > o.y) - (my < o.y); // sub_17D1D
		int dir = sx < 0 ? (sy < 0 ? 7 : sy > 0 ? 5 : 6) : sx > 0 ? (sy < 0 ? 1 : sy > 0 ? 3 : 2) : (sy < 0 ? 0 : 4);
		bool diagonal = dir & 1;
		dir &= ~1;
		int frame0 = o.frame;
		auto canEnter = [&](int cx, int cy) { return _movement.check(cx & 0x3FF, cy & 0x3FF, z, k, rules).passable; };
		if (type == 0x172) {
			if (link == 0 && d > 1) {
				int made = -1, use = dir;
				for (int t = 0; t < 4 && made < 0; ++t) {
					use = (dir + 2 * t) & 7;
					made = _creatures.spawn(0x174, 0x0D, o.x + DX[use], o.y + DY[use], z, true);
				}
				if (made >= 0) {
					int ck = _world.actorObject(made);
					al.unknown12f1[made] = 0x0D;
					o.frame = (uint8_t)(use >> 1);
					_world.objects[ck].frame = (uint8_t)(use + seRoll(0, 1));
					_world.spendMoves(made, 5);
					_world.objects[ck].quality = (uint8_t)actor;
					o.quality = (uint8_t)made;
				} else {
					if (_mdDreamFlag)
						--_mdDreamFlag;
					al.status[actor] |= 0x10;
					_world.deleteObject(k);
					return true;
				}
			}
			spend(al.moves[actor] ? al.moves[actor] : 1);
			return true;
		}
		if (d < 2) {
			_combat.attack(actor, m);
			spend(10);
			return true;
		}
		for (;;) {
			int nx = (o.x + DX[dir]) & 0x3FF, ny = (o.y + DY[dir]) & 0x3FF, trail = -1;
			std::vector<int> here;
			_world.objectsAt(nx, ny, z, here);
			for (int h : here)
				if (!_world.objects[h].deleted && _world.objects[h].type == 0x173 && _world.objects[h].quality == actor) {
					trail = h;
					break;
				}
			if (trail >= 0) { // it would run into its own trail: draw it in
				backed();
				return true;
			}
			if (o.quantity - 1 != dir && canEnter(nx, ny) && mdInViewAt(nx, ny, z)) {
				int opp = (frame0 + 4) & 6, shape;
				if (dir == opp || (frame0 & 6) == dir)
					shape = (dir == 0 || dir == 4) ? 1 : 0;
				else
					shape = ((1 << (dir >> 1)) + (1 << (opp >> 1))) / 3 + 1;
				int px = o.x, py = o.y;
				int piece = seMake(0x173, px, py, z, 0, actor);
				if (piece >= 0) {
					_world.objects[piece].frame = (uint8_t)shape;
					_world.moveObject(k, nx, ny, z);
					setFlags(dir);
					o.quantity = 0xFF;
				}
				spend(5);
				return true;
			}
			if (diagonal) {
				diagonal = false;
				dir = (dir + 2) & 6;
				continue;
			}
			if (seRoll(0, 1) == 0) {
				spend(5);
				return true;
			}
			backed();
			spend(5);
			return true;
		}
	}
	if (wt >= 0x99 && wt <= 0x9E) { // the stoker of the coal (sub_29CE2's chain of six worktypes: 29F0E, 29F7B, 2A053, 2A0D6, 2A154, 2A1DA): 0x99 walks north until a 0x1BE (the shovel's rack) is two squares ahead (the sound 0x10 if in view),
		// 0x9A looks south for the belt 0xBC (a coal 0x1BF is made on the square below) or the furnace 0xC0 and goes on to 0x9B, else walks south; 0x9B waits while the coal lies there and begins
		// again (0x99) when it is gone and no furnace stands there; 0x9C walks south until a 0xBF is two squares ahead (0x9D) and, hemmed in, says "Stoker's path is blocked!" when in view; 0x9D throws
		// the coal on the square below away (0x9E; waits while there is none); 0x9E walks north to the switch 0xE9 (north of him or north-east): when the furnace's flag (bit 4 of actor 0x72) is not set the
		// belt starts and the switch turns, else the power underground is put on again (cut off first when the belt had been started by hand); then 0x9C
		// APPROX(97): the lamps and the switch's frame (sub_3F540, sub_3F74A, sub_3F793) are not done, the walk is the engine's `step`
		auto setWt = [&](int n) {
			al.worktype[actor] = (uint8_t)n;
			al.unknown12f1[actor] = (uint8_t)n;
		};
		auto blocked = [&]() {
			if (mdInView(actor)) {
				message("Stoker's path is blocked!\n");
				mdSfx(0);
			}
		};
		int ox = o.x, oy = o.y;
		switch (wt) {
		case 0x99:
			if (_creatures.step(actor, 0))
				return true;
			if (seFind(0x1BE, ox, (oy - 2) & 0x3FF) < 0)
				return true;
			if (mdInView(actor))
				mdSfx(0x10);
			setWt(0x9A);
			return true;
		case 0x9A:
			if (seFind(0xBC, ox, (oy + 1) & 0x3FF) >= 0) {
				int coal = seMake(0x1BF, ox, (oy + 1) & 0x3FF, o.z, 0, 0);
				if (coal >= 0)
					_world.objects[coal].status &= 0xDF;
				setWt(0x9B);
			} else if (seFind(0xC0, ox, (oy + 1) & 0x3FF) >= 0) {
				setWt(0x9B);
			} else {
				_creatures.step(actor, 4);
			}
			return true;
		case 0x9B:
			if (seFind(0x1BF, ox, (oy + 1) & 0x3FF) >= 0)
				return true;
			if (seFind(0xC0, ox, (oy + 1) & 0x3FF) < 0)
				setWt(0x99);
			return true;
		case 0x9C:
			if (seFind(0xBF, ox, (oy + 2) & 0x3FF) >= 0) {
				setWt(0x9D);
				return true;
			}
			if (_creatures.step(actor, 4))
				return true;
			blocked();
			return true;
		case 0x9D: {
			int coal = seFind(0x1BF, ox, (oy + 1) & 0x3FF);
			if (coal < 0)
				return true;
			while (coal >= 0) {
				_world.deleteObject(coal);
				coal = seFind(0x1BF, ox, (oy + 1) & 0x3FF);
			}
			setWt(0x9E);
			return true;
		}
		default: { // 0x9E
			if (_creatures.step(actor, 0))
				return true;
			int sw = seFind(0xE9, ox, (oy - 1) & 0x3FF);
			if (sw < 0)
				sw = seFind(0xE9, (ox + 1) & 0x3FF, (oy - 1) & 0x3FF);
			if (sw < 0) {
				blocked();
				return true;
			}
			auto &t = al.talkFlags;
			if (t[0x72] & 4) {
				if (!((t[0x73] & 4) && !(t[0x71] & 8))) {
					if (t[0x71] & 8) {
						t[0x73] &= 0xFB;
						t[0x71] &= 0xF7;
						mdPowerUnderground(false);
					}
					t[0x73] |= 4;
					mdPowerUnderground(true);
					mdScenePower();
				}
			} else {
				mdCoalBelt();
				_world.objects[sw].frame |= 4; // APPROX(97): the original sets the switch's word from the object numbered (sw | 4), which looks like a slip
			}
			setWt(0x9C);
			return true;
		}
		}
	}
	if (wt == 0x18) { // sub_50FEC (the cave worm): it makes for the first seed mound 0xFB within ten squares, else for the party; next to a mound it eats it ("The cave worm eats the pod plant!", 10 moves);
		// otherwise it takes the one of the eight steps that scores best (sub_50ED6: 200 less 15 for each square from the goal, 50 more next to it, less 50-100 for a square it cannot enter, a flame
		// 0x9D of frame 1 or more or a wall or water making it -1000) and, if nothing scores above 0, a step at random
		int tx = x, ty = y, pod = -1;
		mdEach(o.x - 10, o.y - 10, o.x + 10, o.y + 10, [&](int i) {
			if (_world.objects[i].type != 0xFB)
				return false;
			pod = i;
			tx = _world.objects[i].x;
			ty = _world.objects[i].y;
			return true;
		}, o.z);
		if (pod >= 0 && std::abs(o.x - tx) <= 1 && std::abs(o.y - ty) <= 1) {
			message("The cave worm eats the pod plant!\n");
			_world.deleteObject(pod);
			_world.spendMoves(actor, 10);
			return true;
		}
		MoveRules rules;
		rules.partyMayShuffle = false;
		auto score = [&](int sx, int sy) {
			int dist = std::max(std::abs(tx - sx), std::abs(ty - sy));
			int s = 200 - dist * 15 + (dist <= 1 ? 50 : 0);
			if (!_movement.check(sx & 0x3FF, sy & 0x3FF, o.z, k, rules).passable) {
				int f = _world.flags.flags1[_world.map.tile(o.z, sx & (o.z == 0 ? 0x3FF : 0xFF), sy & (o.z == 0 ? 0x3FF : 0xFF)) & (TileFlags::TILES - 1)];
				if (f & 6)
					return -1000;
				s -= (f & 8) ? 100 : 50;
			}
			bool deadly = false;
			mdEach(sx - 1, sy - 1, sx + 1, sy + 1, [&](int i) {
				const World::Obj &b = _world.objects[i];
				if (b.type == 0x9D) {
					if (b.frame > 0)
						deadly = true;
					else
						s -= 5;
				} else if (b.type == 0x9E) {
					--s;
				}
				return deadly;
			}, o.z);
			return deadly ? -1000 : s;
		};
		static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
		int best = score(o.x, o.y), dir = -1;
		for (int d = 0; d < 8; ++d) {
			int s = score(o.x + DX[d], o.y + DY[d]);
			if (s > best || (s == best && seRoll(0, 1))) {
				best = s;
				dir = d;
			}
		}
		if (best < 0)
			dir = seRoll(0, 7);
		if (dir >= 0 && _movement.check((o.x + DX[dir]) & 0x3FF, (o.y + DY[dir]) & 0x3FF, o.z, k, rules).passable)
			_world.moveObject(k, (o.x + DX[dir]) & 0x3FF, (o.y + DY[dir]) & 0x3FF, o.z);
		_world.spendMoves(actor, 5);
		return true;
	}
	if (wt == 0x1A) {
		int made = seMake(o.type, o.x, o.y, o.z, al.hp[actor], al.objQuantity[actor] >> 8);
		_world.objects[made].frame = o.frame;
		_world.spendMoves(actor, 5);
		_world.deleteObject(k);
		return true;
	}
	if (wt == 0x15) {
		int best = -1, bestD = 99;
		for (int m = 0; m < al.partySize; ++m) {
			int a = al.partyRoster[m], mk = _world.actorObject(a);
			if (mk < 0 || (al.status[a] & 0x10) || a == actor)
				continue;
			int d = std::max(std::abs(_world.objects[mk].x - o.x), std::abs(_world.objects[mk].y - o.y));
			if (d < bestD) {
				bestD = d;
				best = a;
			}
		}
		if (best >= 0) {
			int mk = _world.actorObject(best);
			if (bestD < 2)
				_combat.applyDamage(actor, best, 6);
			else
				_creatures.walkTo(actor, _world.objects[mk].x, _world.objects[mk].y);
		}
		_world.spendMoves(actor, 3);
		return true;
	}
	static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
	auto hurtSquare = [&]() { // whatever shares the square of the creature is hurt for 1-10 twice
		std::vector<int> here;
		_world.objectsAt(_world.objects[k].x, _world.objects[k].y, _world.objects[k].z, here);
		for (int h : here) {
			int a = _world.objects[h].actor;
			if (h == k || a < 0 || al.hp[a] == 0 || (al.status[a] & 0x10))
				continue;
			_combat.applyDamage(actor, a, seRoll(1, 10) + seRoll(1, 10));
		}
	};
	if (wt == 0x16) { // sub_1C710 (the creature of the maze, 0x18E): it goes for the 0xA2 the body carries, else the one lying in the view; next to the body it strikes, next to the thing it eats it, which in the
		// maze dream's last room (x 0x37 or more, y 0x37 or less; not while the flag 0x40 of actor 0x54 is set) ends the dream scene of actor 0x54
		if (_mdDreamId == 0x60) {
			_world.deleteObject(k);
			return true;
		}
		int tx = -1, ty = -1, lure = -1;
		bool body = mdFindCarried(0, 0xA2) >= 0;
		int bk = _world.actorObject(0);
		if (body && bk >= 0) {
			tx = _world.objects[bk].x;
			ty = _world.objects[bk].y;
		} else {
			for (size_t i = 0x101; i < _world.objects.size() && lure < 0; ++i) {
				const World::Obj &t = _world.objects[i];
				if (!t.deleted && t.onMap() && t.type == 0xA2 && mdInViewAt(t.x, t.y, t.z)) {
					lure = (int)i;
					tx = t.x;
					ty = t.y;
				}
			}
		}
		if (tx < 0) { // nothing to go for: it drifts and hurts (sub_1C622)
			_world.spendMoves(actor, 5);
			int dir = seRoll(0, 7), mask = o.z == 0 ? 0x3FF : 0xFF;
			MoveRules rules;
			if (_movement.check((o.x + DX[dir]) & mask, (o.y + DY[dir]) & mask, o.z, k, rules).passable)
				_world.moveObject(k, (o.x + DX[dir]) & mask, (o.y + DY[dir]) & mask, o.z);
			hurtSquare();
			return true;
		}
		if (std::max(std::abs(tx - o.x), std::abs(ty - o.y)) <= 1) {
			if (body) {
				_combat.attackWith(actor, 0, o.type, false);
				_world.spendMoves(actor, 10);
				return true;
			}
			_world.deleteObject(lure);
			ObjList &al2 = _world.actors;
			if (!(al2.talkFlags[0x54] & 0x40) && o.x >= 0x37 && o.y <= 0x37) {
				mdDreamEnd(0x54);
				al2.talkFlags[0x54] |= 2;
				startTalk(0x54);
				_talkEnd = [this]() { mdDreamWake(); };
				return true;
			}
		}
		_creatures.walkTo(actor, tx, ty);
		hurtSquare();
		_world.spendMoves(actor, 5);
		return true;
	}
	// 0x13
	_world.spendMoves(actor, 5);
	if (o.type == 0x18E && _mdDreamId == 0x44 && seRoll(0, 1) != 0)
		return true;
	int dir = seRoll(0, 7), mask = o.z == 0 ? 0x3FF : 0xFF;
	MoveRules rules;
	int nx = (o.x + DX[dir]) & mask, ny = (o.y + DY[dir]) & mask;
	if (_movement.check(nx, ny, o.z, k, rules).passable)
		_world.moveObject(k, nx, ny, o.z);
	std::vector<int> here;
	_world.objectsAt(_world.objects[k].x, _world.objects[k].y, _world.objects[k].z, here);
	for (int h : here) {
		int a = _world.objects[h].actor;
		if (h == k || a < 0 || al.hp[a] == 0 || (al.status[a] & 0x10))
			continue;
		_combat.applyDamage(actor, a, seRoll(1, 10) + seRoll(1, 10));
	}
	return true;
}

// What the hour changing does (sub_22B27's hour loop): in thin air (while not asleep-until-dawn or dreaming) each member but the mechanical man (6) breathes a charge of the oxygen tank 0x83
// (the quantity; the last charge uses the tank up and the next tank is looked for), without one he gasps; the street lamps (0xE4) are on (frame 7) by night and off (3) by day when the power of the
// surface is on
void Game::mdHour() {
	ObjList &al = _world.actors;
	if (!_mdDreaming && !_mdSleeping && (al.talkFlags[0x10] & 0x20)) {
		for (int m = 0; m < al.partySize; ++m) {
			int a = al.partyRoster[m];
			if (a == 6 || (al.status[a] & 4))
				continue;
			int tank = mdFindCarried(a, 0x83);
			if (tank < 0) {
				mdGasp(a, true);
				continue;
			}
			if (_world.objects[tank].quantity > 1) {
				--_world.objects[tank].quantity;
				mdGasp(a, false);
				continue;
			}
			_world.deleteObject(tank);
			mdGasp(a, mdFindCarried(a, 0x83) < 0);
		}
	}
	if (al.talkFlags[0x73] & 0x10) {
		int frame = (gameClock.hour < 6 || gameClock.hour > 0x11) ? 7 : 3;
		for (size_t i = 0x101; i < _world.objects.size(); ++i)
			if (!_world.objects[i].deleted && _world.objects[i].type == 0xE4)
				_world.objects[i].frame = (uint8_t)frame;
	}
}

} // namespace Ultima6
} // namespace Ultima6

namespace Ultima6 {
namespace Ultima6 {

// sub_2043E: the things that can be struck (ds:0E5C: 0xB3, 0x184, 0x133 the pod, 0x12C, 0xD9 the window) with hit points left in their quantity; -1 if there is none on the square
int Game::mdBreakableAt(int tx, int ty) const {
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	for (int i : here) {
		const World::Obj &o = _world.objects[i];
		if (!o.deleted && o.actor < 0 && o.quantity != 0 && (o.type == 0xB3 || o.type == 0x184 || o.type == 0x133 || o.type == 0x12C || o.type == 0xD9))
			return i;
	}
	return -1;
}

// sub_19A74, the branch of the things: a blow of the weapon (rolled from 1 to its damage) takes hit points off the quantity. A pod (0x133) that is struck "splits open!" and a creature 0x180 comes
// out; a 0x184 (frame 0, quality 11-18, the sound 0x22) goes down by the blow; a window 0xD9 that has no hit points left (the sound 0x1A) becomes a broken one 0xDA, and in the dream 0x44 the last of
// them (none left in x 0x37-0x47, y 0x37-0x47) sets bit 0x40 of actor 0x54's flags; the rest are gone (the sound 0).
// APPROX(97): the kinds of damage (fire, cold: sub_204C0, sub_202F7) and the thawing and freezing of the ice are not done here
void Game::mdHitThing(int attacker, int thing, int weapon, int damage) {
	(void)attacker;
	int dmg = damage;
	if (dmg < 0) {
		dmg = Combat::weaponDamageOf(weapon);
		dmg = dmg <= 1 ? 1 : seRoll(1, dmg);
	}
	World::Obj &o = _world.objects[thing];
	int type = o.type;
	if (o.quantity > dmg) {
		if (type == 0x133) {
			message("The pod splits open!\n");
			_creatures.spawn(0x180, 8, o.x, o.y, o.z);
			_world.deleteObject(thing);
		} else if (type == 0x184) {
			o.frame = 0;
			o.quality = (uint8_t)seRoll(0x0B, 0x12);
			mdSfx(0x22);
			o.quantity = (uint8_t)(o.quantity - dmg);
		} else {
			o.quantity = (uint8_t)(o.quantity - dmg);
		}
		return;
	}
	if (type == 0xD9) {
		mdSfx(0x1A);
		o.type = 0xDA;
		if (_mdDreaming && _mdDreamId == 0x44) {
			bool more = false;
			mdEach(0x37, 0x37, 0x47, 0x47, [&](int i) {
				more |= _world.objects[i].type == 0xD9;
				return more;
			});
			if (!more)
				_world.actors.talkFlags[0x54] |= 0x40;
		}
		return;
	}
	mdSfx(0);
	_world.deleteObject(thing);
}

} // namespace Ultima6
} // namespace Ultima6

namespace Ultima6 {
namespace Ultima6 {

// sub_2F8F8 (run by sub_2CFE2 when a 0x17F is put down): the creatures of type 0x17F (the actors 0xC0 and up) are the pieces of a long body; the frame of each is the sum of 1, 2, 4 and 8 for the
// north, east, south and west neighbours that are 0x17F too
void Game::mdSegments() {
	ObjList &al = _world.actors;
	static const int DX[4] = {0, 1, 0, -1}, DY[4] = {-1, 0, 1, 0};
	auto isPiece = [&](int a) {
		int k = _world.actorObject(a);
		return k >= 0 && !_world.objects[k].deleted && _world.objects[k].type == 0x17F && !(al.status[a] & 0x10);
	};
	for (int a = 0xC0; a < 0x100; ++a) {
		if (!isPiece(a))
			continue;
		World::Obj &o = _world.objects[_world.actorObject(a)];
		int mask = 0;
		for (int d = 0; d < 4; ++d)
			for (int b = 0xC0; b < 0x100; ++b) {
				if (!isPiece(b))
					continue;
				const World::Obj &n = _world.objects[_world.actorObject(b)];
				if (n.x == ((o.x + DX[d]) & 0x3FF) && n.y == ((o.y + DY[d]) & 0x3FF) && n.z == o.z) {
					mask |= 1 << d;
					break;
				}
			}
		o.frame = (uint8_t)mask;
	}
}

} // namespace Ultima6
} // namespace Ultima6

namespace Ultima6 {
namespace Ultima6 {

// sub_1B69D (with sub_1883A, sub_17E0B): who a fighter goes for. Any actor on the level that has a body, is not dead, is not the fighter itself and is seen (not invisible, not swallowed, not of a kind the
// tiles hide; a party member sees an invisible party member), is not a 0x17D, within eleven squares, and of the other side: a fighter of no side goes for anyone that has one; side 0x60 for anyone but its
// own kind (a vine's parts, the brood of a 0x175 are one kind); side 0x40 for those of side 0x20 and the other way round; a sleeper (worktype 7) only within five squares of the party. The nearest
// (the square of the distance) wins, between equals the one whose kind hits harder. -1 if there is none.
int Game::mdTarget(int actor) const {
	const ObjList &al = _world.actors;
	int k = _world.actorObject(actor);
	if (k < 0 || _world.objects[k].z != z)
		return -1;
	const World::Obj &me = _world.objects[k];
	const int side = al.status[actor] & 0x60, mine = me.type;
	int best = -1, bestScore = 0x7FFF;
	auto gap = [&](int a, int b) {
		int d = std::abs(a - b) & 0x3FF;
		return d > 0x200 ? 0x400 - d : d;
	};
	auto power = [&](int type) {
		const CreatureStats *cs = Combat::stats(type);
		return cs ? cs->dmg : 0;
	};
	for (int t = 0; t < ObjList::ACTORS; ++t) {
		int tk = _world.actorObject(t);
		if (tk < 0 || t == actor || _world.objects[tk].deleted || (al.status[t] & 0x10) || _world.objects[tk].z != z)
			continue;
		const World::Obj &o = _world.objects[tk];
		bool seen = !(o.status & 2);
		if (!seen && (al.status[actor] & 0x80) && (al.status[t] & 0x80))
			seen = true;
		if (al.movementFlags[t] & 0x10)
			seen = false;
		if (_world.flags.flags3[(_world.baseTile.firstTile[o.type] + o.frame) & (TileFlags::TILES - 1)] & 0x10)
			seen = false;
		if (!seen || o.type == 0x17D)
			continue;
		int ts = al.status[t] & 0x60;
		if (side == 0 && ts == 0)
			continue;
		if (side == 0x60) {
			int tt = o.type;
			if (tt == mine)
				continue;
			if (mine == 0x174 && (tt == 0x172 || tt == 0x173))
				continue;
			auto brood = [](int ty) { return ty == 0x16A || ty == 0x175 || ty == 0xFE; };
			if (brood(mine) && brood(tt))
				continue;
		}
		if (side == 0x40 && !(ts & 0x20))
			continue;
		if (side == 0x20 && !(ts & 0x40))
			continue;
		int dx = gap(o.x, me.x), dy = gap(o.y, me.y);
		if (std::max(dx, dy) > 0xB)
			continue;
		if (al.worktype[t] == 7 && !(gap(o.x, x) <= 5 && gap(o.y, y) <= 5))
			continue;
		int score = dx * dx + dy * dy;
		if (score < bestScore || (score == bestScore && best >= 0 && power(o.type) > power(_world.objects[_world.actorObject(best)].type))) {
			bestScore = score;
			best = t;
		}
	}
	return best;
}

} // namespace Ultima6
} // namespace Ultima6

namespace Ultima6 {
namespace Ultima6 {

// sub_1BDB5, the choice of the berserk fighter: of those within eight squares that it sees and that are of the other side (side 0x40 against exactly 0x20 and the other way round, a fighter of no
// side against anyone, side 0x60 against anyone but its own kind), the one with most of strength + defence + intelligence (the first when equal); -1 if there is none.
int Game::mdTargetStrongest(int actor) const {
	const ObjList &al = _world.actors;
	int k = _world.actorObject(actor);
	if (k < 0)
		return -1;
	const World::Obj &me = _world.objects[k];
	const int side = al.status[actor] & 0x60;
	int best = -1, bestScore = 0;
	auto gap = [&](int a, int b) {
		int d = std::abs(a - b) & 0x3FF;
		return d > 0x200 ? 0x400 - d : d;
	};
	for (int t = 0; t < ObjList::ACTORS; ++t) {
		int tk = _world.actorObject(t);
		if (tk < 0 || t == actor || _world.objects[tk].deleted || (al.status[t] & 0x10))
			continue;
		const World::Obj &o = _world.objects[tk];
		if (std::max(gap(o.x, me.x), gap(o.y, me.y)) > 8)
			continue;
		bool seen = !(o.status & 2);
		if (!seen && (al.status[actor] & 0x80) && (al.status[t] & 0x80))
			seen = true;
		if (al.movementFlags[t] & 0x10)
			seen = false;
		if (!seen || o.z != me.z)
			continue;
		int ts = al.status[t] & 0x60;
		if (side == 0x40 && ts != 0x20)
			continue;
		if (side == 0x20 && ts != 0x40)
			continue;
		if (side == 0x60) {
			if (o.type == me.type)
				continue;
			if (me.type == 0x174 && (o.type == 0x172 || o.type == 0x173))
				continue;
			auto brood = [](int ty) { return ty == 0x16A || ty == 0x175 || ty == 0xFE; };
			if (brood(me.type) && brood(o.type))
				continue;
		}
		int score = _combat.adjustedStrength(t) + mdDefence(t) + _combat.adjustedIntelligence(t);
		if (score > bestScore) {
			bestScore = score;
			best = t;
		}
	}
	return best;
}

} // namespace Ultima6
} // namespace Ultima6

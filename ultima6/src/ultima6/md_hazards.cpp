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
	const CreatureStats *cs = Combat::stats(me.type);
	if (cs && (cs->flags & (1 << 8)) && (cs->flags & (1 << 10)))
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
// APPROX(97): the flight is not drawn; sub_1A45C's damage routine (the armour) and sub_19088 are not read, the damage is `Combat::applyDamage`; the charges are not used.
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
				if (!cs || !(cs->flags & ((1 << 14) | (1 << 7)))) {
					message("It has no effect.\n");
					continue;
				}
				_world.actors.status[a] |= 2;
				message("`" + mdMemberName(a) + " paralyzed!\n");
				if ((cs->flags & (1 << 7)) && !(cs->flags & (1 << 14)))
					_combat.applyDamage(attacker, a, DAMAGE[idx]);
				continue;
			}
			_combat.applyDamage(attacker, a, DAMAGE[idx]);
		}
	}
}

// sub_29CE2, the worktypes of Martian Dreams that have a routine of their own: 0x13 (sub_1C622) a thing that drifts (5 moves; in the maze dream 0x44 the creature 0x18E does it only half the turns)
// one step in a random direction and hurts whatever shares its square for 1-10 twice; 0x15 (sub_1D819, the child): walks up to the nearest member and hits one next to it for 6; 0x1A
// (sub_2AF67) turns the person into a plain thing of the same looks (its hit points the quantity, its quality kept) where it stands. APPROX(97): the others (0xD Stoker, 0x11, 0x12, 0x14,
// 0x16 - 0x19: sub_1D230, sub_1D7E6, sub_1C91A, sub_1CDC2, sub_1C710, sub_1D89E, sub_50FEC, sub_1CB44) act as the engine's plain fighters do.
bool Game::mdWorktype(int actor) {
	ObjList &al = _world.actors;
	int k = _world.actorObject(actor);
	if (k < 0 || _world.objects[k].deleted || actor >= 0x100)
		return false;
	int wt = al.worktype[actor];
	if (wt != 0x13 && wt != 0x15 && wt != 0x1A)
		return false;
	World::Obj &o = _world.objects[k];
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
	// 0x13
	_world.spendMoves(actor, 5);
	if (o.type == 0x18E && _mdDreamId == 0x44 && seRoll(0, 1) != 0)
		return true;
	static const int DX[8] = {0, 1, 1, 1, 0, -1, -1, -1}, DY[8] = {-1, -1, 0, 1, 1, 1, 0, -1};
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

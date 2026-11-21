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
	int mask = z == 0 ? 0x3FF : 0xFF;
	auto gap = [mask](int d) {
		d &= mask;
		if (d > (mask >> 1))
			d -= mask + 1;
		return std::abs(d);
	};
	return o.z == z && gap(o.x - x) <= 20 && gap(o.y - y) <= 20;
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
				lamp = std::max<int>(lamp, _world.flags.flags1[_world.baseTile.firstTile[o.type] + o.frame] & 3);
			}
		}
		if (lamp)
			light = lamp;
	}
	return light;
}

// sub_22B27(1): one minute of the hazards (the clock itself is the engine's)
void Game::mdMinute() {
	ObjList &al = _world.actors;
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

// Martian Dreams' story events: the script command 0xD1 of the conversations (sub_3F869 = sub_56B3A, a switch over the numbers 1-11, 0x33-0x39, 0x65-0x6F, 0xFE and 0xFF, whose blocks change
// objects at places of the maps, set flags of the actors, move the party or leave an exit code (word_41F2A) for the conversation that called them).
//
// APPROX(88): only the events whose effect is read out of the executable and can be done with the engine's parts are here (1, 2, 4-11, 0x33-0x36, 0xFF, 0x65-0x68, 0x6A-0x6F, 0xFE); the dream scenes
// (the dream ends, sub_3F615, are in md_dream.cpp), the scenes that raise a battle or plant a crop (0x35, 0x37-0x39), the schedule bits of the actors 0x18 and 0x1E (events 4 and 5), the guards (2) and the
// end of the game (0xFF) are not, and what the original does with the exit codes once the conversation is over (Talk's ten cases, sub_55F77) is not either.

#include "ultima6/describe.h"
#include "ultima6/session.h"
#include "ultima6/stats_view.h"

namespace Ultima6 {
namespace Ultima6 {

void Game::mdEvent(int n) {
	ObjList &al = _world.actors;
	// the first thing of a type on a square (sub_2BA57): its frame is set (the original adds frame << 10 to the type), and it may be moved (sub_2CFE2)
	auto find = [&](int type, int x, int y) { return seFind(type, x, y); };
	auto setFrame = [&](int idx, int frame) { _world.objects[idx].frame = (uint8_t)frame; };
	auto aargh = [&]() { message("Aargh!\n"); };
	auto exitCode = [&](int code) { _mdExit = code; };
	switch (n) {
	case 1: { // the control panels (0x1CA) that the party carries are mended (the broken bit 2 goes, the quantity is 4); more than one sets bit 8 of actor 0x39's flags, else it is cleared
		int count = 0;
		for (int m = 0; m < al.partySize; ++m) {
			std::vector<int> inv;
			_world.inventory(al.partyRoster[m], inv, true);
			for (int i : inv)
				if (_world.objects[i].type == 0x1CA) {
					++count;
					mdSfx(4);
					_world.objects[i].quality &= 0xFD;
					_world.objects[i].quantity = 4;
				}
		}
		if (count > 1)
			al.talkFlags[0x39] |= 8;
		else
			al.talkFlags[0x39] &= 0xF7;
		break;
	}
	case 2: // sub_19300(0x3C): actor 0x3C dies
		_world.actors.hp[0x3C] = 0;
		_combat.kill(0x3C);
		break;
	case 4: // the people 0x1E and 0x18 are no longer held back (the bit 0x80 of their object status goes)
		for (int a : {0x1E, 0x18}) {
			int k = _world.actorObject(a);
			if (k >= 0)
				_world.objects[k].status &= 0x7F;
		}
		break;
	case 5: { // actor 0x18 is hidden (bit 0x80), stops, and goes to 0x18, 0x18 of level 6
		int k = _world.actorObject(0x18);
		if (k >= 0) {
			_world.objects[k].status |= 0x80;
			mdPlace(k, 0x18, 0x18, 6);
		}
		al.worktype[0x18] = 0;
		break;
	}
	case 0x35: { // (the combat jingle first, sub_2FF7D) a swarm: six of the creature 0x91 are made round the party (sub_2652C, sub_18126), the number that found a place is the counter of the dream's deaths (byte_41153)
		mdSting();
		_mdDreamFlag = 0;
		for (int i = 0; i < 6; ++i)
			if (_creatures.spawn(0x91, 8, x, y, z) >= 0)
				++_mdDreamFlag;
		if (!_mdDreamFlag)
			aargh();
		break;
	}
	case 0xFF: // the end of the game: the last key ends play and the ending (MARTIAN.EXE's sub_15C84) follows
		_ending = 1;
		break;
	case 6: { // the two things 0xB5 at 0x2C3,0x1F3 and 0x2C4,0x1F3: the first (frame 3) moves to 0x2C2,0x1F3, the second gets frame 7
		int a = find(0xB5, 0x2C3, 0x1F3);
		if (a < 0) {
			aargh();
		} else {
			setFrame(a, 3);
			mdPlace(a, 0x2C2, 0x1F3, 0);
		}
		int b = find(0xB5, 0x2C4, 0x1F3);
		if (b < 0)
			aargh();
		else
			setFrame(b, 7);
		break;
	}
	case 7: { // the 0x98 at 0x2C7,0x1DC gets frame 7
		int a = find(0x98, 0x2C7, 0x1DC);
		if (a < 0)
			aargh();
		else
			setFrame(a, 7);
		break;
	}
	case 8: { // a loaded rail car (0x19A, frame above 3, quality 3) in the square 0x9A..0xAA, 0xBB..0xCB sets bit 4 of actor 0x3A's flags
		for (size_t i = 0; i < _world.objects.size(); ++i) {
			const World::Obj &o = _world.objects[i];
			if (!o.deleted && o.onMap() && o.type == 0x19A && o.frame > 3 && o.quality == 3 && o.x >= 0x9A && o.x <= 0xAA && o.y >= 0xBB && o.y <= 0xCB) {
				al.talkFlags[0x3A] |= 4;
				break;
			}
		}
		break;
	}
	case 9:
	case 10:
		exitCode(n);
		break;
	case 11: { // the 0x98 at 0x13F,0x194: quality 0, frame 7
		int a = find(0x98, 0x13F, 0x194);
		if (a < 0) {
			aargh();
		} else {
			_world.objects[a].quality = 0;
			setFrame(a, 7);
		}
		break;
	}
	case 0x33: { // the 0x4F at 9,0x1F gets frame 1
		int a = find(0x4F, 9, 0x1F);
		if (a < 0)
			aargh();
		else
			setFrame(a, 1);
		break;
	}
	case 0x34: // a dream scene ends (sub_3F615(0x5C): the person 0x5C goes and 0x53 takes the place)
		mdDreamEnd(0x5C);
		exitCode(0x34);
		break;
	case 0x36: // a dream scene ends (sub_3F615(0x52))
		mdDreamEnd(0x52);
		exitCode(0x36);
		break;
	case 0x65:
	case 0x66:
	case 0x68:
		mdSting(); // sub_2FF7D: the combat jingle
		exitCode(n);
		break;
	case 0x67:
		exitCode(n);
		break;
	case 0x69: { // the sound 0x32; by where actor 0x67 stands (x 0x2C or not) the thing 0xDB at 0x3B, 0xC (frame 3) or at 0x2B, 0xF (frame 7) is changed; actor 0x69 becomes a 0x171
		mdSfx(0x32);
		int k67 = _world.actorObject(0x67);
		bool first = k67 >= 0 && (_world.objects[k67].x & 0x3FF) == 0x2C;
		int d = find(0xDB, first ? 0x3B : 0x2B, first ? 0x0C : 0x0F);
		if (d < 0) {
			aargh();
			break;
		}
		setFrame(d, first ? 3 : 7);
		int k69 = _world.actorObject(0x69);
		if (k69 >= 0)
			_world.objects[k69].type = 0x171;
		exitCode(n);
		break;
	}
	case 0x37: { // the planting battle: actor 0x56 goes to level 6; the body (actor 0) is given 4 of 0x9E, a 0xF2 and a 0xA0 of frame 1; a hole (0x101) with a seed (0xFC) in it and a mound (0xFB, quality 15) are made at 0x6F, 0x19; four of the creature 0x16E (frame 1, worktype 0x18) stand at each of 0xA, 0x6F; 0x19, 0x60 and 0x28, 0x6F
		int k56 = _world.actorObject(0x56);
		if (k56 >= 0)
			mdPlace(k56, 0x56, 0x56, 6);
		_world.addToInventory(0, 0x9E, 0, 4);
		_world.addToInventory(0, 0xF2, 0, 1);
		int ice = _world.addToInventory(0, 0xA0, 0, 1);
		if (ice >= 0)
			_world.objects[ice].frame = 1;
		int hole = seMake(0x101, 0x6F, 0x19, z, 1, 0);
		_world.objects[hole].status &= 0xDF;
		int mound = seMake(0xFB, 0x6F, 0x19, z, 0, 0x0F);
		_world.objects[mound].status &= 0xDF;
		seFill(hole, 0xFC, 1);
		mdSting();
		static const int SPOTS[3][2] = {{0x0A, 0x6F}, {0x19, 0x60}, {0x28, 0x6F}};
		for (const auto &sp : SPOTS)
			for (int i = 0; i < 4; ++i) {
				int a = _creatures.spawn(0x16E, 0x18, sp[0], sp[1], z);
				if (a < 0) {
					aargh();
				} else {
					int k = _world.actorObject(a);
					if (k >= 0)
						_world.objects[k].frame = 1;
				}
			}
		break;
	}
	case 0x38: { // the stone puzzle: the flags of actor 0x55 (0x10: no stones, 8: all the stones alike and 25 of them with the number 10, 4: the lens (0x1A9, 0x1AA) is there, 0x20: a stone in the body's hands) are worked out from the stones (0x84) of x 0x49-0x57, y 0x30-0x51 and what the body carries
		uint8_t &f = al.talkFlags[0x55];
		f = (uint8_t)((f | 0x18) & 0xFB);
		int stones = 0, same = 0;
		mdEach(0x49, 0x30, 0x57, 0x51, [&](int i) {
			const World::Obj &o = _world.objects[i];
			if (o.type == 0x84) {
				++stones;
				f &= 0xEF;
				if (same != 0 && o.quantity != same) {
					f &= 0xF7;
					same = -1;
				} else if (same == 0) {
					same = o.quantity;
				}
			} else if (o.type == 0x1A9 || o.type == 0x1AA) {
				f |= 4;
			}
			return false;
		});
		std::vector<int> mine;
		_world.inventory(0, mine, false);
		for (int i : mine) {
			int t = _world.objects[i].type;
			if (t == 0x84)
				f = (uint8_t)((f & 0xE7) | 0x20);
			else if (t == 0x1A9 || t == 0x1AA)
				f |= 4;
		}
		if (stones != 0 && stones != 0x19)
			f &= 0xF7;
		if (same != 0 && same != 0x0A)
			f &= 0xF7;
		exitCode(0x38);
		break;
	}
	case 0x39: { // the body bag (0x155, quality 0x5B) in x 8-0x16, y 0x18-0x39 is taken away and actor 0x5B gets up whole as a plain person (flag 0x10 of its story byte goes)
		int bag = -1;
		for (size_t i = 0; i < _world.objects.size() && bag < 0; ++i) {
			const World::Obj &o = _world.objects[i];
			if (!o.deleted && o.onMap() && o.z == z && o.type == 0x155 && o.quality == 0x5B && o.x >= 8 && o.x <= 0x16 && o.y >= 0x18 && o.y <= 0x39)
				bag = (int)i;
		}
		if (bag < 0) {
			aargh();
			break;
		}
		_world.deleteObject(bag);
		al.status[0x5B] &= 0xEF;
		al.hp[0x5B] = (uint8_t)ActorStats::maxHealth(_world, 0x5B);
		al.status[0x5B] &= 0x9F;
		int k = _world.actorObject(0x5B);
		if (k >= 0) {
			_world.objects[k].type = al.baseObjFrame[0x5B] & 0x3FF;
			_world.objects[k].frame = (uint8_t)(al.baseObjFrame[0x5B] >> 10);
		}
		al.talkFlags[0x5B] &= 0xEF;
		break;
	}
	case 0x6A: // back to where the dream began (the place kept in globals 0x14 and 0x15)
		if (al.globals.size() > 0x15)
			changeLevel(al.globals[0x14], al.globals[0x15], z);
		break;
	case 0x6B: // to 0x6A,0x2C, which is kept as the place to come back to
		mdSfx(4);
		changeLevel(0x6A, 0x2C, z);
		if (al.globals.size() > 0x15) {
			al.globals[0x14] = 0x6A;
			al.globals[0x15] = 0x2C;
		}
		break;
	case 0x6C:
	case 0x6D:
	case 0x6E: { // the party goes to a place (0x6E,0x17; 0x7A,0x0D; 0x82,0x18) in combat mode, the first two with a 0xE0 thing (frame 3 / 7) put at 0x75,0x17 / 0x7A,0x12 unless there is one
		if (al.globals.size() > 0x34 && !al.globals[0x34])
			toggleCombat();
		if (al.globals.size() > 0x33)
			al.globals[0x33] = 6;
		if (n == 0x6C || n == 0x6D) {
			int px = n == 0x6C ? 0x6E : 0x7A, py = n == 0x6C ? 0x17 : 0x0D, tx = n == 0x6C ? 0x75 : 0x7A, ty = n == 0x6C ? 0x17 : 0x12;
			changeLevel(px, py, z);
			if (find(0xE0, tx, ty) < 0) {
				int o = seMake(0xE0, tx, ty, z, 0, 0);
				_world.objects[o].frame = n == 0x6C ? 3 : 7;
				_world.objects[o].status &= 0xDF;
			}
		} else {
			changeLevel(0x82, 0x18, z);
		}
		break;
	}
	case 0x6F: // the Avatar is made whole
		al.hp[1] = (uint8_t)ActorStats::maxHealth(_world, 1);
		break;
	case 0xFE: // the dizziness (the counter of the berries) goes up
		if (al.globals.size() > 0x16)
			++al.globals[0x16];
		break;
	default: // (the others are not done: see above)
		break;
	}
}

} // namespace Ultima6
} // namespace Ultima6

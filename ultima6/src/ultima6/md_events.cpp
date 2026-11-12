// Martian Dreams' story events: the script command 0xD1 of the conversations (sub_3F869 = sub_56B3A, a switch over the numbers 1-11, 0x33-0x39, 0x65-0x6F, 0xFE and 0xFF, whose blocks change
// objects at places of the maps, set flags of the actors, move the party or leave an exit code (word_41F2A) for the conversation that called them).
//
// APPROX(88): only the events whose effect is read out of the executable and can be done with the engine's parts are here (1, 6-11, 0x33, 0x34, 0x36, 0x65-0x68, 0x6A-0x6F, 0xFE); the dream scenes
// (sub_3F615 / sub_3F61A end a dream), the scenes that raise a battle or plant a crop (0x35, 0x37-0x39), the schedule bits of the actors 0x18 and 0x1E (events 4 and 5), the guards (2) and the
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
					sfx(4);
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
	case 0x34: // (a dream ends, sub_3F615(0x5C): not done)
		exitCode(0x34);
		break;
	case 0x36: // (a dream ends, sub_3F615(0x52): not done)
		exitCode(0x36);
		break;
	case 0x65:
	case 0x66:
	case 0x67:
	case 0x68:
		exitCode(n);
		break;
	case 0x6A: // back to where the dream began (the place kept in globals 0x14 and 0x15)
		if (al.globals.size() > 0x15)
			changeLevel(al.globals[0x14], al.globals[0x15], z);
		break;
	case 0x6B: // to 0x6A,0x2C, which is kept as the place to come back to
		sfx(4);
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

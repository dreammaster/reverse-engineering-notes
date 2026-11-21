// Martian Dreams' dream machines and the dreams (GAME.EXE overlay seg057: sub_5BC53 the machines' use, sub_4FB90 / sub_4FE5E the start of a dream, sub_50007 the waking, sub_50651 the end of a
// scene, sub_50218 the squares of the dream with their flags, sub_27EE6 the step onto them).
//
// A dream is the Avatar's body (actor 0, parked at 6,4 of level 6 while nobody dreams) put on level 2 (or 3) of the map; the party stays where it sat, the roster gets a place for the body
// (the one after the last member, which the party size does not count) and the player walks it alone (the original's word_4165C = 0x92, `_solo`). The squares of the dream have objects of type
// 0x1CD, whose quantity and quality are the place they lead to (y and x) and whose status says what else they do (the bits 0xE5): 0x24 wakes the dreamer, 1 forgets the dream, 0xC4, 0xC5, 0xE0
// and the like finish a scene. Waking puts the body back, sets the clock to eight in the morning of the next day and calls the morning song.
//
// APPROX(95): the scenes of the dreams themselves (the routines sub_3F6A0 .. sub_3F6D7 = sub_5189D .. sub_52B0C that the flags 4, 0x40, 0x85, 0xA1 and the dream numbers 5, 0x20, 0x44 and
// 0xA0 call), the dream picture panels (MDREAM.LZC), the fades and the shapes of the transitions (sub_258D9), the creatures the dream clears (sub_2CAAD), the stray flags (the array ds:6EC6,
// byte_41153, 41134, 41154) and the state of a dream that is saved are not done.

#include "ultima6/session.h"
#include "ultima6/stats_view.h"

#include <cstring>

namespace Ultima6 {
namespace Ultima6 {

// The dream body, actor 0: the Avatar's looks for the sex the game keeps (the original's byte_41158), whole, and put on the dream's square. sub_4FB90 / sub_4FE5E from the dream machine's
// chair on: the roster gets the body after the last member and the player controls it alone.
void Game::mdDreamBody(int bx, int by, int bz, int id) {
	ObjList &al = _world.actors;
	int k = _world.actorObject(0);
	if (k < 0)
		return;
	World::Obj &body = _world.objects[k];
	body.frame = 1;
	body.status &= 0x18;
	al.status[0] = 0xC0;
	al.hp[0] = (uint8_t)ActorStats::maxHealth(_world, 1);
	al.worktype[0] = 2;
	body.type = mdMale(1) ? 0x157 : 0x158;
	mdPlace(k, bx, by, bz);
	for (int w = 0; w < 3; ++w)
		mdSetPsychic(0, w, 0);
	if (al.globals.size() > 0x16)
		al.globals[0x16] = 0; // byte_41136: no dizziness
	int slot = al.partySize;
	if (slot < ObjList::PARTY_MAX) {
		al.partyRoster[slot] = 0;
		std::memcpy(al.partyNames[slot], al.partyNames[0], ObjList::NAME_SIZE);
		_solo = slot;
		_member = slot;
		_selected = 0;
		_leaderObject = k;
	}
	x = body.x;
	y = body.y;
	z = body.z;
	_mdDreaming = true;
	_mdDreamId = id;
	chooseMusic();
}

// sub_4FB90(quality, sitter): the date the dreamer wakes on is kept (this day if it is before eight in the morning, else the next); the Avatar dreams for real (the body goes to the place of
// the machine's quality: 0 and 1 to x 100, y 59 of level 2, the others to 147, 52), another member of the party sleeps until dawn while the party waits.
void Game::mdDreamStart(int quality, int sitter) {
	_mdWake[0] = gameClock.day;
	_mdWake[1] = gameClock.month;
	_mdWake[2] = gameClock.year;
	if (gameClock.hour >= 8) {
		if (++_mdWake[0] > 0x1C) {
			_mdWake[0] = 1;
			if (++_mdWake[1] > 0xC) {
				_mdWake[1] = 1;
				++_mdWake[2];
			}
		}
	}
	if (sitter == 1) {
		if (quality <= 1)
			mdDreamBody(0x64, 0x3B, 2, 0);
		else
			mdDreamBody(0x93, 0x34, 2, 0);
		return;
	}
	ObjList &al = _world.actors;
	al.status[sitter] |= 4;
	_solo = -1;
	message(mdMemberName(sitter) + " dreams until dawn while the party waits.\n");
	advanceTime(60 - gameClock.minute);
	while (!_dead && !(gameClock.year >= _mdWake[2] && gameClock.month >= _mdWake[1] && gameClock.day >= _mdWake[0] && gameClock.hour >= 8))
		advanceTime(20);
	al.status[sitter] &= 0xFB;
	_mdMorning = true;
}

// sub_4FE5E: the dream of the machine that has a story flag (the machine of quality 3 when talkFlags[0x46] has bit 1): the body goes to 0x35, 0x26 of level 3, and the dream is number 0xE1
void Game::mdDreamStartLevel3() {
	_mdWake[0] = gameClock.day;
	_mdWake[1] = gameClock.month;
	_mdWake[2] = gameClock.year;
	if (gameClock.hour >= 8 && ++_mdWake[0] > 0x1C) {
		_mdWake[0] = 1;
		if (++_mdWake[1] > 0xC) {
			_mdWake[1] = 1;
			++_mdWake[2];
		}
	}
	mdDreamBody(0x35, 0x26, 3, 0xE1);
}

// The squares of a rectangle of the level (sub_2BF6E, sub_2BE67): the objects in the order of the list, the first for which `f` says true stops the walk
void Game::mdEach(int x1, int y1, int x2, int y2, const std::function<bool(int)> &f) {
	for (size_t i = 0; i < _world.objects.size(); ++i) {
		const World::Obj &o = _world.objects[i];
		if (o.deleted || !o.onMap() || o.z != z || o.x < x1 || o.x > x2 || o.y < y1 || o.y > y2)
			continue;
		if (f((int)i))
			return;
	}
}

// The cleaning up that goes with the number of the dream (the table at cs:0AAD of sub_50218, run when a flagged square is touched and when the dreamer wakes): 5 (sub_523D0) takes the
// people 0x17B and 0x193 of the region x 0x76-0xCE, y 0x98-0xCC away; 0x40 (sub_5189D's cliff) gives the body its looks again; 0x44 (sub_51C35) takes the things 0x18E of the region 0x33-0x43,
// 0x21-0x3D away and marks the scene in the flags of actor 0x54; 0xA0 (sub_51759) takes everything the body carries but the things 0x18 away. The number 0x20 does nothing here.
void Game::mdDreamLeave() {
	ObjList &al = _world.actors;
	switch (_mdDreamId) {
	case 5:
		for (int a = 2; a < ObjList::ACTORS; ++a) {
			int k = _world.actorObject(a);
			if (k < 0)
				continue;
			const World::Obj &o = _world.objects[k];
			if (o.deleted || !o.onMap() || o.z != z || o.x < 0x76 || o.x > 0xCE || o.y < 0x98 || o.y > 0xCC)
				continue;
			if (o.type == 0x17B || o.type == 0x193)
				_world.deleteObject(k);
		}
		break;
	case 0x40: {
		int k = _world.actorObject(0), a1 = _world.actorObject(1);
		if (k >= 0 && a1 >= 0) {
			_world.objects[k].type = _world.objects[a1].type;
			_world.objects[k].frame = 9;
		}
		break;
	}
	case 0x44: {
		uint8_t &f = al.talkFlags[0x54];
		if (!(f & 0x80))
			f |= (f & 8) ? 0x10 : 8;
		mdEach(0x21, 0x33, 0x3D, 0x43, [this](int i) {
			if (_world.objects[i].type != 0x18E)
				return false;
			_world.deleteObject(i);
			return true;
		});
		break;
	}
	case 0xA0: {
		std::vector<int> inv;
		int k = _world.actorObject(0);
		if (k >= 0)
			_world.inventory(0, inv, true);
		for (int i : inv)
			if (_world.objects[i].type != 0x18)
				_world.deleteObject(i);
		break;
	}
	default:
		break;
	}
}

// sub_514B0(qty, qual), the scene of the square 0x20 once the body has come to (qual, qty): actor 0x5B is put right (hit points made whole, a plain person, at 13, 29 of level 2) unless it is
// already there as the person 0x15B; the story flags and the sides of the actors 0x59-0x5D are reset (worktype 0x8F for 0x5B); the 0x4F at 9, 31 loses its frame; the things 0x10, 0x38, 0x14B,
// 0x14E and 0x155 within x 8-0x1F, y 0x10-0x37 are taken away; the body is given a 0x14B with a 0x38 in it; and as many foes (0x16C, with the offsets of ds:00AE and ds:00B6, as the party has
// members besides the Avatar, or four) as that appear round the body, with the combat they start; the number of them is kept for the death routine (byte_41153).
void Game::mdSceneBattle(int qty, int qual) {
	ObjList &al = _world.actors;
	int b = _world.actorObject(0x5B);
	if (b >= 0 && !(_world.objects[b].type == 0x15B && _world.objects[b].z == 2)) {
		al.status[0x5B] &= 0xEF;
		al.hp[0x5B] = (uint8_t)ActorStats::maxHealth(_world, 0x5B);
		al.status[0x5B] &= 0xF7;
		_world.objects[b].type = al.baseObjFrame[0x5B] & 0x3FF;
		_world.objects[b].frame = (uint8_t)(al.baseObjFrame[0x5B] >> 10);
		mdPlace(b, 0x0D, 0x1D, 2);
	}
	al.talkFlags[0x5D] &= 0xA7;
	al.talkFlags[0x5C] &= 0xE6;
	al.talkFlags[0x5B] = al.talkFlags[0x5A] = al.talkFlags[0x59] = 0;
	for (int a = 0x59; a <= 0x5D; ++a)
		al.status[a] &= 0x9F;
	al.worktype[0x5B] = 0x8F;
	al.worktype[0x5A] = al.worktype[0x5D] = al.worktype[0x5C] = al.worktype[0x59] = 0;
	int f = seFind(0x4F, 9, 0x1F);
	if (f >= 0)
		_world.objects[f].frame = 0;
	else
		message("Aargh!\n");
	for (size_t i = 0x101; i < _world.objects.size(); ++i) {
		const World::Obj &o = _world.objects[i];
		if (o.deleted || !o.onMap() || o.z != z || o.x < 8 || o.x > 0x1F || o.y < 0x10 || o.y > 0x37)
			continue;
		if (o.type == 0x10 || o.type == 0x38 || o.type == 0x14B || o.type == 0x14E || o.type == 0x155)
			_world.deleteObject((int)i);
	}
	int vial = _world.addToInventory(0, 0x14B, 0, 1);
	if (vial < 0) {
		message("Aargh!\n");
	} else {
		_world.objects[vial].status &= 0xDF;
		seFill(vial, 0x38, 1);
	}
	int n = al.partySize - 1;
	if (n < 1)
		n = 4;
	_mdDreamFlag = n;
	// the offsets of the foes: the bytes of ds:00AE + 2 i and ds:00B6 + 2 i of GAME.EXE (i = 4 reads past the table: -1, -23)
	static const int DX[5] = {0, 1, 0, -1, -1}, DY[5] = {0, 0, 1, 0, -23};
	for (int i = n; i > 0; --i)
		mdMakeCreature(0x16C, qual + DX[i < 5 ? i : 4], qty + DY[i < 5 ? i : 4], 8);
	if (!_combatMode)
		toggleCombat();
}

// sub_507B0 (sub_3F664): a thing of the type (the quantity in the low byte of `word`, the quality in the high byte) made at x, y of level 2, not "temporary" (its bit 0x20 is cleared)
int Game::mdMakeThing(int type, int word, int mx, int my) {
	int o = seMake(type & 0x3FF, mx, my, 2, word & 0xFF, word >> 8);
	_world.objects[o].frame = (uint8_t)(type >> 10);
	_world.objects[o].status &= 0xDF;
	return o;
}

// sub_2652C as the scenes use it: a creature of the type (the frame in the high bits of the word) at the square with the default worktype 8, not temporary
int Game::mdMakeCreature(int word, int mx, int my, int worktype) {
	int a = _creatures.spawn(word & 0x3FF, worktype, mx, my, z);
	if (a < 0) {
		message("Aargh!\n");
		return -1;
	}
	int k = _world.actorObject(a);
	if (k >= 0) {
		_world.objects[k].frame = (uint8_t)(word >> 10);
		_world.objects[k].status &= 0xDF;
	}
	return a;
}

// sub_52159(), the scene of the square 5: actor 0x50 is put at 0xA5, 0xA4 of level 2 as a person with frame 9 and its flags reset; the 0x92 at 0xC9, 0x9B and the things 0xF, 0x28, 0x93, 0x96, 0x97,
// 0x14E-0x150 of the region x 0x76-0xCE, y 0x98-0xCC are taken away and put there anew (0x93, 0x28, 0xF, 0x96 and 0x97 with the qualities 1 and 8); the body's first psychic number is 2 and two
// creatures (0x193 at 0xB0, 0xA2 and a 0x17B with frame 15 at 0xA0, 0xA3) appear.
void Game::mdSceneFive() {
	ObjList &al = _world.actors;
	al.talkFlags[0x50] &= 0xDE;
	int p = _world.actorObject(0x50);
	if (p >= 0) {
		mdPlace(p, 0xA5, 0xA4, 2);
		_world.objects[p].frame = 9;
		_world.objects[p].status |= 0x80;
	}
	al.worktype[0x50] = 0;
	al.status[0x50] &= 0x9F;
	int d = seFind(0x92, 0xC9, 0x9B);
	if (d >= 0)
		_world.deleteObject(d);
	static const int GONE[8] = {0xF, 0x28, 0x93, 0x96, 0x97, 0x14E, 0x14F, 0x150};
	for (size_t i = 0x101; i < _world.objects.size(); ++i) {
		const World::Obj &o = _world.objects[i];
		if (o.deleted || !o.onMap() || o.z != z || o.x < 0x76 || o.x > 0xCE || o.y < 0x98 || o.y > 0xCC)
			continue;
		for (int t : GONE)
			if (o.type == t) {
				_world.deleteObject((int)i);
				break;
			}
	}
	mdMakeThing(0x93, 0, 0xC1, 0x9C);
	mdMakeThing(0x28, 0, 0xC1, 0x9B);
	mdMakeThing(0x0F, 0, 0xC7, 0x9A);
	mdMakeThing(0x96, 0x100, 0xC1, 0xA1);
	mdMakeThing(0x97, 0x800, 0xC1, 0xA1);
	mdSetPsychic(0, 0, 2);
	mdMakeCreature(0x193, 0xB0, 0xA2, 8);
	mdMakeCreature(0x3D7B, 0xA0, 0xA3, 8);
}

// sub_51944(), the scene of the square 0x44: actor 0x54 is put at 0x37, 0x3D of level 2 as a person with frame 9 and its flags reset; the things 0xA1, 0xA2, 0xD9, 0xDA and 0x18E of the
// region x 0x21-0x3D, y 0x33-0x43 are taken away; the maze is made anew: a 0xA1 at 0x24, 0x36, the hedges 0xD9 (quantity 3, a frame of 0-2 by a roll of 0-5 halved) in the columns 0x37, 0x39 and
// 0x3B of the rows 0x39-0x41 (not 0x37 in row 0x3E, 0x39 only in the rows 0x3B-0x41 but 0x40) and at 0x38 and 0x3A in the rows 0x39 and 0x41, and a creature 0x18E (worktype 0x16) at 0x3B, 0x41.
void Game::mdSceneMaze() {
	ObjList &al = _world.actors;
	al.talkFlags[0x54] &= 0xBE;
	int p = _world.actorObject(0x54);
	if (p >= 0) {
		mdPlace(p, 0x37, 0x3D, 2);
		_world.objects[p].frame = 9;
		_world.objects[p].status |= 0x80;
	}
	al.worktype[0x54] = 0;
	al.status[0x54] &= 0x9F;
	for (size_t i = 0x101; i < _world.objects.size(); ++i) {
		const World::Obj &o = _world.objects[i];
		if (o.deleted || !o.onMap() || o.z != z || o.x < 0x21 || o.x > 0x3D || o.y < 0x33 || o.y > 0x43)
			continue;
		if (o.type == 0xA1 || o.type == 0xA2 || o.type == 0xD9 || o.type == 0xDA || o.type == 0x18E)
			_world.deleteObject((int)i);
	}
	mdMakeThing(0xA1, 0, 0x24, 0x36);
	auto hedge = [&](int hx, int hy) { mdMakeThing(0xD9 | ((seRoll(0, 5) >> 1) << 10), 3, hx, hy); };
	for (int r = 0x39; r <= 0x41; ++r) {
		if (r != 0x3E)
			hedge(0x37, r);
		if (r > 0x3A && r != 0x40)
			hedge(0x39, r);
		hedge(0x3B, r);
	}
	hedge(0x38, 0x39);
	hedge(0x38, 0x41);
	hedge(0x3A, 0x39);
	hedge(0x3A, 0x41);
	int c = mdMakeCreature(0x18E, 0x3B, 0x41, 0x16);
	(void)c;
}

// sub_52843(), the scene of the square 0x60: the flags of actor 0x55 are cleared and it is put at 0x4E, 0x4D of level 2 as a person with frame 1; the things 0x6F, 0x70, 0x84, 0x1A9 and 0x1AA of
// the region x 0x49-0x57, y 0x4A-0x51 are taken away and the room is made anew: a 0x70 (quality 0xFF) in each corner, a 0x1A9 and a 0x84 (quantity 9) at 0x4D, 0x50, and the 0x84 stones whose
// quantities number them (di + v + 2 in the even columns 0x49-0x51 but 0x4D, 0xA in the odd ones 0x4A-0x50, and 0xB at the four squares 0x4A / 0x50 by 0x4B / 0x4D).
void Game::mdSceneSix() {
	ObjList &al = _world.actors;
	al.talkFlags[0x55] = 0;
	int p = _world.actorObject(0x55);
	if (p >= 0) {
		mdPlace(p, 0x4E, 0x4D, 2);
		_world.objects[p].frame = 1;
		_world.objects[p].status |= 0x80;
	}
	al.worktype[0x55] = 0;
	al.status[0x55] &= 0x9F;
	for (size_t i = 0x101; i < _world.objects.size(); ++i) {
		const World::Obj &o = _world.objects[i];
		if (o.deleted || !o.onMap() || o.z != z || o.x < 0x49 || o.x > 0x57 || o.y < 0x4A || o.y > 0x51)
			continue;
		if (o.type == 0x6F || o.type == 0x70 || o.type == 0x84 || o.type == 0x1A9 || o.type == 0x1AA)
			_world.deleteObject((int)i);
	}
	mdMakeThing(0x70, 0xFF00, 0x49, 0x4A);
	mdMakeThing(0x70, 0xFF00, 0x51, 0x4A);
	mdMakeThing(0x70, 0xFF00, 0x49, 0x51);
	mdMakeThing(0x70, 0xFF00, 0x51, 0x51);
	mdMakeThing(0x1A9, 0, 0x4D, 0x50);
	mdMakeThing(0x84, 9, 0x4D, 0x50);
	for (int di = 0; di <= 8; di += 2) {
		if (di == 4)
			continue;
		for (int v = 2; v <= 6; v += 2)
			mdMakeThing(0x84, di + v + 2, di + 0x49, v + 0x4A);
	}
	for (int di = 1; di <= 7; di += 2)
		for (int v = 5; v <= 7; v += 2)
			mdMakeThing(0x84, 0x0A, di + 0x49, v + 0x4A);
	mdMakeThing(0x84, 0x0B, 0x4A, 0x4B);
	mdMakeThing(0x84, 0x0B, 0x4A, 0x4D);
	mdMakeThing(0x84, 0x0B, 0x50, 0x4B);
	mdMakeThing(0x84, 0x0B, 0x50, 0x4D);
}

// sub_52491(qty, qual), the scene of the square 4 (the room at x = qual, y = qty): in the region x qual-3 .. qual+0x5C, y qty-8 .. qty+0x37 the things 0x78 are made invisible and the things
// 1, 3, 7, 0x2C, 0x4B, 0x58, 0x65, 0x7A, 0xB8, 0x100, 0x116, 0x12D, 0x14B, 0x181, 0x1A1 and 0x1A2 are taken away; nine things (0x1A1 and 0x1A2 with frames) make the little room at the square,
// actor 0x57 stands at qual, qty-2 as a person with frame 9 and its flags reset; and each 0x78 of the region holds what the table at ds:1F02 names for its quality: a 0x7A is made at its
// place, anything else (not 0) goes into a 0x14B made there (four of a 0x4B).
void Game::mdSceneFour(int qty, int qual) {
	ObjList &al = _world.actors;
	int x1 = qual - 3, y1 = qty - 8, x2 = x1 + 0x5F, y2 = y1 + 0x3F;
	static const int GONE[16] = {1, 3, 7, 0x2C, 0x4B, 0x58, 0x65, 0x7A, 0xB8, 0x100, 0x116, 0x12D, 0x14B, 0x181, 0x1A1, 0x1A2};
	for (size_t i = 0x101; i < _world.objects.size(); ++i) {
		const World::Obj &o = _world.objects[i];
		if (o.deleted || !o.onMap() || o.z != z || o.x < x1 || o.x >= x1 + 0x60 || o.y < y1 || o.y > y1 + 0x3F)
			continue;
		if (o.type == 0x78) {
			_world.objects[i].status |= 2;
			continue;
		}
		for (int t : GONE)
			if (o.type == t) {
				_world.deleteObject((int)i);
				break;
			}
	}
	(void)x2;
	(void)y2;
	mdMakeThing(0x1A1, 0x102, qual, qty - 2);
	mdMakeThing(0x5A1, 0x100, qual + 1, qty - 2);
	mdMakeThing(0x9A1, 0x100, qual + 2, qty - 2);
	mdMakeThing(0xDA1, 0x100, qual, qty - 1);
	mdMakeThing(0x15A1, 0x100, qual + 2, qty - 1);
	mdMakeThing(0x19A1, 0x101, qual, qty);
	mdMakeThing(0x1DA1, 0x100, qual + 1, qty);
	mdMakeThing(0x21A1, 0x100, qual + 2, qty);
	mdMakeThing(0x11A2, 0x109, qual + 1, qty - 1);
	al.talkFlags[0x57] = 0;
	int p = _world.actorObject(0x57);
	if (p >= 0) {
		mdPlace(p, qual, qty - 2, z);
		_world.objects[p].frame = 9;
		_world.objects[p].status |= 0x80;
	}
	al.worktype[0x57] = 0;
	al.status[0x57] &= 0x9F;
	// what the stored things hold: ds:1F02 + quality of GAME.EXE
	static const uint8_t HOLDS[16] = {0x00, 0x7A, 0x4B, 0x03, 0x07, 0xB8, 0x2C, 0x65, 0x2D, 0x16, 0x81, 0x00, 0x59, 0x6F, 0x75, 0x20};
	for (size_t i = 0x101; i < _world.objects.size(); ++i) {
		const World::Obj o = _world.objects[i];
		if (o.deleted || !o.onMap() || o.z != z || o.type != 0x78 || o.x < x1 || o.x >= x1 + 0x60 || o.y < y1 || o.y > y1 + 0x3F)
			continue;
		int held = HOLDS[o.quality & 15];
		if (held == 0x7A) {
			mdMakeThing(0x7A, 1, o.x, o.y);
		} else if (held) {
			int box = mdMakeThing(0x14B, 0, o.x, o.y);
			seFill(box, held, held == 0x4B ? 4 : 1);
		}
	}
}

// sub_50007: the dreamer wakes. The body goes back to its place at level 6, the members' psychic numbers are lost and the poisoned ones are left nearly dead (more than 30 hit points lose 24-30,
// more than 10 get 1-10, else 1), the clock reads eight in the morning of the day kept, the view goes back to the Avatar and the morning song plays.
void Game::mdDreamWake() {
	if (!_mdDreaming)
		return;
	ObjList &al = _world.actors;
	mdDreamLeave(); // sub_50218(0): what the dream's last kind of square leaves to clean up
	for (int w = 0; w < 3; ++w)
		mdSetPsychic(0, w, 0);
	al.status[0] &= 0x7F;
	gameClock.hour = 8;
	gameClock.minute = 0;
	gameClock.day = _mdWake[0];
	gameClock.month = _mdWake[1];
	gameClock.year = _mdWake[2];
	for (int m = 0; m < al.partySize; ++m) {
		int s = al.partyRoster[m];
		for (int w = 0; w < 3; ++w)
			mdSetPsychic(s, w, 0);
		if (!(al.status[s] & 8))
			continue;
		if (al.hp[s] > 0x1E)
			al.hp[s] = (uint8_t)(al.hp[s] - seRoll(0x18, 0x1E));
		else if (al.hp[s] > 0x0A)
			al.hp[s] = (uint8_t)seRoll(1, 0x0A);
		else
			al.hp[s] = 1;
	}
	int k = _world.actorObject(0);
	if (k >= 0)
		mdPlace(k, _world.objects[_world.actorObject(1)].x, _world.objects[_world.actorObject(1)].y, 6); // parked at level 6 (the camera's square, sub_2CFE2(6, x, y, 0))
	al.worktype[0] = 0;
	al.worktype[1] = 2;
	_mdDreaming = false;
	_mdDreamId = 0;
	_solo = -1;
	if (al.partySize < ObjList::PARTY_MAX)
		al.partyRoster[al.partySize] = 0;
	setControl(0);
	message("You wake up.\n");
	_mdMorning = true;
	chooseMusic();
}

// sub_50651(actor): a scene of the dream is over. Actor 0x5C goes away (to level 6, hidden) and 0x53 takes its place; the actor is put on the square where its schedule starts and the things of
// the scene there (a 0x1CD, a 0x124 on the square above) are taken away; the actor's mark 0x80 is set (the array ds:6EC6, not done), and when the marks of the four (0x52, 0x54, 0x55, 0x56) or
// the eight (0x50-0x57) scenes are all set, a story flag of actor 0x60 / 0x20 is.
void Game::mdDreamEnd(int actor) {
	if (!_mdDreaming)
		return;
	ObjList &al = _world.actors;
	if (actor == 0x5C) {
		int k = _world.actorObject(0x5C);
		if (k >= 0) {
			mdPlace(k, 0x5C, 0x5C, 6);
			_world.objects[k].status |= 0x80;
		}
		al.worktype[0x5C] = 0;
		actor = 0x53;
	}
	Schedule::Entry e = _world.schedule.entry(_world.schedule.index[actor]);
	int px = e.x & 0x3FF, py = e.y & 0x3FF;
	int ak = _world.actorObject(actor);
	if (ak >= 0)
		mdPlace(ak, px, py, z);
	int a = seFind(0x1CD, px, py);
	if (a >= 0)
		_world.deleteObject(a);
	else
		message("Aargh!\n");
	int b = seFind(0x124, px, (py - 1) & 0x3FF);
	if (b >= 0)
		_world.deleteObject(b);
	else
		message("Aargh!\n");
	auto &t = al.talkFlags;
	if ((t[0x56] & 0x80) && (t[0x54] & 0x80) && (t[0x52] & 0x80) && (t[0x55] & 0x80))
		t[0x60] |= 0x10;
	if ((t[0x50] & 0x80) && (t[0x57] & 0x80) && (t[0x56] & 0x80) && (t[0x54] & 0x80) && (t[0x55] & 0x80) && (t[0x52] & 0x80) && (t[0x51] & 0x80) && (t[0x53] & 0x80))
		t[0x20] |= 0x10;
}

// sub_3F615 and sub_3F61A: the scene of the actor is over and the dreamer wakes
void Game::mdDreamFinish(int actor) {
	if (!_mdDreaming)
		return;
	mdDreamEnd(actor);
	mdDreamWake();
}

// sub_55F77's switch over the exit code a conversation left (word_41F2A): the codes 0x34 and 0x36 (a scene of the dream is over) wake the dreamer after the end of actor 0x55's scene, 0x38 does it
// when its flags say the scene has been played (bit 8 set, bit 4 not); the other codes (9, 0xA, 0x65-0x69: the scenes that put things on the maps) are not done.
void Game::mdExitCode(int code) {
	ObjList &al = _world.actors;
	if (code == 0x34 || code == 0x36 || (code == 0x38 && (al.talkFlags[0x55] & 0x0C) == 8))
		mdDreamFinish(0x55);
}

// sub_25C46 with the dream's own flavour: the body goes to the square (qual, qty) of the same level (the quality is x and the quantity y), with the sound 0x33 for the frame 1. Not dreaming,
// the whole party goes (sub_25B56)
void Game::mdDreamGo(int frame, int qual, int qty) {
	if (frame == 1)
		mdSfx(0x33);
	if (_mdDreaming) {
		if (_leaderObject >= 0) {
			_world.moveObject(_leaderObject, qual, qty, z);
			x = qual;
			y = qty;
		}
	} else {
		changeLevel(qual, qty, z);
	}
}

// sub_50218(object): the dreamer stepped on a square with a 0x1CD that has flags
void Game::mdDreamTouch(int obj) {
	ObjList &al = _world.actors;
	World::Obj o = _world.objects[obj];
	int kind = o.status & 0xE5, qual = o.quality, qty = o.quantity, frame = o.frame;
	auto &t = al.talkFlags;
	if (_mdDreamId && kind != 0xA5 && kind != 0xE5)
		mdDreamLeave();
	auto removeScene = [&]() { // the picture above and the square itself go
		int p = seFind(0x124, o.x, (o.y - 1) & 0x3FF);
		if (p >= 0)
			_world.deleteObject(p);
		_world.deleteObject(obj);
	};
	switch (kind) {
	case 0:
		break;
	case 1:
		_mdDreamId = 0;
		return;
	case 0x24:
		mdDreamWake();
		return;
	case 4: // sub_52491, before the body goes on
		mdSceneFour(qty, qual);
		break;
	case 0xA4: { // actor 0x56 appears at 0x19, 0x6F of level 2 as a plain person
		t[0x56] &= 1;
		int k = _world.actorObject(0x56);
		if (k >= 0) {
			mdPlace(k, 0x19, 0x6F, 2);
			_world.objects[k].frame = 1;
			_world.objects[k].status |= 0x80;
		}
		al.worktype[0x56] = 0;
		al.status[0x56] &= 0x9F;
		break;
	}
	case 0xC1: { // the scene is done: half the time the persons 0x6D and 0x67 trade places and looks
		t[0x6B] |= 4;
		removeScene();
		if (seRoll(1, 100) <= 50) {
			int a = _world.actorObject(0x67), b = _world.actorObject(0x6D);
			if (a >= 0 && b >= 0) {
				int ax = _world.objects[a].x, ay = _world.objects[a].y, bx = _world.objects[b].x, by = _world.objects[b].y;
				mdPlace(b, ax, ay, z);
				mdPlace(a, bx, by, z);
				std::swap(_world.objects[a].frame, _world.objects[b].frame);
			}
		}
		break;
	}
	case 0x40: { // sub_5189D: "You fall off the cliff!", the body lies there (type 0x126, or 0x127 for a woman) and the first child (0x187) of the rectangle x-1..x+1, y-8..y-4 is killed
		message("You fall off the cliff!\n");
		int k = _world.actorObject(0);
		if (k >= 0)
			_world.objects[k].type = mdMale(1) ? 0x126 : 0x127;
		mdEach(x - 1, y - 8, x + 1, y - 4, [this](int i) {
			if (_world.objects[i].type != 0x187)
				return false;
			int a = _world.objects[i].actor;
			if (a >= 0)
				_world.actors.status[a] |= 0x10;
			_world.deleteObject(i);
			return true;
		});
		break;
	}
	case 0xC4:
		break;
	case 0xC5:
		t[0x6A] |= 4;
		removeScene();
		break;
	case 0xE0:
		t[0x6C] |= 4;
		removeScene();
		break;
	case 0xC0:
		al.hp[0] = (uint8_t)ActorStats::maxHealth(_world, 1);
		break;
	default: // 0x85, 0xA1: the scenes of sub_52B0C, sub_51C96 (APPROX(95)); the square only leads on
		break;
	}
	if (kind != 0xA5)
		_mdDreamFlag = 0;
	for (int w = 0; w < 3; ++w)
		mdSetPsychic(0, w, 0);
	mdDreamGo(frame, qual, qty);
	switch (kind) {
	case 0xC5:
		t[0x68] = 0;
		break;
	case 0xC0:
		al.hp[0] = (uint8_t)ActorStats::maxHealth(_world, 1);
		break;
	case 5:
		mdSceneFive();
		break;
	case 0x44:
		mdSceneMaze();
		break;
	case 0x60:
		mdSceneSix();
		break;
	case 0x20:
		mdSceneBattle(qty, qual);
		break;
	case 0x25: // sub_5182E: a child (0x187) stands on the square north of the one the body came to, as a person on the move (worktype 0x15)
		mdMakeCreature(0x187, qual, qty - 1, 0x15);
		break;
	case 0xE5:
		if (!(t[0x66] & 8))
			_mdDreamFlag = 6;
		return; // (the dream number is not changed)
	default:
		break;
	}
	_mdDreamId = kind;
}

// sub_27EE6, the part for the squares of the dream: the first 0x1CD on the square the body entered leads on (to y = quantity, x = quality) and does what its flags say
void Game::mdStepOn() {
	std::vector<int> here;
	_world.objectsAt(x, y, z, here);
	for (int i : here) {
		const World::Obj &o = _world.objects[i];
		if (o.deleted || o.type != 0x1CD || i == _leaderObject)
			continue;
		int kind = o.status & 0xE5;
		if (kind == 0xA5 && _world.actors.hp[0] <= 4)
			return;
		if (kind)
			mdDreamTouch(i);
		else
			mdDreamGo(o.frame, o.quality, o.quantity);
		return;
	}
}

// sub_5BC53(cabinet): the dream machine of the room (the 0x1C9 cabinet's panel works it). The things of the room within five squares: the chair 0x122 with frame 1, the pod 0x121 (the square
// where the sitter is), the control 0x120 (its quality is the machine's), the 0x64. Without them "The machine does not work."; a thing 0x123 on the pod starts the scene of actor 0x21 and a
// 0x11F that of actor 6; with nobody on the pod "There is nobody sitting in the machine."; when the dreamer is in the dream (level 2 or 3 is the camera's, not the surface) the machine wakes
// it, else it starts the dream of the sitter.
void Game::mdDreamMachine(int cabinet) {
	ObjList &al = _world.actors;
	if (al.talkFlags[0x46] & 8) {
		message("The dream machines seem to have ceased functioning.\n");
		return;
	}
	const World::Obj &c = _world.objects[cabinet];
	int mask = z == 0 ? 0x3FF : 0xFF;
	int chair = -1, other = -1, quality = -1, podX = -1, podY = -1;
	for (size_t i = 0; i < _world.objects.size(); ++i) {
		const World::Obj &o = _world.objects[i];
		if (o.deleted || !o.onMap() || o.z != z)
			continue;
		if (((o.x - c.x + 5) & mask) > 10 || ((o.y - c.y + 5) & mask) > 10)
			continue;
		if (o.type == 0x122) {
			if (o.frame == 1)
				chair = (int)i;
		} else if (o.type == 0x121) {
			podX = o.x;
			podY = o.y;
		} else if (o.type == 0x64) {
			other = (int)i;
		} else if (o.type == 0x120) {
			quality = o.quality;
		}
	}
	// sub_2C111(1, x, y): the first living actor on the pod's square
	int sitter = -1;
	for (int a = 1; a < ObjList::ACTORS && podX >= 0; ++a) {
		int k = _world.actorObject(a);
		if (k < 0 || _world.objects[k].type == 0 || _world.objects[k].deleted || (al.status[a] & 0x10))
			continue;
		const World::Obj &o = _world.objects[k];
		if (o.x == podX && o.y == podY && o.z == z) {
			sitter = a;
			break;
		}
	}
	if (chair < 0 || quality < 0 || other < 0) {
		message("The machine does not work.\n");
		return;
	}
	int thing = seFind(0x123, podX, podY);
	if (thing >= 0) { // the scene of actor 0x21
		if ((al.talkFlags[0x21] & 4) || !(al.talkFlags[0x61] & 0x10)) {
			message("It has no effect.\n");
			return;
		}
		startTalk(0x21);
		_talkEnd = [this, thing, podX, podY]() {
			ObjList &a = _world.actors;
			a.talkFlags[0x21] |= 4;
			int k = _world.actorObject(0x61);
			if (k >= 0)
				mdPlace(k, 0x61, 0x61, 6);
			a.worktype[0x61] = 0;
			if (k >= 0)
				_world.objects[k].status |= 0xCB;
			int h = _world.actorObject(0x21);
			if (h >= 0)
				mdPlace(h, podX, (podY + 1) & 0x3FF, z);
			// sub_19300(0x21) (the actor is killed: its death is APPROX(95): it is left dead where it stands)
			a.hp[0x21] = 0;
			a.status[0x21] |= 0x10;
			_world.deleteObject(thing);
		};
		return;
	}
	thing = seFind(0x11F, podX, podY);
	if (thing >= 0) { // the scene of actor 6
		if (!_world.objects[thing].quality) {
			message("It has no effect.\n");
			return;
		}
		startTalk(6);
		_talkEnd = [this, thing, podX, podY]() {
			ObjList &a = _world.actors;
			int h = _world.actorObject(6);
			if (h >= 0)
				mdPlace(h, podX, (podY + 1) & 0x3FF, z);
			int k = _world.actorObject(0x63);
			if (k >= 0)
				mdPlace(k, 0x63, 0x63, 6);
			a.worktype[0x63] = 0;
			if (k >= 0)
				_world.objects[k].status |= 0xCB;
			_world.deleteObject(thing);
		};
		return;
	}
	if (sitter < 0) {
		message("There is nobody sitting in the machine.\n");
		return;
	}
	if (z != 0) { // (the camera is in the dream: the machine wakes the dreamer)
		mdDreamWake();
		return;
	}
	if (quality < 2)
		al.talkFlags[0x60] |= 8;
	if (quality != 3) {
		mdDreamStart(quality, sitter);
		return;
	}
	if (al.talkFlags[0x46] & 1) {
		mdDreamStartLevel3();
		return;
	}
	message("The machine does not work.\n");
}

} // namespace Ultima6
} // namespace Ultima6

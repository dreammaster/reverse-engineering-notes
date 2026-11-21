// Martian Dreams' levers that work the story's machinery (GAME.EXE overlay seg057): the drawbridge of the canal (sub_4F5A0, sub_4F7D7, sub_4F926, a lever of quality 0xA), the power panel of the
// surface and the underground (sub_5328B, quality 0x14) and the lever that turns the world round or starts a dream (sub_53496, quality 0x15). The levers of qualities 1 and 2 are in md_use.cpp.
//
// APPROX(96): the scene routines the power and canal levers call (sub_3F740, sub_3F74A, sub_3F793, sub_3F745, sub_3F798, sub_3F7A7, sub_3F7AC, sub_3F540, sub_3F7B1: lamps, lights and the
// canal's water) only change the story flags here; the sounds of the drawbridge's movement (sub_36E34 with 0x49F0) and the redraws are left out; the turning of the world (word_4115E, a rotation of
// the map by that many eighths of a turn that the map drawing and the steps use) is kept as a number but the map is not turned.

#include "ultima6/session.h"

#include <algorithm>

namespace Ultima6 {
namespace Ultima6 {

// sub_4F5A0(lever): the drawbridge. It needs the power of the surface (bit 0x10 of the flags of actor 0x73); the two levers (0xC4, quality 0xA) and up to five anchors (0xC3 with frame 0) that
// have the lever's number (quantity / quality) are found; the anchor's quantity gives the direction (bit 0x80: along y) and the length (the low 5 bits); the span is the box beside the anchors
// by that length: with anything but 0xC3 in it "The bridge doesn't move."; with 0xC3 in it the bridge is out and is taken in (the pieces with a frame go), else the pieces are made (frame 3 or
// 2 at the far end, 1 elsewhere) and the levers' frames are 1 - (the bridge was out).
void Game::mdLeverBridge(int lever) {
	ObjList &al = _world.actors;
	if (!(al.talkFlags[0x73] & 0x10)) {
		message("There doesn't seem to be any power for the machinery.\n");
		mdSfx(5);
		return;
	}
	int number = _world.objects[lever].quantity;
	int minX = 0x3FF, maxX = 0, minY = 0x3FF, maxY = 0;
	std::vector<int> levers, pieces;
	for (size_t i = 0x101; i < _world.objects.size(); ++i) {
		const World::Obj &o = _world.objects[i];
		if (o.deleted || !o.onMap())
			continue;
		if (o.type == 0xC4 && o.quality == 0xA && o.quantity == number && levers.size() < 2) {
			levers.push_back((int)i);
		} else if (o.quality == number && o.type == 0xC3 && o.frame == 0 && pieces.size() < 5) {
			pieces.push_back((int)i);
			minX = std::min<int>(minX, o.x);
			maxX = std::max<int>(maxX, o.x);
			minY = std::min<int>(minY, o.y);
			maxY = std::max<int>(maxY, o.y);
		}
	}
	if (levers.empty() || pieces.empty()) {
		message("Argh--drawbridge data inconsistent!\n");
		return;
	}
	int q = _world.objects[pieces[0]].quantity;
	bool alongY = (q & 0x80) != 0;
	int length = q & 0x1F;
	if (alongY) {
		minY -= length;
		--maxY;
	} else {
		minX -= length;
		--maxX;
	}
	bool out = false, blocked = false;
	mdEach(minX, minY, maxX, maxY, [&](int i) {
		if (_world.objects[i].type == 0xC3) {
			out = true;
			return false;
		}
		blocked = true;
		return true;
	});
	if (blocked) {
		message("The bridge doesn't move.\n");
		mdSfx(5);
		return;
	}
	for (int l : levers)
		_world.objects[l].frame = (uint8_t)(1 - (out ? 1 : 0));
	mdSfx(0x12);
	auto clearSquare = [&](int sx, int sy) {
		std::vector<int> here;
		_world.objectsAt(sx, sy, z, here);
		for (int i : here)
			if (_world.objects[i].type == 0xC3 && _world.objects[i].frame != 0)
				_world.deleteObject(i);
	};
	if (out) { // taken in: the row (column) by the anchors first, then the others
		if (alongY) {
			for (int sx = minX; sx <= maxX; ++sx)
				clearSquare(sx, minY);
			for (int sy = minY + 1; sy <= maxY; ++sy)
				for (int sx = minX; sx <= maxX; ++sx)
					clearSquare(sx, sy);
		} else {
			for (int sy = minY; sy <= maxY; ++sy)
				clearSquare(minX, sy);
			for (int sx = minX + 1; sx <= maxX; ++sx)
				for (int sy = minY; sy <= maxY; ++sy)
					clearSquare(sx, sy);
		}
		message("The drawbridge retracts into its slot.\n");
		return;
	}
	auto make = [&](int sx, int sy, int frame) {
		int o = seMake(0xC3, sx, sy, z, 0, number);
		_world.objects[o].status &= 0xDF;
		_world.objects[o].frame = (uint8_t)frame;
	};
	if (alongY) {
		for (int sy = maxY; sy >= minY; --sy)
			for (int sx = maxX; sx >= minX; --sx)
				make(sx, sy, sx == maxX ? 3 : 1);
	} else {
		for (int sx = maxX; sx >= minX; --sx)
			for (int sy = maxY; sy >= minY; --sy)
				make(sx, sy, sy == maxY ? 2 : 1);
	}
	message("The drawbridge slides out to cross the canal.\n");
}

// sub_53550 (the underground power on) and sub_53760 (off): the machines of every level change: on, a 0x9A gets the next frame, a 0xC9 the bit 4 of its frame, a 0xD6 becomes 0xD7, a 0xE9 of frame 1
// or 3 goes 4 on, and a 0xD0 gets a 0xD1 (frame 1) on the square below unless one is there; off puts it all back (0x9A to frame 0, 0xC9 frame without the bit 4, 0xD7 to 0xD6, 0xE9 frame 5
// or 7 goes 4 back, the 0xD1 are taken away). APPROX(96): the marks the original makes for the members of the party in some regions (the array at ds:BC8E) are left out.
void Game::mdPowerUnderground(bool on) {
	for (size_t i = 0x101; i < _world.objects.size(); ++i) {
		World::Obj &o = _world.objects[i];
		if (o.deleted)
			continue;
		switch (o.type) {
		case 0x9A:
			o.frame = on ? o.frame + 1 : 0;
			break;
		case 0xC9:
			o.frame = on ? (o.frame | 4) : (o.frame & 0xFB);
			break;
		case 0xD6:
			if (on)
				o.type = 0xD7;
			break;
		case 0xD7:
			if (!on)
				o.type = 0xD6;
			break;
		case 0xD0:
			if (on && o.onMap()) {
				int below = (o.y + 1) & 0x3FF;
				std::vector<int> here;
				_world.objectsAt(o.x, below, o.z, here);
				bool have = false;
				for (int h : here)
					have |= _world.objects[h].type == 0xD1;
				if (!have) {
					int n = seMake(0xD1, o.x, below, o.z, 0, 0);
					_world.objects[n].frame = 1;
					_world.objects[n].status &= 0xDF;
				}
			}
			break;
		case 0xD1:
			if (!on)
				_world.deleteObject((int)i);
			break;
		case 0xE9:
			if (on && (o.frame == 1 || o.frame == 3))
				o.frame += 4;
			else if (!on && (o.frame == 5 || o.frame == 7))
				o.frame -= 4;
			break;
		default:
			break;
		}
	}
}

// sub_53D8D: the part of the surface at x 0x3D0-0x3F0, y 0x1D0-0x1E7 (level 0): a 0xC9 gets the bit 4 of its frame, a 0xD6 becomes 0xD7 (the lamp panel sub_53960 is left out)
void Game::mdPowerSurface() {
	mdEach(0x3D0, 0x1D0, 0x3F0, 0x1E7, [this](int i) {
		World::Obj &o = _world.objects[i];
		if (o.type == 0xC9)
			o.frame |= 4;
		else if (o.type == 0xD6)
			o.type = 0xD7;
		return false;
	}, 0);
}

// sub_53E92: the scene is put back: the 0xD7 of the region x 0x3D0-0x3F0, y 0x1D0-0x1E7 become 0xD6 again and the things 0x13B (the machine guns) of the four regions round the towers (x 0xC8-0xD7,
// y 0x290-0x2A0; 0x348-0x357, 0x290-0x2A0; 0x138-0x147, 0x178-0x187; 0x2C8-0x2D7, 0x1F8-0x207) are taken away
void Game::mdPowerReset() {
	mdEach(0x3D0, 0x1D0, 0x3F0, 0x1E7, [this](int i) {
		if (_world.objects[i].type == 0xD7)
			_world.objects[i].type = 0xD6;
		return false;
	}, 0);
	static const int RECTS[4][4] = {{0xC8, 0x290, 0xD7, 0x2A0}, {0x348, 0x290, 0x357, 0x2A0}, {0x138, 0x178, 0x147, 0x187}, {0x2C8, 0x1F8, 0x2D7, 0x207}};
	for (const auto &r : RECTS) {
		std::vector<int> gone;
		mdEach(r[0], r[1], r[2], r[3], [&](int i) {
			if (_world.objects[i].type == 0x13B)
				gone.push_back(i);
			return false;
		}, 0);
		for (int i : gone)
			_world.deleteObject(i);
	}
}

// sub_538EE: by night (before six and after 17 o'clock) every street lamp (0xE4) goes seven frames on
void Game::mdLamps() {
	bool night = gameClock.hour < 6 || gameClock.hour > 0x11;
	if (!night)
		return;
	for (size_t i = 0x101; i < _world.objects.size(); ++i)
		if (!_world.objects[i].deleted && _world.objects[i].type == 0xE4)
			_world.objects[i].frame += 7;
}

// sub_4F323: "The coal begins moving down the belt.", the power underground is on (the flags of actors 0x73 bit 4 and 0x71 bit 8)
void Game::mdCoalBelt() {
	message("The coal begins moving down the belt.\n");
	_world.actors.talkFlags[0x73] |= 4;
	_world.actors.talkFlags[0x71] |= 8;
	mdPowerUnderground(true);
}

// sub_5415A (the lenses aligned): the 0xD2 of frame 0 and 1 in the region x 0x13B-0x140, y 0x116-0x11A of the surface go
void Game::mdLensScene() {
	std::vector<int> gone;
	mdEach(0x13B, 0x116, 0x140, 0x11A, [&](int i) {
		if (_world.objects[i].type == 0xD2 && _world.objects[i].frame < 2)
			gone.push_back(i);
		return false;
	}, 0);
	for (int i : gone)
		_world.deleteObject(i);
}

// sub_542B1 (the canals work): the 0x1CB of frame 1 of the region round 0x241, 0x27B (the original takes the 40 x 40 window of the map the view has loaded, ds:BBAA / BBAC, which is not kept here:
// x 0x22E-0x255, y 0x268-0x28F is used) get frame 3, and a 0x1CC is made at 0x243, 0x27A
void Game::mdCanalScene() {
	mdEach(0x22E, 0x268, 0x255, 0x28F, [this](int i) {
		if (_world.objects[i].type == 0x1CB && _world.objects[i].frame == 1)
			_world.objects[i].frame = 3;
		return false;
	}, 0);
	int o = seMake(0x1CC, 0x243, 0x27A, 0, 0, 0);
	_world.objects[o].status &= 0xDF;
}

// sub_543C4 (Chsheket treated): the actors 0x41, 0x43, 0x44 and 0x45 die, 0x42 stands at 0x394, 0x284 and 0x46 at 0x393, 0x282 of the surface, 0x40 goes to level 6; all are people the story
// has hidden (the bit 0x80 of their status, worktype 0)
void Game::mdChsheketScene() {
	ObjList &al = _world.actors;
	for (int a : {0x41, 0x43, 0x44, 0x45}) {
		al.hp[a] = 0;
		_combat.kill(a);
	}
	auto put = [&](int a, int px, int py, int pz) {
		int k = _world.actorObject(a);
		if (k >= 0) {
			_world.objects[k].status |= 0x80;
			mdPlace(k, px, py, pz);
		}
		al.worktype[a] = 0;
	};
	put(0x42, 0x394, 0x284, 0);
	put(0x40, 0x40, 0x40, 6);
	put(0x46, 0x393, 0x282, 0);
}

// sub_5328B(lever): the power lever of the story. The flags are those of actors 0x73 (4: the power underground, 0x10: on the surface), 0x71 (8), 0x76 (0x20: the ruby lenses aligned, 0x40: the
// canals working), 0x2E, 0x30 and 0x39 (0x80) and 0x06 (2: Chsheket treated); the number of the lever (its quantity) picks what it does: 1 the power underground, 2 the surface, 3 the lenses,
// 4 the canals, 5 the underground power cut off, 6 treats Chsheket (his type 0x165 becomes 0x162). The lever turns over (frame ^ 1) and the sound 0x11 plays, except after the refusals of 2
// and 5 and the default.
void Game::mdLeverPanel(int lever) {
	ObjList &al = _world.actors;
	auto &t = al.talkFlags;
	World::Obj &o = _world.objects[lever];
	bool turn = true;
	switch (o.quantity) {
	case 1:
		if (t[0x73] & 4) {
			if (t[0x71] & 8) {
				t[0x73] &= 0xFB;
				t[0x71] &= 0xF7;
				mdPowerUnderground(false);
				t[0x73] |= 4;
				mdPowerUnderground(true);
				mdPowerSurface();
			} else {
				message("Sorry, boss, the power is on for good.\n");
			}
		} else {
			t[0x73] |= 4;
			mdPowerUnderground(true);
			mdPowerSurface();
		}
		break;
	case 2:
		if (!(t[0x73] & 4) || (t[0x71] & 8)) {
			message("You can't turn the surface power on until the underground power is permanently on.\n");
			turn = false;
		} else if (t[0x73] & 0x10) {
			message("Sorry, boss.  The surface power can't be turned off.\n");
			turn = false;
		} else {
			t[0x73] |= 0x10;
			mdLamps();
			mdPowerReset();
		}
		break;
	case 3:
		if (t[0x76] & 0x20) {
			message("The ruby lenses are already aligned.\n");
		} else {
			t[0x2E] |= 0x80;
			t[0x39] |= 0x80;
			t[0x30] |= 0x80;
			t[0x76] |= 0x20;
			mdLensScene();
		}
		break;
	case 4:
		if (t[0x76] & 0x40)
			message("The canals are already working.\n");
		else if (!(t[0x76] & 0x20))
			message("There is no water pressure in the valve system.\n");
		else {
			t[0x76] |= 0x40;
			mdCanalScene();
		}
		break;
	case 5:
		if (t[0x73] & 4) {
			if (t[0x71] & 8) {
				message("Power prematurely cut off.\n");
				t[0x73] &= 0xFB;
				t[0x71] &= 0xF7;
				mdPowerUnderground(false);
			} else {
				message("Power is already permanently on.\n");
				turn = false;
			}
		} else {
			mdCoalBelt();
		}
		break;
	case 6: {
		int k = _world.actorObject(6);
		if (k >= 0 && _world.objects[k].type == 0x165) {
			_world.objects[k].type = 0x162;
			t[6] |= 2;
			mdChsheketScene();
		} else {
			message("Chsheket has already been treated.\n");
		}
		break;
	}
	default:
		message("The lever doesn't do anything!\n");
		turn = false;
		break;
	}
	if (turn) {
		o.frame ^= 1;
		mdSfx(0x11);
	}
}

// sub_53496(lever): by the lever's quantity 1 turns the world round (4 eighths of a turn) or back, 2 starts the dream of whoever is in control (the quality 0 machine), 3 the dream of level 3
void Game::mdLeverWorld(int lever) {
	World::Obj &o = _world.objects[lever];
	switch (o.quantity) {
	case 1:
		if (_mdTurn) {
			message("The world returns to normal.\n");
			_mdTurn = 0;
			o.frame = 0;
		} else {
			message("You feel a strange disorientation.\n");
			_mdTurn = 4;
			o.frame = 1;
		}
		mdSfx(0x11);
		break;
	case 2:
		mdDreamStart(0, _world.actors.partyRoster[_member < ObjList::PARTY_MAX ? _member : 0]);
		break;
	case 3:
		mdDreamStartLevel3();
		break;
	default:
		message("Strangely, the lever does nothing at all!\n");
		break;
	}
}

} // namespace Ultima6
} // namespace Ultima6

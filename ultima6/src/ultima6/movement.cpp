#include "ultima6/movement.h"

namespace Ultima6 {
namespace Ultima6 {

#include "ultima6/movement_tables.inc"

int MovementClasses::classOf(int type) {
	for (int i = 0; i < COUNT; ++i)
		if (CLASS_TYPES[i] == type)
			return i;
	return -1;
}

uint16_t MovementClasses::flags(int cls) {
	return cls < 0 ? (uint16_t)UNLISTED_FLAGS : CLASS_FLAGS[cls];
}

void Movement::touching(int x, int y, int z, std::vector<Touch> &out) const {
	out.clear();
	int mask = z == 0 ? 0x3FF : 0xFF;
	x &= mask;
	y &= mask;
	std::vector<int> here;
	// Objects on the cell itself, then the parts that reach in from the cell to the east (double width),
	// to the south (double height) and to the south-east (both), as sub_199FF finds them.
	static const int DX[4] = {0, 1, 0, 1}, DY[4] = {0, 0, 1, 1};
	for (int k = 0; k < 4; ++k) {
		_world.objectsAt((x + DX[k]) & mask, (y + DY[k]) & mask, z, here);
		for (int idx : here) {
			const World::Obj &o = _world.objects[idx];
			int f2 = _world.flags.flags2[(_world.baseTile.firstTile[o.type] + o.frame) & (TileFlags::TILES - 1)];
			int part = 0;
			switch (k) {
			case 0:
				break;
			case 1:
				if (!(f2 & 0x80))
					continue;
				part = 1;
				break;
			case 2:
				if (!(f2 & 0x40))
					continue;
				part = (f2 & 0x80) ? 2 : 1;
				break;
			default:
				if ((f2 & 0xC0) != 0xC0)
					continue;
				part = 3;
				break;
			}
			out.push_back({idx, part});
		}
	}
}

MoveResult Movement::check(int x, int y, int z, int mover, const MoveRules &rules) const {
	const TileFlags &tf = _world.flags;
	const BaseTile &bt = _world.baseTile;
	const std::vector<World::Obj> &objs = _world.objects;
	const World::Obj &me = objs[mover];
	const int T = TileFlags::TILES - 1;

	MoveResult res;
	int mask = z == 0 ? 0x3FF : 0xFF;
	x &= mask;
	y &= mask;

	int moverTile = (bt.firstTile[me.type] + me.frame) & T;
	bool bigMover = (tf.flags2[moverTile] & 0xC0) == 0xC0;
	int type = me.type;
	int cls = MovementClasses::classOf(type);
	int cflags = MovementClasses::flags(cls);
	bool fly = cflags & MovementClasses::FLIES;
	bool tail = type == 0x19E || type == 0x19F;
	bool water = (cflags & MovementClasses::SWIMS) || tail;
	bool ghost = cflags & MovementClasses::IGNORES_OBJECTS;
	bool walker = (!water && !fly) || cls == 1;
	bool inParty = me.status & 0x80;
	bool moverIsActor = me.actor >= 0;

	// --- terrain ---
	int f = tf.flags1[_world.map.tile(z, x, y) & T];
	bool ok = false;
	if (type == 0x168) {
		ok = f & 0x08;
	} else {
		if (walker && !(f & 0x02))
			ok = true;
		if (water && (f & 0x02) && (f & 0x01))
			ok = true;
		if (tail && (f & 0x01))
			ok = true;
		if (fly && !(f & 0x04))
			ok = true;
		if (ghost)
			ok = true;
		if (f & 0x08) {
			res.damaging = true;
			if (rules.avoidDamage)
				return res;
		}
	}

	// --- objects ---
	std::vector<Touch> list;
	touching(x, y, z, list);
	bool more = true;
	for (const Touch &t : list) {
		if (!more)
			break;
		if (t.object == mover)
			continue;
		const World::Obj &o = objs[t.object];
		int di = o.type;
		bool isActor = o.actor >= 0;
		int si = isActor ? o.actor : 0x100;

		enum { SKIP, FALL, BLOCK } verdict = FALL;
		int f1 = 0; // var_1A: flags of the object's tile, set below (or of the mover's, for segment checks)

		if (di == 0x1A0 && !rules.questFlag) {
			// The guard of a Sacred Quest: "Thou art not upon a Sacred Quest! Passage denied!"
			res.denied = rules.deniedPending;
			res.passable = false;
			return res;
		}

		bool segmentSkip = false;
		if ((type >= 0x1AA || type == 0x19C) && !isActor && di == type) {
			// Segments of one long creature: the parts behind the head do not block the way they point away from.
			int mf = tf.flags1[moverTile];
			if ((mf & 0x80) && me.y > y)
				segmentSkip = true;
			else if ((mf & 0x40) && me.x < x)
				segmentSkip = true;
			else if ((mf & 0x20) && me.y < y)
				segmentSkip = true;
			else if ((mf & 0x10) && me.x > x)
				segmentSkip = true;
		}
		if (segmentSkip)
			verdict = SKIP;
		else if (type == 0x162 && (di == 0xD5 || di == 0x11A || di == 0x136)) {
			ok = true;
			verdict = SKIP;
		} else {
			int eff = (bt.firstTile[di] + o.frame - t.part) & T;
			if (bigMover && t.part == 0 && (tf.flags2[eff] & 0xC0))
				return res; // a two-by-two mover cannot start over a big object
			if (walker && (tf.flags3[eff] & 0x04)) {
				ok = true;
				if (!(tf.flags3[eff] & 0x10)) {
					res.passable = true;
					return res;
				}
			}
			f1 = tf.flags1[eff];
			if (f1 & 0x08) {
				res.damaging = true;
				if (rules.avoidDamage)
					return res;
			}

			bool cont = true;
			if (type == 0x19D && di == 0x19D) {
				verdict = (o.quality == me.quality + 1) ? BLOCK : FALL;
			} else if (walker && !(f1 & 0x02)) {
				verdict = FALL;
			} else {
				verdict = BLOCK; // decided below unless something lets the mover through
				if (water) {
					if (((f1 & 0x02) && (f1 & 0x01)) || (tf.flags3[eff] & 0x10))
						verdict = FALL;
				}
				if (verdict == BLOCK && tail) {
					if ((f1 & 0x01) || (tf.flags3[eff] & 0x10))
						verdict = FALL;
				}
				if (verdict == BLOCK) {
					bool carryOn = true;
					if (fly && !(f1 & 0x04)) {
						if (!isActor)
							carryOn = false;
						else if (!(MovementClasses::flags(MovementClasses::classOf(di)) & MovementClasses::FLIES))
							carryOn = false;
						if (!carryOn)
							verdict = FALL;
					}
					if (carryOn) {
						if (ghost && !isActor) {
							verdict = FALL;
						} else if (inParty) {
							if (si == rules.leader || !rules.partyMayShuffle || !isActor || !(o.status & 0x80) ||
								(o.status & 0x16) || (rules.actorFlags && ((*rules.actorFlags)[si] & 0x10)))
								verdict = BLOCK;
							else
								verdict = SKIP; // the party members change places
						} else if (!moverIsActor) {
							verdict = BLOCK;
						} else {
							bool door = (di >= 0x129 && di <= 0x12C && o.frame < 8) || di == 0x116 || di == 0x118;
							verdict = (door && (cflags & MovementClasses::OPENS_DOORS)) ? FALL : BLOCK;
						}
					}
				}
			}
			(void)cont;
		}

		if (verdict == SKIP)
			continue;
		if (verdict == BLOCK) {
			ok = false;
			more = false;
		}
		// Any other creature in the way stops the mover, except for a few kinds that it may share a tile with.
		if (isActor && o.actor != 0) {
			static const int SHARE[5] = {0x164, 0x162, 0x157, 0x167, 0x165};
			bool shareable = false;
			for (int s : SHARE)
				shareable |= di == s;
			if (!shareable || di == type) {
				res.passable = false;
				return res;
			}
		}
		if ((f1 & 0x04) && (type == 0x16A || type == 0x1A8 || type == 0x19B || type == 0x1A9)) {
			ok = false;
			more = false;
		}
	}
	res.passable = ok;
	return res;
}

} // namespace Ultima6
} // namespace Ultima6

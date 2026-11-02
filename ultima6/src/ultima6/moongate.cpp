#include "ultima6/session.h"

#include "ultima6/describe.h"

#include <algorithm>
#include <cstdlib>

namespace Ultima6 {
namespace Ultima6 {

// The two moons (advance_time+5EC, GAME.EXE 138CD..): each day has a gate for each moon (the pairs at ds:036A), and the phase of a moon is (gate * 3 + 18 or 20 - hour) mod 24
// (a C remainder, so it may be negative and is then kept as a byte).
static const uint8_t MOON_GATES[28][2] = {{0, 0}, {7, 0}, {7, 7}, {6, 6}, {6, 5}, {5, 4}, {5, 3}, {4, 2}, {3, 1}, {3, 0}, {2, 0}, {2, 7}, {1, 6}, {1, 5},
										   {0, 4}, {7, 3}, {7, 2}, {6, 1}, {6, 0}, {5, 0}, {5, 7}, {4, 6}, {3, 5}, {3, 4}, {2, 3}, {2, 2}, {1, 1}, {1, 0}};

void Game::updateMoons() {
	int day = clock.day < 1 ? 1 : (clock.day > 28 ? 28 : clock.day);
	_gateA = MOON_GATES[day - 1][0];
	_gateB = MOON_GATES[day - 1][1];
	_phaseA = (uint8_t)((_gateA * 3 + 0x12 - clock.hour) % 24);
	_phaseB = (uint8_t)((_gateB * 3 + 0x14 - clock.hour) % 24);
}

// sub_1887A: to the place where the buried moonstone of gate `n` lies (objlist +0x1C1B, eight sets of x, y, z words); when it holds nothing the party stays where it is
void Game::gateTo(int n) {
	const std::vector<uint8_t> &g = _world.actors.globals;
	size_t o = 0x2A + 6 * (size_t)n;
	int gx = 0, gy = 0, gz = 0;
	if (o + 6 <= g.size()) {
		gx = g[o] | (g[o + 1] << 8);
		gy = g[o + 2] | (g[o + 3] << 8);
		gz = g[o + 4] | (g[o + 5] << 8);
	}
	if (gx || gy || gz)
		changeLevel(gx, gy, gz);
}

// sub_275BD, the part for the objects the party steps on: a blue moon gate (type 0x55) takes the party to the gate of one of the moons, a red one (0x54) to the
// place its quality names (the 24 places at ds:171C / 174E / 1780, one for each square of the 5 x 5 around the Orb's user).
bool Game::enterMoongate(int px, int py) {
	if (_aboard)
		return false;
	std::vector<int> here;
	_world.objectsAt(px, py, z, here);
	for (int idx : here) {
		const World::Obj &o = _world.objects[idx];
		if (o.type == 0x55) {
			if (clock.hour == 0 && clock.minute < 10) {
				changeLevel(0x18, 0x1D, 1);
				advanceTime(10);
			} else {
				int d = std::abs(7 - (int)_phaseA) - std::abs(7 - (int)_phaseB);
				bool first = d < 0 || (d == 0 && (clock.minute < 30) == (_phaseA < _phaseB));
				gateTo(first ? _gateA : _gateB);
			}
			return true;
		}
		if (o.type == 0x54) {
			static const uint16_t X[24] = {899, 935, 435, 503, 147, 919, 68, 307, 188, 159, 739, 0, 0, 0, 227, 23, 128, 108, 923, 295, 75, 327, 387, 831};
			static const uint16_t Y[24] = {499, 262, 395, 358, 883, 934, 45, 352, 45, 942, 699, 0, 0, 0, 131, 22, 86, 221, 876, 38, 507, 822, 787, 166};
			static const uint8_t Z[24] = {0, 0, 0, 0, 0, 0, 5, 0, 5, 0, 0, 0, 0, 0, 0, 1, 5, 5, 0, 0, 0, 0, 0, 0};
			int q = o.quality;
			if (q >= 1 && q <= 24)
				changeLevel(X[q - 1], Y[q - 1], Z[q - 1]);
			return true;
		}
	}
	return false;
}

// use_orb_of_the_moons (34E19): asks "Where: " for a square within two of the user, and a red moon gate appears there if the square can be entered.
// APPROX(55): the first test of the original (bit 5 of byte_48B40, whose meaning is not traced) is replaced by "on the surface"; the square's test (`can_actor_enter`)
// is Movement's check for the leader; the object that is created lacks the animation
void Game::useOrb(int idx) {
	if (z != 0) {
		message("\nYou can't figure out how to use it.\n");
		return;
	}
	message("Where: ");
	_useItem = idx;
	_pending = 'o';
	_cursor = true;
	_cx = _cy = 5;
}

void Game::orbAt(int tx, int ty) {
	int item = _useItem;
	_useItem = -1;
	int mask = MapData::SURFACE_SIZE - 1;
	int dx = ((tx - x + 512) & mask) - 512, dy = ((ty - y + 512) & mask) - 512;
	if (std::abs(dx) > 2 || std::abs(dy) > 2) {
		message("Failed\n");
		return;
	}
	int q = dy == 0 && std::abs(dx) <= 1 ? 0 : (dy + 2) * 5 + dx + 3;
	MoveRules rules;
	bool free = _movement.check(tx, ty, z, _leaderObject, rules).passable;
	std::vector<int> here;
	_world.objectsAt(tx, ty, z, here);
	for (int i : here)
		if (_world.objects[i].type == 0x12D || _world.objects[i].type == 0x12E)
			free = false;
	if (!free) {
		message("Failed\n");
		_world.deleteObject(item);
		return;
	}
	message("a red moon gate appears.\n");
	World::Obj gate;
	gate.status = 0;
	gate.x = (uint16_t)tx;
	gate.y = (uint16_t)ty;
	gate.z = (uint8_t)z;
	gate.type = 0x54;
	gate.frame = 0;
	gate.quantity = 1;
	gate.quality = (uint8_t)q;
	gate.actor = -1;
	gate.parent = -1;
	_world.objects.push_back(gate);
	_world.addToMap((int)_world.objects.size() - 1);
}

} // namespace Ultima6
} // namespace Ultima6

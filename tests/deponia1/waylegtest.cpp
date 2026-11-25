#define private public
#define protected public
#include <cstdio>
#include <vector>

#include "AppGlobals.h"
#include "TGWaySystem.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

// The direction of each leg of a way along several points (TGWaySystem::GetNextPosition, asm 249540): it is worked out when a leg
// starts, also for the legs after the first (a character walked a ramp facing the direction of its first leg).
int main() {
	TGWaySystem way;

	// east 40, then north (the screen's y points down) 40, then west 40: three legs, then the destination
	way._path = {wxPoint{40, 0}, wxPoint{40, -40}, wxPoint{0, -40}};
	way._pathIndex = 0;
	way._destination = wxPoint{0, -40};
	way._headingForDestination = false;
	way._walkingToWayPoint = false;
	way._angleKnown = false;

	wxRealPoint position;

	position.x = 0.0;
	position.y = 0.0;
	std::vector<int> angles;
	bool arrived = false;

	for (int step = 0; step < 200 && !arrived; step++) {
		int angle = -1;

		arrived = way.GetNextPosition(position, &angle, 7.0f);

		if (angle != -1 && (angles.empty() || angles.back() != angle))
			angles.push_back(angle);
	}

	CHECK(arrived);
	CHECK(angles.size() >= 3);
	CHECK(angles.size() > 0 && angles[0] == 0);
	CHECK(angles.size() > 1 && angles[1] == 90);
	CHECK(angles.size() > 2 && angles[2] == 180);

	for (int angle : angles)
		printf("angle %d\n", angle);

	printf("%d failures\n", failures);
	return failures != 0;
}

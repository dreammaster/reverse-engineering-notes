#include <cstdio>
#include <vector>

#include "AppGlobals.h"
#include "TPolygonList.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); failures++; } } while (0)

double calcDist(double x1, double y1, double x2, double y2);

static std::vector<wxPoint> square(int x, int y, int size) {
	return {{x, y}, {x + size, y}, {x + size, y + size}, {x, y + size}};
}

// The helpers for polygons (the walkable areas, the hit areas of objects): the list that owns its polygons, the lists of
// points with separators, the bounding boxes and the tests of lines and points against polygons.
int main() {
	setvbuf(stdout, nullptr, _IONBF, 0);
	wxLog::loglevel = 2;

	std::vector<wxPoint> a = square(0, 0, 10);
	std::vector<wxPoint> b = {{20, 20}, {30, 20}, {25, 30}};

	// the list owns copies
	TPolygonList list;

	CHECK(list.empty());
	list.push_back(a);
	list.push_back(b);
	CHECK(list.size() == 2);
	CHECK(list.front() != &a);
	CHECK(list.at(1)->size() == 3);
	CHECK(list[0]->size() == 4);
	CHECK(list.back() == list[1]);
	CHECK(&**list.rbegin() == list[1]);

	TPolygonList copy(list);

	CHECK(copy.size() == 2);
	CHECK(copy[0] != list[0]);
	CHECK(*copy[1] == b);

	TPolygonList assigned;

	assigned.push_back(square(50, 50, 5));
	assigned = list;
	CHECK(assigned.size() == 2);
	assigned = assigned;
	CHECK(assigned.size() == 2);
	assigned.erase(assigned.begin());
	CHECK(assigned.size() == 1);
	CHECK(*assigned[0] == b);

	// (the polygon of pop_back stays: it is not freed, so it is lost here on purpose)
	TPolygonList popped;
	std::vector<wxPoint> *keep;

	popped.push_back(a);
	keep = popped.back();
	popped.pop_back();
	CHECK(popped.empty());
	delete keep;

	copy.clear();
	CHECK(copy.empty());

	// point lists with the separator between polygons
	std::vector<wxPoint> points;

	CreatePointListFromPolygons(list, points);
	CHECK(points.size() == 8);
	CHECK(points.size() == 8 && points[4] == kPolygonSeparator);

	TPolygonList back;

	CHECK(CreatePolygonsFromPointList(points, back));
	CHECK(back.size() == 2);
	CHECK(back.size() == 2 && *back[0] == a && *back[1] == b);

	// (a polygon of fewer than three points is made, but it is told)
	std::vector<wxPoint> thin = {{0, 0}, {5, 5}, kPolygonSeparator, {1, 1}, {2, 2}, {3, 5}};

	CHECK(!CreatePolygonsFromPointList(thin, back));
	CHECK(back.size() == 2);

	// the empty polygons are left out of the list of points
	TPolygonList withEmpty;

	withEmpty.push_back(a);
	withEmpty.push_back(std::vector<wxPoint>());
	withEmpty.push_back(b);
	CreatePointListFromPolygons(withEmpty, points);
	CHECK(points.size() == 8);

	// the bounding boxes count the last row and column too
	wxRect box = GetBoundingBox(list);

	CHECK(box.x == 0 && box.y == 0 && box.width == 31 && box.height == 31);
	box = GetBoundingBox(a);
	CHECK(box.x == 0 && box.y == 0 && box.width == 11 && box.height == 11);
	box = GetBoundingBox(b);
	CHECK(box.x == 20 && box.y == 20 && box.width == 11 && box.height == 11);

	TPolygonList none;

	box = GetBoundingBox(none);
	CHECK(box.width == 0 && box.height == 0);

	// lines against lines
	CHECK(LinesCut(wxPoint{-5, 5}, wxPoint{15, 5}, wxPoint{0, 0}, wxPoint{0, 10}));
	CHECK(!LinesCut(wxPoint{-5, 5}, wxPoint{-1, 5}, wxPoint{0, 0}, wxPoint{0, 10}));
	CHECK(!LinesCut(wxPoint{0, 0}, wxPoint{10, 0}, wxPoint{0, 5}, wxPoint{10, 5}));
	CHECK(LinesCut(wxPoint{0, 0}, wxPoint{10, 10}, wxPoint{0, 10}, wxPoint{10, 0}));

	// lines against polygons
	CHECK(LineCuts(wxPoint{-5, 5}, wxPoint{5, 5}, a));
	CHECK(!LineCuts(wxPoint{2, 2}, wxPoint{8, 8}, a));
	CHECK(!LineCuts(wxPoint{-5, -5}, wxPoint{-1, 5}, a));
	CHECK(LineCuts(wxPoint{-5, 5}, wxPoint{5, 5}, list));
	CHECK(LineCuts(wxPoint{22, 15}, wxPoint{25, 25}, list));
	CHECK(!LineCuts(wxPoint{12, 12}, wxPoint{18, 18}, list));
	CHECK(!LineCuts(wxPoint{0, 0}, wxPoint{1, 1}, std::vector<wxPoint>()));
	CHECK(GetNumCuts(wxPoint{-5, 5}, wxPoint{15, 5}, a) == 2);
	CHECK(GetNumCuts(wxPoint{2, 2}, wxPoint{8, 8}, a) == 0);
	CHECK(GetNumCuts(wxPoint{-5, 5}, wxPoint{5, 5}, a) == 1);
	{
		std::vector<wxPoint> twoPoints = {{0, 0}, {10, 10}};

		CHECK(GetNumCuts(wxPoint{-5, 5}, wxPoint{15, 5}, twoPoints) == 0);
	}

	// points against polygons
	CHECK(PointInPolygon(5, 5, a));
	CHECK(!PointInPolygon(15, 5, a));
	CHECK(!PointInPolygon(-1, 5, a));
	CHECK(PointInPolygon(25, 23, b));
	CHECK(!PointInPolygon(21, 29, b));

	// (a polygon inside another is a hole)
	TPolygonList ring;

	ring.push_back(square(0, 0, 100));
	ring.push_back(square(40, 40, 20));
	CHECK(PointInPolygonList(10, 10, ring));
	CHECK(!PointInPolygonList(50, 50, ring));
	CHECK(!PointInPolygonList(150, 50, ring));
	CHECK(IsPointInsidePolygon(wxPoint{10, 10}, ring));
	CHECK(!IsPointInsidePolygon(wxPoint{50, 50}, ring));
	CHECK(!IsPointInsidePolygon(wxPoint{150, 50}, ring));

	// segments in the area
	CHECK(LineInPolygonSet(5, 5, 20, 20, ring));
	CHECK(LineInPolygonSet(10, 50, 30, 50, ring));
	CHECK(!LineInPolygonSet(10, 50, 90, 50, ring));
	CHECK(!LineInPolygonSet(10, 10, 150, 10, ring));

	// the way round the corner of an L, and round the hole of the ring
	TPolygonList ell;

	ell.push_back({{0, 0}, {100, 0}, {100, 40}, {40, 40}, {40, 100}, {0, 100}});

	std::vector<wxPoint> path;

	CHECK(shortestPath(10, 90, 90, 10, ell, path));
	printf("the way round the corner:");
	for (const wxPoint &point : path)
		printf(" %d,%d", point.x, point.y);
	printf("\n");
	CHECK(!path.empty());
	CHECK((!path.empty() && path.front() == wxPoint{40, 40}));

	// (the corners are added to the path that is given: it is not cleared)
	CHECK(shortestPath(10, 10, 90, 10, ell, path));
	CHECK(path.size() == 2);
	path.clear();
	CHECK(shortestPath(10, 10, 90, 10, ell, path));
	CHECK(path.size() == 1);
	CHECK((path.size() == 1 && path[0] == wxPoint{90, 10}));

	CHECK(!shortestPath(-10, -10, 90, 10, ell, path));

	path.clear();
	CHECK(shortestPath(10, 50, 90, 50, ring, path));
	printf("the way round the hole:");
	for (const wxPoint &point : path)
		printf(" %d,%d", point.x, point.y);
	printf("\n");
	CHECK(path.size() >= 2);

	CHECK(calcDist(0, 0, 3, 4) == 5.0);
	CHECK(calcDist(1, 1, 1, 1) == 0.0);

	printf("%d failures\n", failures);
	return failures ? 1 : 0;
}

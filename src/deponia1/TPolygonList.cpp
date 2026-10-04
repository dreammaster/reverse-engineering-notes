#include "TPolygonList.h"

#include <algorithm>
#include <cmath>

#include "Diagnostics.h"

static const char *const kSourceFile = "/home/simon/Documents/jenkins/branchPillars/src/baselib/polygon.cpp";

const wxPoint kPolygonSeparator = {-10000, -10000};

TPolygonList::TPolygonList(const TPolygonList &other) {
	*this = other;
}

TPolygonList::~TPolygonList() {
	clear();
}

TPolygonList &TPolygonList::operator=(const TPolygonList &other) {
	if (this == &other)
		return *this;

	clear();
	for (const std::vector<wxPoint> *polygon : other._polygons)
		_polygons.push_back(new std::vector<wxPoint>(*polygon));
	return *this;
}

void TPolygonList::clear() {
	for (std::vector<wxPoint> *polygon : _polygons)
		delete polygon;
	_polygons.clear();
}

void TPolygonList::erase(iterator position) {
	delete *position;
	_polygons.erase(position);
}

void TPolygonList::push_back(const std::vector<wxPoint> &polygon) {
	_polygons.push_back(new std::vector<wxPoint>(polygon));
}

// Confirmed (asm lines 1376827-1376953)
bool CreatePolygonsFromPointList(const std::vector<wxPoint> &points, TPolygonList &outPolygons) {
	bool result = true;

	outPolygons.clear();

	std::vector<wxPoint> *current = nullptr;
	unsigned count = 0;

	for (const wxPoint &point : points) {
		if (point == kPolygonSeparator) {
			if (count < 3)
				result = false;
			count = 0;
			current = nullptr;
			continue;
		}

		if (!current) {
			outPolygons.push_back(std::vector<wxPoint>());
			current = outPolygons.back();
		}
		current->push_back(point);
		count++;
	}

	if (!points.empty() && count < 3)
		result = false;

	return result;
}

// Confirmed (asm lines 1376954-1377179)
void CreatePointListFromPolygons(const TPolygonList &polygons, std::vector<wxPoint> &outPoints) {
	outPoints.clear();
	outPoints.reserve(20);

	bool first = true;
	for (const std::vector<wxPoint> *polygon : polygons) {
		if (polygon->empty())
			continue;

		if (!first)
			outPoints.push_back(kPolygonSeparator);
		for (const wxPoint &point : *polygon)
			outPoints.push_back(point);
		first = false;
	}
}

// Confirmed (asm lines 1375355-1375588)
wxRect GetBoundingBox(const TPolygonList &polygons) {
	bool any = false;
	int minX = 0, minY = 0, maxX = 0, maxY = 0;

	for (const std::vector<wxPoint> *polygon : polygons) {
		for (const wxPoint &point : *polygon) {
			if (!any) {
				minX = maxX = point.x;
				minY = maxY = point.y;
				any = true;
				continue;
			}

			minX = std::min(minX, point.x);
			minY = std::min(minY, point.y);
			maxX = std::max(maxX, point.x);
			maxY = std::max(maxY, point.y);
		}
	}

	wxRect result;
	if (any) {
		result.x = minX;
		result.y = minY;
		result.width = maxX - minX + 1;
		result.height = maxY - minY + 1;
	} else {
		x_assert(false, "false", kSourceFile, 0xAB);
	}
	return result;
}

// Confirmed (asm lines 1375800-1376000, the vector overload)
wxRect GetBoundingBox(const std::vector<wxPoint> &points) {
	wxRect result;

	if (points.empty()) {
		x_assert(false, "false", kSourceFile, 0x111);
		return result;
	}

	int minX = points[0].x, minY = points[0].y, maxX = minX, maxY = minY;
	for (const wxPoint &point : points) {
		minX = std::min(minX, point.x);
		minY = std::min(minY, point.y);
		maxX = std::max(maxX, point.x);
		maxY = std::max(maxY, point.y);
	}

	result.x = minX;
	result.y = minY;
	result.width = maxX - minX + 1;
	result.height = maxY - minY + 1;
	return result;
}

// The side of the line a-b a point c is on: 1, 0 or -1 (the sign is inverted
// against the usual cross product, as in the original).
static int side(const wxPoint &a, const wxPoint &b, const wxPoint &c) {
	int value = (c.x - a.x) * (b.y - a.y) + (a.y - c.y) * (b.x - a.x);

	if (value < 0)
		return 1;
	if (value == 0)
		return 0;
	return -1;
}

// Confirmed (asm lines 1376292-1376537)
bool LinesCut(const wxPoint &a, const wxPoint &b, const wxPoint &c, const wxPoint &d) {
	if (side(a, b, c) == side(a, b, d))
		return false;

	return side(c, d, a) != side(c, d, b);
}

// Confirmed (asm lines 247392-247504)
bool PointInPolygon(int x, int y, std::vector<wxPoint> &polygon) {
	bool inside = false;
	size_t count = polygon.size();

	for (size_t i = 0; i < count; i++) {
		const wxPoint &current = polygon[i];
		const wxPoint &next = polygon[(i == count - 1) ? 0 : i + 1];

		// the edge has to straddle the horizontal line through the point
		bool currentAbove = current.y >= y;
		bool nextAbove = next.y >= y;
		if (currentAbove == nextAbove)
			continue;

		// ... and the point must not be to the left of both ends
		if (current.x > x && x < next.x)
			continue;

		wxPoint point = {x, y};
		wxPoint farLeft = {-10000, y};
		if (LinesCut(current, next, point, farLeft))
			inside = !inside;
	}
	return inside;
}

// Confirmed (asm lines 247731-247862)
bool PointInPolygonList(int x, int y, TPolygonList &polygons) {
	bool inside = false;

	for (size_t i = 0; i < polygons.size(); i++) {
		if (PointInPolygon(x, y, *polygons[i]))
			inside = !inside;
	}
	return inside;
}

// Confirmed (asm lines 1377691-1377720)
bool IsPointInsidePolygon(const wxPoint &point, const TPolygonList &polygons) {
	int count = 0;

	for (const std::vector<wxPoint> *polygon : polygons) {
		if (IsPointInsidePolygon(point, *polygon))
			count++;
	}
	return (count & 1) != 0;
}

// Approximation of asm lines 1377180-1377690: that is a float scan-line
// algorithm (it collects the x of every edge crossing the point's row, sorts
// them and tests whether the point's x falls between a pair of them); this is
// the even-odd rule, which gives the same answer except for points exactly on
// an edge or vertex. A polygon needs at least three points.
bool IsPointInsidePolygon(const wxPoint &point, const std::vector<wxPoint> &polygon) {
	if (polygon.size() < 3)
		return false;

	bool inside = false;
	for (size_t i = 0, j = polygon.size() - 1; i < polygon.size(); j = i++) {
		const wxPoint &a = polygon[i];
		const wxPoint &b = polygon[j];

		if ((a.y > point.y) != (b.y > point.y)) {
			double intersectX = a.x + static_cast<double>(point.y - a.y) / (b.y - a.y) * (b.x - a.x);
			if (point.x < intersectX)
				inside = !inside;
		}
	}
	return inside;
}

// Confirmed (asm lines 248161-248190)
double calcDist(double x1, double y1, double x2, double y2) {
	return sqrt((x2 - x1) * (x2 - x1) + (y2 - y1) * (y2 - y1));
}

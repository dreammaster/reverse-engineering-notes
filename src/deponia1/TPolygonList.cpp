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

// Confirmed (asm lines 1376396-1376462)
bool LineCuts(const wxPoint &a, const wxPoint &b, const std::vector<wxPoint> &polygon) {
	if (polygon.empty())
		return false;

	wxPoint previous = polygon.back();

	for (const wxPoint &point : polygon) {
		if (LinesCut(a, b, point, previous))
			return true;
		previous = point;
	}
	return false;
}

// Confirmed (asm lines 1376538-1376630)
bool LineCuts(const wxPoint &a, const wxPoint &b, const TPolygonList &polygons) {
	for (const std::vector<wxPoint> *polygon : polygons) {
		if (LineCuts(a, b, *polygon))
			return true;
	}
	return false;
}

// Confirmed (asm lines 1376463-1376537)
int GetNumCuts(const wxPoint &a, const wxPoint &b, const std::vector<wxPoint> &polygon) {
	// (the original reads the polygon as edges from the last point round; a polygon
	// of fewer than three points has none)
	if (polygon.size() < 3)
		return 0;

	int cuts = 0;
	wxPoint previous = polygon.back();

	for (const wxPoint &point : polygon) {
		if (LinesCut(a, b, point, previous))
			cuts++;
		previous = point;
	}
	return cuts;
}

// Confirmed (asm lines 247863-248160). The edge points are moved into the
// segment's frame: x along the segment (0 to its length), y across it.
bool LineInPolygonSet(double x1, double y1, double x2, double y2, TPolygonList &polygons) {
	double dx = x2 - x1;
	double dy = y2 - y1;
	double length = std::sqrt(dx * dx + dy * dy);
	double ux = dx / length;
	double uy = dy / length;

	for (size_t i = 0; i < polygons.size(); i++) {
		const std::vector<wxPoint> &polygon = *polygons[i];
		size_t count = polygon.size();

		for (size_t j = 0; j < count; j++) {
			const wxPoint &first = polygon[j];
			const wxPoint &second = polygon[(j + 1 == count) ? 0 : j + 1];

			double ax = first.x - x1;
			double ay = first.y - y1;
			double bx = second.x - x1;
			double by = second.y - y1;

			// the segment is an edge of the polygon (either way round)
			if (ay == 0 && ax == 0 && by == dy && bx == dx)
				return true;
			if (by == 0 && bx == 0 && ay == dy && ax == dx)
				return true;

			double aAlong = ax * ux + ay * uy;
			double aAcross = ay * ux - ax * uy;
			double bAlong = bx * ux + by * uy;
			double bAcross = by * ux - bx * uy;
			bool crosses = false;

			// the edge crosses the line of the segment: where?
			if ((0 > aAcross && bAcross > 0) || (0 > bAcross && aAcross > 0)) {
				double along = (bAlong - aAlong) * (0 - aAcross) / (bAcross - aAcross) + aAlong;

				if (!(along < 0 || length < along))
					return false;
			}

			// an edge along the line of the segment must not overlap it only partly
			if (aAcross == 0 && bAcross == 0) {
				if (aAlong < 0)
					crosses = bAlong >= 0;
				else
					crosses = length >= aAlong || !(length < bAlong);

				if (crosses && (0 > aAlong || 0 > bAlong || aAlong > length || bAlong > length))
					return false;
			}
		}
	}

	return PointInPolygonList((int)(x1 + dx * 0.5), (int)(y1 + dy * 0.5), polygons);
}

namespace {

// A vertex of the graph shortestPath() searches, with the way found to it.
struct PathNode {
	int x, y;
	int previous;
	float distance;
};

} // End of anonymous namespace

// Confirmed (asm lines 251746-252152): a Dijkstra search over the start, the
// polygon vertices and the end point, where two points are joined when
// LineInPolygonSet() says they see each other. The settled nodes are kept at the
// front of the array, the next one swapped in behind them.
bool shortestPath(double x1, double y1, double x2, double y2, TPolygonList &polygons, std::vector<wxPoint> &path) {
	int startX = (int)x1;
	int startY = (int)y1;
	int endX = (int)x2;
	int endY = (int)y2;

	if (!PointInPolygonList(startX, startY, polygons))
		return false;

	// (the original also tests whether the end point is in the area, but ignores the
	// result)
	if (LineInPolygonSet(x1, y1, x2, y2, polygons)) {
		path.push_back(wxPoint{endX, endY});
		return true;
	}

	std::vector<PathNode> nodes;

	nodes.push_back(PathNode{startX, startY, 0, 0.0f});
	for (size_t i = 0; i < polygons.size(); i++) {
		for (const wxPoint &point : *polygons[i])
			nodes.push_back(PathNode{point.x, point.y, 0, 0.0f});
	}
	nodes.push_back(PathNode{endX, endY, 0, 0.0f});

	int last = (int)nodes.size() - 1;
	int settled = 0;
	int limit;

	for (;;) {
		double best = 9999999.0;
		int bestTo = 0;
		int bestFrom = 0;

		limit = settled + 1;
		for (int from = 0; from < limit; from++) {
			if (last < limit)
				break;

			for (int to = limit; to <= last; to++) {
				if (!LineInPolygonSet(nodes[from].x, nodes[from].y, nodes[to].x, nodes[to].y, polygons))
					continue;

				double total = std::sqrt((double)(nodes[to].x - nodes[from].x) * (nodes[to].x - nodes[from].x) +
				                         (double)(nodes[to].y - nodes[from].y) * (nodes[to].y - nodes[from].y)) +
				               nodes[from].distance;

				if (best > total) {
					best = total;
					bestTo = to;
					bestFrom = from;
				}
			}
		}

		if (best == 9999999.0)
			return false;

		nodes[bestTo].previous = bestFrom;
		nodes[bestTo].distance = (float)best;
		std::swap(nodes[bestTo], nodes[limit]);
		settled++;

		if (bestTo >= last)
			break;
	}

	// the way back from the end point, without it and without the start
	int count = 0;

	for (int node = nodes[limit].previous; node > 0; node = nodes[node].previous)
		count++;
	path.resize(count);
	for (int index = count - 1, node = nodes[limit].previous; index >= 0; index--, node = nodes[node].previous)
		path[index] = wxPoint{nodes[node].x, nodes[node].y};
	return true;
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

#include "TGWaySystem.h"

#include <algorithm>
#include <cmath>
#include <cstring>
#include <exception>
#include <utility>

#include "Diagnostics.h"
#include "TManagedObject.h"
#include "datastruct/visionaireobject.h"
#include "vstables/fieldIds.h"

wxPoint TGWaySystem::destinationForCmp = {0, 0};
std::vector<wxPoint> drawPoints;

static bool sameId(const std::uint8_t *first, const std::uint8_t *second) {
	return std::memcmp(first, second, 4) == 0;
}

// Confirmed (asm lines 250249-250313)
TGWaySystem::TGWaySystem()
	: _pathIndex(-1), _angleKnown(false), _bestLength(-1), _currentLength(-1), _destination{0, 0},
	  _headingForDestination(false), _walkingToWayPoint(true) {
}

// Confirmed (asm lines 250314-250543): the points of the triangulations and the
// triangulations are freed (the triangles belong to them).
TGWaySystem::~TGWaySystem() {
	for (std::vector<p2t::Point *> &corners : _trianglePoints) {
		for (p2t::Point *corner : corners)
			delete corner;
	}
	for (p2t::CDT *triangulation : _triangulations)
		delete triangulation;
}

// Confirmed (asm lines 247325-247391)
bool TGWaySystem::ClosestPoint(const TVisionaireObject *first, const TVisionaireObject *second) {
	const wxPoint &a = *first->GetPoint(kPointPosition);
	const wxPoint &b = *second->GetPoint(kPointPosition);

	int distanceA = std::abs(a.x - destinationForCmp.x) + std::abs(a.y - destinationForCmp.y);
	int distanceB = std::abs(b.x - destinationForCmp.x) + std::abs(b.y - destinationForCmp.y);

	return distanceA < distanceB;
}

// Confirmed (asm lines 247505-247605)
TVisObjRef TGWaySystem::GetPointNextTo(const wxPoint &position) {
	TVisObjRef nearest;
	int nearestDistance = -1;

	for (TVisionaireObject *object : _wayPoints) {
		TVisObjRef wayPoint(*object);
		const wxPoint &place = *wayPoint.GetPoint(kPointPosition);
		int distance = (place.x - position.x) * (place.x - position.x) + (place.y - position.y) * (place.y - position.y);

		if (nearestDistance == -1 || nearestDistance > distance) {
			nearest = wayPoint;
			nearestDistance = distance;
		}
	}
	return nearest;
}

// Confirmed (asm lines 247606-247730)
TVisObjRef TGWaySystem::GetAllPointsNextTo(const wxPoint &position, TVList &points) {
	TVisObjRef nearest;
	int nearestDistance = -1;

	for (TVisionaireObject *object : _wayPoints) {
		TVisObjRef wayPoint(*object);
		wxPoint place = *wayPoint.GetPoint(kPointPosition);

		if (LineCuts(position, place, _borders))
			continue;

		int distance = (place.x - position.x) * (place.x - position.x) + (place.y - position.y) * (place.y - position.y);

		if (nearestDistance == -1 || nearestDistance > distance) {
			nearest = wayPoint;
			nearestDistance = distance;
		}
		points.push_back(wayPoint);
	}
	return nearest;
}

// Confirmed (asm lines 249066-249195)
wxPoint TGWaySystem::CheckPosition(const wxPoint &position) {
	for (p2t::Triangle *triangle : _triangles) {
		if (PointInTriangle(*triangle, position))
			return position;
	}

	for (p2t::Triangle *triangle : _triangles) {
		if (!PointInTriangleTolerant(*triangle, position))
			continue;
		if (PointInTriangle(*triangle, position))
			continue;

		// just outside the triangle: its nearest point
		point nearest = closesPointOnTriangle(triangle->GetPoint(0), triangle->GetPoint(1), triangle->GetPoint(2),
		                                      position);

		return wxPoint{(int)nearest.x, (int)nearest.y};
	}
	return position;
}

// Confirmed (asm lines 249540-249794). The way is walked leg by leg: `position`
// is moved `speed` towards the current target; once it would pass it, it is put
// on it and the next point of the way becomes the target. The direction of a leg
// is worked out when it starts.
bool TGWaySystem::GetNextPosition(wxRealPoint &position, int *angle, float speed) {
	wxPoint target;

	if (_headingForDestination || _pathIndex < 0 || _pathIndex >= (int)_path.size())
		target = _destination;
	else
		target = _path[_pathIndex];

	// the target has been reached exactly: on to the next point of the way
	if ((int)position.x == target.x && (int)position.y == target.y) {
		if (_pathIndex < (int)_path.size() - 1) {
			_pathIndex++;
			target = _path[_pathIndex];
		}
	}

	float targetX = (float)target.x;
	float targetY = (float)target.y;
	float dx = targetX - position.x;
	float dy = targetY - position.y;

	if (!_angleKnown)
		*angle = GetAngle(dx, dy);

	float length = std::sqrt(dx * dx + dy * dy);
	float step = 0.0f;

	if (length > 0.0f)
		step = speed / length;

	position.x = dx * step + position.x;
	position.y = dy * step + position.y;

	// has the target been passed (or reached)?
	bool reached = false;

	if (dx > 0.0f && position.x >= targetX)
		reached = true;
	else if (0.0f > dx && targetX >= position.x)
		reached = true;
	else if (dy > 0.0f && position.y >= targetY)
		reached = true;
	else if (0.0f > dy && targetY >= position.y)
		reached = true;
	else if (dy == 0.0f && dx == 0.0f)
		reached = true;

	if (!reached) {
		_angleKnown = true;
		return false;
	}

	position.x = targetX;
	position.y = targetY;

	// not at the end of the way yet
	if (_pathIndex < (int)_path.size() - 1) {
		_angleKnown = true;
		return false;
	}

	if (_walkingToWayPoint)
		return true;

	// the way's end is reached; the destination is next, unless that is where we are
	if ((int)targetX == _destination.x && (int)targetY == _destination.y)
		return true;

	_headingForDestination = true;
	_angleKnown = false;
	return false;
}

// Confirmed (asm lines 249795-249820)
void TGWaySystem::SetDestination(const wxPoint &destination) {
	_walkingToWayPoint = false;
	_headingForDestination = true;
	_destination = destination;
	_pathIndex = -1;
	_angleKnown = false;
	_path.clear();
}

// Confirmed (asm lines 249821-249865)
wxPoint TGWaySystem::GetDestination() const {
	if (!_walkingToWayPoint)
		return _destination;
	if (_path.empty())
		return wxPoint{-1, -1};
	return _path.back();
}

// Confirmed (asm lines 249866-249907)
int TGWaySystem::GetDirectionToNextDestination(const wxPoint &position) const {
	wxPoint destination = _destination;

	if (_walkingToWayPoint) {
		destination = wxPoint{-1, -1};
		if (!_path.empty())
			destination = _path.back();
	}
	return GetAngle((float)(destination.x - position.x), (float)(destination.y - position.y));
}

// Confirmed (asm lines 249908-249924): the argument is not used
void TGWaySystem::SetNoLines(int /*lines*/) {
	TPointSize::UpdateSizeSystem(_wayPoints);
}

// Confirmed (asm lines 249925-249941)
int TGWaySystem::GetNoLines() const {
	return 0;
}

// Confirmed (asm lines 249942-249957)
float TGWaySystem::GetCalculatedSize(const wxPoint &position) const {
	return TPointSize::GetCalculatedSize(position);
}

// Confirmed (asm lines 249958-249973)
void TGWaySystem::UpdateSizeSystem() {
	TPointSize::UpdateSizeSystem(_wayPoints);
}

// Confirmed (asm lines 249974-249990)
bool TGWaySystem::IsWalkingToWayPoint() const {
	return _walkingToWayPoint;
}

// Confirmed (asm lines 249991-250011)
bool TGWaySystem::CheckCrossBorders(const wxPoint &first, const wxPoint &second) const {
	return LineCuts(first, second, _borders);
}

// Confirmed (asm lines 250012-250248): a breadth first walk over the links
// between way points.
bool TGWaySystem::IsConnected(const TVisObjRef &first, const TVisObjRef &second) const {
	if (first == second)
		return true;

	TVList reached;
	TVList links;

	first.GetLinks(kPointRelations, TypeOrder::kValue0, links);
	for (TVisionaireObject *link : links)
		reached.push_back(TVisObjRef(*link));

	for (size_t i = 0; i < reached.size(); i++) {
		TVisionaireObject *wayPoint = reached.at((int)i);

		if (sameId(second.GetId(), wayPoint->GetId()))
			return true;

		TVList further;

		wayPoint->GetLinks(kPointRelations, TypeOrder::kValue0, further);
		for (TVisionaireObject *link : further) {
			bool known = false;

			for (TVisionaireObject *other : reached) {
				if (sameId(other->GetId(), link->GetId())) {
					known = true;
					break;
				}
			}
			if (!known)
				reached.push_back(TVisObjRef(*link));
		}
	}
	return false;
}

// Confirmed (asm lines 250544-251745): the outline is cut into polygons; a polygon
// that lies inside another one is a hole of it (only one level of them is
// understood: a polygon inside a hole is another hole of the outer one). The
// outer polygons are triangulated together with their holes.
bool TGWaySystem::SetWaySystem(const TVisObjRef &waySystem) {
	waySystem.GetLinks(kWaySystemPoints, TypeOrder::kValue0, _wayPoints);
	_path.clear();
	_pathIndex = -1;

	std::vector<wxPoint> points;

	waySystem.GetPoints(kWaySystemBorder, points);
	CreatePolygonsFromPointList(points, _borders);

	if (_borders.size() == 0)
		return points.size() > 2;

	// drop the previous triangulation
	for (std::vector<p2t::Point *> &old : _trianglePoints) {
		for (p2t::Point *corner : old)
			delete corner;
	}
	for (p2t::CDT *triangulation : _triangulations)
		delete triangulation;
	_trianglePoints.clear();
	_triangulations.clear();
	_triangles.clear();

	// which polygons are inside which
	size_t count = _borders.size();
	std::vector<std::vector<int> > holes(count);
	std::vector<bool> isHole(count, false);

	for (size_t outer = 0; outer < count; outer++) {
		for (size_t inner = 0; inner < count; inner++) {
			if (outer == inner)
				continue;

			const wxPoint &first = _borders[inner]->at(0);

			if (PointInPolygon(first.x, first.y, *_borders[outer])) {
				isHole[inner] = true;
				holes[outer].push_back((int)inner);
			}
		}
	}

	for (size_t polygon = 0; polygon < count; polygon++) {
		if (isHole[polygon])
			continue;

		std::vector<p2t::Point *> outline;

		_trianglePoints.push_back(outline);
		for (const wxPoint &corner : *_borders[polygon])
			_trianglePoints.back().push_back(new p2t::Point((double)corner.x, (double)corner.y));
		if (_trianglePoints.back().size() <= 2)
			continue;

		p2t::CDT *triangulation = nullptr;

		try {
			triangulation = new p2t::CDT(_trianglePoints.back());

			for (int hole : holes[polygon]) {
				_trianglePoints.push_back(std::vector<p2t::Point *>());
				for (const wxPoint &corner : *_borders[hole])
					_trianglePoints.back().push_back(new p2t::Point((double)corner.x, (double)corner.y));
				triangulation->AddHole(_trianglePoints.back());
			}

			triangulation->Triangulate();

			std::vector<p2t::Triangle *> triangles = triangulation->GetTriangles();

			for (p2t::Triangle *triangle : triangles)
				_triangles.push_back(triangle);
			_triangulations.push_back(triangulation);
		} catch (std::exception &error) {
			// The format the original logs this with reads as the two letters "wa"
			// (not resolved); its arguments are the id and name of the way system,
			// the name of its parent and the exception's text.
			delete triangulation;
			if (wxLog::loglevel >= 0) {
				std::string text = error.what();

				wxLog::logexpanded(L"%d %s %s %s", PackVisId(waySystem.GetId()),
				                   waySystem.GetName().c_str().c_str(),
				                   waySystem.GetParent().GetName().c_str().c_str(),
				                   std::wstring(text.begin(), text.end()).c_str());
			}
		}
	}

	return true;
}

// Confirmed (asm lines 252153-252624): `wayPoint` is added to the route; when it
// is the way's end and the route is shorter than the best one so far, the route
// becomes the way; otherwise the way points linked to it, nearest to the end
// first, are searched through (while the route can still beat the best).
bool TGWaySystem::SearchWay(TVisionaireObject *wayPoint) {
	// a way point is only visited once on a route
	if (std::find(_route.items.begin(), _route.items.end(), wayPoint) != _route.items.end())
		return false;

	_route.push_back(wayPoint);

	bool found = false;

	if (sameId(wayPoint->GetId(), _targetWayPoint.GetId())) {
		if (_bestLength == -1 || _bestLength > _currentLength) {
			_bestLength = _currentLength;
			_path.clear();
			for (TVisionaireObject *routePoint : _route)
				_path.push_back(*routePoint->GetPoint(kPointPosition));
			found = true;
		}
		_route.pop_back();
		return found;
	}

	if (_currentLength < _bestLength || _bestLength == -1) {
		TVList links;

		wayPoint->GetLinks(kPointRelations, TypeOrder::kValue0, links);
		std::sort(links.items.begin(), links.items.end(), ClosestPoint);

		for (TVisionaireObject *link : links) {
			if (link->IsEmpty())
				continue;

			const wxPoint &from = *wayPoint->GetPoint(kPointPosition);
			const wxPoint &to = *link->GetPoint(kPointPosition);
			int distance = (int)std::sqrt((double)(from.x - to.x) * (from.x - to.x) +
			                              (double)(from.y - to.y) * (from.y - to.y));

			if (_currentLength + distance < _bestLength || _bestLength == -1) {
				_currentLength += distance;
				if (SearchWay(link))
					found = true;
				_currentLength -= distance;
			}
		}
	}

	_route.pop_back();
	return found;
}

// Confirmed (asm lines 252625-253066)
bool TGWaySystem::CalculateWay(const TVisObjRef &wayPoint, const wxPoint &from, const wxPoint &to) {
	_bestLength = -1;
	_currentLength = -1;
	_route.clear();
	_pathIndex = -1;
	_path.clear();
	_targetWayPoint = wayPoint;
	destinationForCmp = *wayPoint.GetPoint(kPointPosition);
	_angleKnown = false;
	_headingForDestination = false;

	TVList visible;
	TVisObjRef nearest = GetAllPointsNextTo(from, visible);

	if (visible.size() == 0) {
		// no way point can be seen from here
		bool ok;

		if (LineInPolygonSet(from.x, from.y, to.x, to.y, _borders)) {
			_path.push_back(to);
			ok = true;
		} else {
			ok = shortestPath(from.x, from.y, to.x, to.y, _borders, _path);
		}

		if (!ok)
			return false;
		_pathIndex = 0;
		_destination = to;
		return true;
	}

	if (IsConnected(nearest, _targetWayPoint))
		SearchWay(nearest.GetObjectPointer());

	if (!_path.empty()) {
		// start from the farthest way point of the way that can be seen directly
		if (visible.size() > 1 && _path.size() >= 2) {
			int farthest = -1;

			for (size_t i = 0; i < visible.size(); i++) {
				for (int index = 1; index < (int)_path.size(); index++) {
					if (*visible.at((int)i)->GetPoint(kPointPosition) == _path[index])
						farthest = std::max(farthest, index);
				}
			}
			if (farthest > 0)
				_path.erase(_path.begin(), _path.begin() + farthest);
		}
		if (_path[0] == from)
			_path.erase(_path.begin());
	}

	_pathIndex = 0;

	if (to.x == -1 || to.y == -1) {
		_walkingToWayPoint = true;
		return !_path.empty();
	}

	if (_path.empty()) {
		_walkingToWayPoint = false;
		return false;
	}

	// the way ends at the first of its points the target can be seen from
	_walkingToWayPoint = true;
	for (size_t i = 0; i < _path.size(); i++) {
		if (!LineCuts(_path[i], to, _borders)) {
			_walkingToWayPoint = false;
			_destination = to;
			_path.erase(_path.begin() + i + 1, _path.end());
			break;
		}
	}
	return true;
}

// Confirmed (asm lines 253067-254196)
bool TGWaySystem::CalculateWayTriangles(const wxPoint &from, const wxPoint &to) {
	wxPoint target = to;

	_bestLength = -1;
	_currentLength = -1;
	_route.clear();
	_pathIndex = -1;
	_angleKnown = false;
	_headingForDestination = false;
	_path.clear();

	if (_triangles.empty())
		return false;

	// the triangle `from` is in
	p2t::Triangle *fromTriangle = nullptr;

	for (p2t::Triangle *triangle : _triangles) {
		if (PointInTriangleTolerant(*triangle, from)) {
			fromTriangle = triangle;
			break;
		}
	}

	// `to` is in the triangulation: walk straight to it
	for (p2t::Triangle *triangle : _triangles) {
		if (PointInTriangle(*triangle, target)) {
			_path.push_back(target);
			_pathIndex = 0;
			_destination = target;
			return true;
		}
	}

	if (!fromTriangle)
		return false;

	// `to` is outside: look at where the outline meets the circle round `from` that
	// reaches `to`, on both sides of that
	glm::vec2 center = {(float)from.x, (float)from.y};
	glm::vec2 toVector = {(float)to.x, (float)to.y};
	float radiusF = std::sqrt((center.y - toVector.y) * (center.y - toVector.y) +
	                          (center.x - toVector.x) * (center.x - toVector.x));
	double radius = radiusF;
	std::vector<glm::vec2> candidates;

	for (size_t polygon = 0; polygon < _borders.size(); polygon++) {
		const std::vector<wxPoint> &outline = *_borders[polygon];

		for (size_t index = 0; index < outline.size(); index++) {
			size_t next = (index + 1 == outline.size()) ? 0 : index + 1;
			glm::vec2 edgeFrom = {(float)outline[index].x, (float)outline[index].y};
			glm::vec2 edgeTo = {(float)outline[next].x, (float)outline[next].y};
			glm::vec2 first, second;

			if (!circleLineSegmentIntersection(edgeFrom, edgeTo, center, radius, first, second))
				continue;

			// a point 2 pixels to either side of each intersection, across the edge
			float edgeX = edgeFrom.x - edgeTo.x;
			float edgeY = edgeFrom.y - edgeTo.y;
			float inverse = 1.0f / std::sqrt(edgeY * edgeY + edgeX * edgeX);
			float stepX = edgeX * inverse + edgeX * inverse;
			float stepY = edgeY * inverse + edgeY * inverse;

			candidates.push_back(glm::vec2{stepY + first.x, first.y - stepX});
			candidates.push_back(glm::vec2{first.x - stepY, first.y + stepX});
			candidates.push_back(glm::vec2{stepY + second.x, second.y - stepX});
			candidates.push_back(glm::vec2{second.x - stepY, second.y + stepX});
		}
	}

	if (candidates.empty())
		return false;

	drawPoints.clear();
	for (const glm::vec2 &candidate : candidates)
		drawPoints.push_back(wxPoint{(int)candidate.x, (int)candidate.y});

	// the candidates that lie nearest to the direction of `to` first
	int reference = (int)((double)(std::atan2(center.y - toVector.y, center.x - toVector.x) * 180.0f) / 3.14159265359);
	std::vector<int> deviations;

	for (const glm::vec2 &candidate : candidates) {
		int direction = (int)((double)(std::atan2(center.y - candidate.y, center.x - candidate.x) * 180.0f) /
		                      3.14159265359);

		deviations.push_back(angleDist(direction, reference));
	}

	std::vector<size_t> order;

	argsort(deviations, order);
	for (size_t index : order) {
		if (deviations[index] > 80)
			return false;

		target = wxPoint{(int)candidates[index].x, (int)candidates[index].y};
		for (p2t::Triangle *triangle : _triangles) {
			if (PointInTriangle(*triangle, target)) {
				_path.push_back(target);
				_pathIndex = 0;
				_destination = target;
				return true;
			}
		}
	}
	return false;
}

// Confirmed (asm lines 1376190-1376291): the area of the triangle is worked out
// once and kept.
static double triangleArea(p2t::Triangle &triangle) {
	if (!triangle.areaCalculated) {
		triangle.areaCalculated = true;

		double x0 = triangle.GetPoint(0)->x, y0 = triangle.GetPoint(0)->y;
		double x1 = triangle.GetPoint(1)->x, y1 = triangle.GetPoint(1)->y;
		double x2 = triangle.GetPoint(2)->x, y2 = triangle.GetPoint(2)->y;
		double twice = (x2 - x1) * y0 - y1 * x2 + (y1 - y2) * x0 + x1 * y2;

		triangle.area = (double)(int)twice * 0.5;
	}
	return triangle.area;
}

// The barycentric coordinates of the position in the triangle: s, t and u = 1 - s - t.
static void barycentric(p2t::Triangle &triangle, const wxPoint &position, double &s, double &t, double &u) {
	double x0 = triangle.GetPoint(0)->x, y0 = triangle.GetPoint(0)->y;
	double x1 = triangle.GetPoint(1)->x, y1 = triangle.GetPoint(1)->y;
	double x2 = triangle.GetPoint(2)->x, y2 = triangle.GetPoint(2)->y;
	double inverse = 1.0 / (triangleArea(triangle) * 2.0);
	double px = position.x;
	double py = position.y;

	s = (x2 * y0 - y2 * x0 + (y2 - y0) * px + (x0 - x2) * py) * inverse;
	t = (y1 * x0 - x1 * y0 + (y0 - y1) * px + (x1 - x0) * py) * inverse;
	u = 1.0 - s - t;
}

// Confirmed (asm lines 1376190-1376291)
bool PointInTriangle(p2t::Triangle &triangle, const wxPoint &position) {
	double s, t, u;

	barycentric(triangle, position, s, t, u);
	return s >= 0 && t >= 0 && u >= 0;
}

// Confirmed (asm lines 248964-249065)
bool PointInTriangleTolerant(p2t::Triangle &triangle, const wxPoint &position) {
	double s, t, u;

	barycentric(triangle, position, s, t, u);
	return s >= -0.2 && t >= -0.2 && u >= -0.2;
}

// Confirmed (asm lines 248241-248615): the nearest point of the triangle (the
// standard point/triangle distance construction by regions of the plane,
// which the original follows - region 6 included - in single precision).
point closesPointOnTriangle(p2t::Point *first, p2t::Point *second, p2t::Point *third, const wxPoint &position) {
	// the edges from the first corner
	float e0x = (float)(second->x - first->x);
	float e0y = (float)(second->y - first->y);
	float e1x = (float)(third->x - first->x);
	float e1y = (float)(third->y - first->y);
	float dx = (float)first->x - (float)position.x;
	float dy = (float)first->y - (float)position.y;

	float a = e0x * e0x + e0y * e0y;
	float b = e0x * e1x + e0y * e1y;
	float c = e1x * e1x + e1y * e1y;
	float d = e0x * dx + e0y * dy;
	float e = dx * e1x + dy * e1y;
	float det = a * c - b * b;
	float s0 = b * e - c * d;
	float t0 = b * d - a * e;
	float s, t;

	// the part shared by the regions from which the nearest point is on the edge
	// between the second and third corner
	bool edgeSecondThird = false;
	float numer = 0.0f;

	if (det > s0 + t0) {
		if (0 > s0) {
			if (0 > t0 && 0 > d) {
				s = -d / a;
				t = 0;
				if (!(s > 0))
					s = 0;
				else if (!(1.0f > s))
					s = 1.0f;
			} else {
				t = -e / c;
				if (!(t > 0))
					t = 0;
				else if (!(1.0f > t))
					t = 1.0f;
				s = 0;
			}
		} else if (0 > t0) {
			s = -d / a;
			t = 0;
			if (!(s > 0))
				s = 0;
			else if (!(1.0f > s))
				s = 1.0f;
		} else {
			float inverse = 1.0f / det;

			s = s0 * inverse;
			t = t0 * inverse;
		}
	} else if (0 > s0) {
		float tmp0 = b + d;
		float tmp1 = c + e;

		if (tmp1 > tmp0) {
			numer = tmp1 - tmp0;
			edgeSecondThird = true;
		} else {
			s = 0;
			t = -e / c;
			if (!(t > 0))
				t = 0;
			else if (!(1.0f > t))
				t = 1.0f;
		}
	} else if (0 > t0) {
		if (a + d > b + e) {
			numer = ((e + c) - b) - d;
			edgeSecondThird = true;
		} else {
			t = 0;
			s = -e / c;
			if (!(s > 0))
				s = 0;
			else if (!(1.0f > s))
				s = 1.0f;
		}
	} else {
		numer = ((e + c) - b) - d;
		edgeSecondThird = true;
	}

	if (edgeSecondThird) {
		float denom = (a - (b + b)) + c;

		s = numer / denom;
		if (!(s > 0)) {
			t = 1.0f;
			s = 0;
		} else if (1.0f > s) {
			t = 1.0f - s;
		} else {
			t = 0;
			s = 1.0f;
		}
	}

	double y = (double)(e0y * s) + first->y;
	double x = (double)(s * e0x) + first->x;

	y += (double)(e1y * t);
	x += (double)(t * e1x);
	return point{(float)x, (float)y};
}

// Confirmed (asm lines 249196-249466): the intersections of the line through the
// two points with the circle, as parameters t along the segment (0 to 1 is the
// segment); only those on the segment count.
bool circleLineSegmentIntersection(const glm::vec2 &first, const glm::vec2 &second, const glm::vec2 &center,
                                   double radius, glm::vec2 &intersection1, glm::vec2 &intersection2) {
	float dx = second.x - first.x;
	float dy = second.y - first.y;
	float ex = first.x - center.x;
	float ey = first.y - center.y;
	float b = dy * ey + dx * ex;
	float a = dy * dy + dx * dx;
	float distance = std::sqrt(ey * ey + ex * ex);
	double discriminant = (double)b * (double)b - ((double)distance * (double)distance - radius * radius) * (double)a;

	if (0 > discriminant)
		return false;

	double root = std::sqrt(discriminant);
	double t1 = ((double)(-(dy * ey + dx * ex)) + root) / (double)a;
	double t2 = ((double)(-(ey * dy + ex * dx)) - root) / (double)a;
	bool outside1 = (0 > t1) || (t1 > 1.0);
	bool outside2 = (0 > t2) || (t2 > 1.0);

	float f1 = (float)t1;
	float f2 = (float)t2;

	intersection1.x = f1 * dx + first.x;
	intersection1.y = f1 * dy + first.y;
	intersection2.x = f2 * dx + first.x;
	intersection2.y = f2 * dy + first.y;

	if (!outside2 && outside1) {
		// only the second one is on the segment
		intersection1 = intersection2;
		return true;
	}
	if (!outside2)
		return true;
	if (outside1)
		return false;
	// only the first one is on the segment
	intersection2 = intersection1;
	return true;
}

// Confirmed (asm lines 249507-249539)
int angleDist(int first, int second) {
	int difference = first - second;

	return std::min(std::min(std::abs(difference - 360), std::abs(difference + 360)), std::abs(difference));
}

static bool lessByValue(const std::pair<size_t, const int *> &first, const std::pair<size_t, const int *> &second) {
	return *first.second < *second.second;
}

// Confirmed (asm lines 256618-256916)
void argsort(const std::vector<int> &values, std::vector<size_t> &indices) {
	std::vector<std::pair<size_t, const int *> > pairs;

	for (size_t i = 0; i < values.size(); i++)
		pairs.push_back(std::make_pair(i, &values[i]));
	std::sort(pairs.begin(), pairs.end(), lessByValue);

	for (const std::pair<size_t, const int *> &pair : pairs)
		indices.push_back(pair.first);
}

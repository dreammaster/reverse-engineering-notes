// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 247325-254196): the way system of a scene
// (what a TGCharacter walks on). It is made from a scene's TTWaySystem data
// object: an outline (kWaySystemBorder: one polygon for the walkable area and
// more inside it that are holes) and the way points (kWaySystemPoints) that are
// linked to each other (kPointRelations). From the outline it builds a
// constrained Delaunay triangulation (p2t), used to keep a position inside the
// walkable area and to walk to places outside it.
//
// A way is found in one of two ways:
//  - CalculateWay(): over the way points: the points that can be seen from the
//    start, a search through their links to the point the target is nearest to
//    (SearchWay()), then the way points trimmed to the first one the target can
//    be seen from. Falls back to a straight line, or shortestPath() round the
//    corners of the outline, when no way point can be seen.
//  - CalculateWayTriangles(): for a target outside the triangulation: the point
//    of the outline nearest in direction.
// GetNextPosition() then moves a position along the way a step at a time.
//
// Original layout: the TPointSize base (0x18 bytes), +0x18 the outline polygons,
// +0x30 the way points, +0x48 the way (points to walk to), +0x60 the index in it,
// +0x64 whether the direction has been worked out, +0x68/+0x6C the length of the
// best way found so far / of the one being searched, +0x70 the way point the way
// is to end at, +0x78 the way point route being searched, +0x90 the
// triangulations, +0xA8 all their triangles, +0xC0 the points of them,
// +0xD8 the destination, +0xE0/+0xE1 the two flags below.
//
// Not reconstructed: a handful of leftover geometry helpers the binary has next
// to these but nothing calls (Len, Normalize, Distance, LinePointDistance,
// LinePointPoint, dot, _point, toVec2, MapPointToLine and others).
#pragma once

#include <vector>

#include "GlmStub.h"
#include "TPointSize.h"
#include "TPolygonList.h"
#include "WxStub.h"
#include "datastruct/vlist.h"
#include "datastruct/visobjref.h"
#include "p2t/p2t.h"

class TVisionaireObject;

class TGWaySystem : public TPointSize {
public:
	TGWaySystem();
	~TGWaySystem();

	/** Builds the outline polygons, the way points and the triangulation from a
	 *  TTWaySystem object; false when it has no outline (and fewer than three
	 *  points). */
	bool SetWaySystem(const TVisObjRef &waySystem);

	/** Finds a way from `from` to `to` (both positions on the scene) that ends at
	 *  the way point `wayPoint`; false when there is none. */
	bool CalculateWay(const TVisObjRef &wayPoint, const wxPoint &from, const wxPoint &to);
	/** The way to a `to` that is outside the walkable area: to the point of its
	 *  outline that lies nearest the direction of `to` (within 80 degrees); true
	 *  also when `to` is inside the area (the way is `to` itself). */
	bool CalculateWayTriangles(const wxPoint &from, const wxPoint &to);
	/** Moves `position` by `speed` along the way; sets `angle` to the direction
	 *  (degrees) it is going. True when the destination has been reached. */
	bool GetNextPosition(wxRealPoint &position, int *angle, float speed);

	/** A position that is inside the walkable area: the one given, or the nearest
	 *  point of the nearest triangle. */
	wxPoint CheckPosition(const wxPoint &position);

	/** Walk straight to `destination` (no way). */
	void SetDestination(const wxPoint &destination);
	/** Where the way ends: the last way point when walking along way points,
	 *  else the destination ((-1, -1) when the way is empty). */
	wxPoint GetDestination() const;
	/** The direction (degrees) from `position` to the destination. */
	int GetDirectionToNextDestination(const wxPoint &position) const;
	bool IsWalkingToWayPoint() const;

	/** Whether the segment crosses the outline. */
	bool CheckCrossBorders(const wxPoint &first, const wxPoint &second) const;
	/** Whether two way points are linked, directly or through others. */
	bool IsConnected(const TVisObjRef &first, const TVisObjRef &second) const;

	/** The way point nearest to the position. */
	TVisObjRef GetPointNextTo(const wxPoint &position);
	/** The way point nearest to the position that can be seen from it; `points`
	 *  gets every way point that can be. */
	TVisObjRef GetAllPointsNextTo(const wxPoint &position, TVList &points);

	// The size system of TPointSize, forwarded.
	void SetNoLines(int lines);
	int GetNoLines() const;
	float GetCalculatedSize(const wxPoint &position) const;
	void UpdateSizeSystem();

	/** The comparison the way points are sorted by: nearer to destinationForCmp. */
	static bool ClosestPoint(const TVisionaireObject *first, const TVisionaireObject *second);
	/** The position ClosestPoint() measures from (the end way point's). */
	static wxPoint destinationForCmp;

private:
	/** The depth first search over the linked way points, from `wayPoint` to
	 *  _targetWayPoint; keeps the shortest way found (in _path). */
	bool SearchWay(TVisionaireObject *wayPoint);

	TPolygonList _borders;                  // +0x18, the outline: walkable area and holes
	TVList _wayPoints;                      // +0x30
	std::vector<wxPoint> _path;             // +0x48, the points to walk to
	int _pathIndex;                         // +0x60, the one being walked to
	bool _angleKnown;                       // +0x64, the direction of the leg is known
	int _bestLength;                        // +0x68, -1: no way found yet
	int _currentLength;                     // +0x6C
	TVisObjRef _targetWayPoint;             // +0x70
	TVList _route;                          // +0x78, the way points searched through
	std::vector<p2t::CDT *> _triangulations; // +0x90
	std::vector<p2t::Triangle *> _triangles; // +0xA8
	std::vector<std::vector<p2t::Point *> > _trianglePoints; // +0xC0
	wxPoint _destination;                   // +0xD8
	bool _headingForDestination;            // +0xE0, walking straight to _destination
	bool _walkingToWayPoint;                // +0xE1, the way is along way points
};

/** Positions of the candidates CalculateWayTriangles() considered (a debug aid:
 *  the overlay draws them). */
extern std::vector<wxPoint> drawPoints;

/** A float pair (the original's `point`). */
struct point {
	float x, y;
};

/** Whether the point is inside the triangle (the sign of its barycentric
 *  coordinates); caches the area in the triangle. */
bool PointInTriangle(p2t::Triangle &triangle, const wxPoint &position);
/** The same with a tolerance of 0.2 outside the edges. */
bool PointInTriangleTolerant(p2t::Triangle &triangle, const wxPoint &position);
/** The point of the triangle nearest to the position. */
point closesPointOnTriangle(p2t::Point *first, p2t::Point *second, p2t::Point *third, const wxPoint &position);
/** The intersections of the segment p1-p2 with the circle of the given radius
 *  around `center`; both set when it crosses twice (twice the same when once);
 *  false when none is on the segment. */
bool circleLineSegmentIntersection(const glm::vec2 &first, const glm::vec2 &second, const glm::vec2 &center,
                                   double radius, glm::vec2 &intersection1, glm::vec2 &intersection2);
/** The difference of two directions in degrees (0 to 180). */
int angleDist(int first, int second);
/** The indices of the values in ascending order of the values. */
void argsort(const std::vector<int> &values, std::vector<size_t> &indices);

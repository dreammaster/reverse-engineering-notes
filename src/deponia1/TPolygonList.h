// Not yet assert-confirmed to a specific file; stays at the top level.
//
// Confirmed (Deponia_Linux.asm lines 526053-526453, 531923-532260): TPolygonList
// is a list of polygons, each one an owned std::vector<wxPoint> - a plain
// std::vector<std::vector<wxPoint> *> (begin/end pointers at +0/+8) whose
// clear()/erase()/destructor/operator= delete the polygons it owns and
// push_back() stores a copy. (pop_back() only drops the pointer, it does not
// free the polygon - reproduced.)
//
// The free functions below are the polygon helpers the way system and the
// editor use (Deponia_Linux.asm lines 247392-248160, 1375215-1377690). A point
// list holding several polygons separates them with a point at (-10000,
// -10000).
#pragma once

#include <vector>

#include "WxStub.h"

class TPolygonList {
public:
	typedef std::vector<std::vector<wxPoint> *>::iterator iterator;
	typedef std::vector<std::vector<wxPoint> *>::const_iterator const_iterator;
	typedef std::vector<std::vector<wxPoint> *>::reverse_iterator reverse_iterator;

	TPolygonList() = default;
	TPolygonList(const TPolygonList &other);
	~TPolygonList();

	TPolygonList &operator=(const TPolygonList &other);

	/** Deletes the polygons. */
	void clear();
	/** Deletes the polygon at the iterator and removes it. */
	void erase(iterator position);
	void push_back(const std::vector<wxPoint> &polygon);
	/** Removes the last polygon from the list WITHOUT freeing it. */
	void pop_back() {
		_polygons.pop_back();
	}

	std::vector<wxPoint> *&at(int index) {
		return _polygons.at(index);
	}
	std::vector<wxPoint> *&front() {
		return _polygons.front();
	}
	std::vector<wxPoint> *&back() {
		return _polygons.back();
	}
	iterator begin() {
		return _polygons.begin();
	}
	iterator end() {
		return _polygons.end();
	}
	const_iterator begin() const {
		return _polygons.begin();
	}
	const_iterator end() const {
		return _polygons.end();
	}
	reverse_iterator rbegin() {
		return _polygons.rbegin();
	}
	reverse_iterator rend() {
		return _polygons.rend();
	}
	unsigned long size() const {
		return _polygons.size();
	}
	bool empty() const {
		return _polygons.empty();
	}
	std::vector<wxPoint> *&operator[](unsigned long index) {
		return _polygons[index];
	}
	std::vector<wxPoint> *const &operator[](unsigned long index) const {
		return _polygons[index];
	}

private:
	std::vector<std::vector<wxPoint> *> _polygons;
};

/** The marker between two polygons in a flat point list. */
extern const wxPoint kPolygonSeparator;

/** Splits a flat point list at the separators into polygons; false if a polygon
 *  has fewer than three points (it is created anyway). */
bool CreatePolygonsFromPointList(const std::vector<wxPoint> &points, TPolygonList &outPolygons);
/** The opposite: all the polygons' points with a separator between polygons. */
void CreatePointListFromPolygons(const TPolygonList &polygons, std::vector<wxPoint> &outPoints);

/** The bounding rectangle of all the points (width and height are inclusive:
 *  maximum - minimum + 1); empty (and asserted against) if there are none. */
wxRect GetBoundingBox(const TPolygonList &polygons);
wxRect GetBoundingBox(const std::vector<wxPoint> &points);

/** Whether the segments a-b and c-d intersect. */
bool LinesCut(const wxPoint &a, const wxPoint &b, const wxPoint &c, const wxPoint &d);
/** Whether the segment a-b crosses any edge of the polygon (closed: the last
 *  point is joined to the first). */
bool LineCuts(const wxPoint &a, const wxPoint &b, const std::vector<wxPoint> &polygon);
/** The same for all the polygons. */
bool LineCuts(const wxPoint &a, const wxPoint &b, const TPolygonList &polygons);
/** How many edges of the polygon the segment a-b crosses (none for fewer than
 *  three points). */
int GetNumCuts(const wxPoint &a, const wxPoint &b, const std::vector<wxPoint> &polygon);
/** Whether the segment from (x1, y1) to (x2, y2) lies inside the walkable area:
 *  it crosses no polygon edge (an edge it runs along has to lie inside it, and
 *  one that is exactly the segment counts) and its middle is inside. */
bool LineInPolygonSet(double x1, double y1, double x2, double y2, TPolygonList &polygons);
/** The corners of the shortest way from (x1, y1) to (x2, y2) through the area
 *  (polygon vertices that can see each other), without the start and the end
 *  point; when they see each other the end point alone. False when the start is
 *  not in the area, or there is no way. */
bool shortestPath(double x1, double y1, double x2, double y2, TPolygonList &polygons, std::vector<wxPoint> &path);
/** Even-odd test of a point against one polygon (a ray to the left, counting
 *  the edges it crosses with LinesCut). */
bool PointInPolygon(int x, int y, std::vector<wxPoint> &polygon);
/** The same over all the polygons (so a polygon inside another is a hole). */
bool PointInPolygonList(int x, int y, TPolygonList &polygons);
/** The number of polygons the point is in, odd = inside. */
bool IsPointInsidePolygon(const wxPoint &point, const TPolygonList &polygons);
/** The same for one polygon (the original is a float scan-line version; this is
 *  the even-odd equivalent, which differs only for points exactly on an edge). */
bool IsPointInsidePolygon(const wxPoint &point, const std::vector<wxPoint> &polygon);
double calcDist(double x1, double y1, double x2, double y2);

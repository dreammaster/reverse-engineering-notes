// poly2tri (p2t): the constrained Delaunay triangulation library the engine's
// way system uses (TGWaySystem::CalculateWayTriangles builds a p2t::CDT from the
// walkable area's outline and its holes and takes the triangles it returns).
//
// This is the open-source library (poly2tri 1.3, sweep-line CDT), not code
// reconstructed instruction by instruction: the binary's copy has the same
// functions as that version (p2t::Sweep with FillRightAboveEdgeEvent ... /
// FinalizationPolygon / HoleAngle / BasinAngle, p2t::SweepContext::MeshClean,
// Orient2d with its 1e-12 epsilon - checked against Deponia_Linux.asm lines
// 2309547-2316294, 2400171-2401904), so the library's own algorithm is written
// out here, with the public-field names it has (x, y, ...) adjusted to this
// project's conventions. The class and method names are the original's.
#pragma once

#include <list>
#include <vector>

namespace p2t {

struct Edge;

struct Point {
	double x, y;

	/** The edges that end in this point (the edges are owned by the context). */
	std::vector<Edge *> edgeList;

	Point() : x(0.0), y(0.0) {
	}
	Point(double x, double y) : x(x), y(y) {
	}

	bool operator==(const Point &other) const {
		return x == other.x && y == other.y;
	}
	bool operator!=(const Point &other) const {
		return !(*this == other);
	}
};

/** A constrained edge: `p` is always the lower end (smaller y, then smaller x). */
struct Edge {
	Point *p, *q;

	Edge(Point &p1, Point &p2);
};

class Triangle {
public:
	/** Which of the three edges are constrained (part of the outline) / Delaunay
	 *  edges (only while a legalisation is going on); edge i is the one opposite
	 *  point i. */
	bool constrainedEdge[3];
	bool delaunayEdge[3];

	/** The way system's PointInTriangle() caches the triangle's area here: the
	 *  binary's Triangle has a flag (+6) and a double (+8) the library does not. */
	bool areaCalculated = false;
	double area = 0.0;

	Triangle(Point &a, Point &b, Point &c);

	Point *GetPoint(int index);
	Point *PointCW(Point &point);
	Point *PointCCW(Point &point);
	Point *OppositePoint(Triangle &t, Point &p);

	Triangle *GetNeighbor(int index);
	void MarkNeighbor(Point *p1, Point *p2, Triangle *t);
	void MarkNeighbor(Triangle &t);
	void ClearNeighbors();
	void ClearDelunayEdges();

	bool Contains(Point *p);
	bool Contains(Point *p, Point *q);
	/** Rotates the triangle clockwise around its first point / around `opoint`
	 *  replacing it with `npoint` (the flip of two triangles). */
	void Legalize(Point &point);
	void Legalize(Point &opoint, Point &npoint);

	int Index(const Point *p);
	int EdgeIndex(const Point *p1, const Point *p2);

	void MarkConstrainedEdge(int index);
	void MarkConstrainedEdge(Edge &edge);
	void MarkConstrainedEdge(Point *p, Point *q);

	Triangle *NeighborCW(Point &point);
	Triangle *NeighborCCW(Point &point);
	bool GetConstrainedEdgeCCW(Point &p);
	bool GetConstrainedEdgeCW(Point &p);
	void SetConstrainedEdgeCCW(Point &p, bool ce);
	void SetConstrainedEdgeCW(Point &p, bool ce);
	bool GetDelunayEdgeCCW(Point &p);
	bool GetDelunayEdgeCW(Point &p);
	void SetDelunayEdgeCCW(Point &p, bool e);
	void SetDelunayEdgeCW(Point &p, bool e);

	Triangle &NeighborAcross(Point &opoint);

	bool IsInterior() const {
		return _interior;
	}
	void IsInterior(bool interior) {
		_interior = interior;
	}

private:
	Point *_points[3];
	Triangle *_neighbors[3];
	bool _interior;
};

/** A node of the advancing front (a doubly linked list of points). */
struct Node {
	Point *point;
	Triangle *triangle;
	Node *next;
	Node *prev;
	double value;

	explicit Node(Point &p) : point(&p), triangle(nullptr), next(nullptr), prev(nullptr), value(p.x) {
	}
	Node(Point &p, Triangle &t) : point(&p), triangle(&t), next(nullptr), prev(nullptr), value(p.x) {
	}
};

class AdvancingFront {
public:
	AdvancingFront(Node &head, Node &tail);

	Node *head() {
		return _head;
	}
	Node *tail() {
		return _tail;
	}
	Node *LocateNode(double x);
	Node *LocatePoint(const Point *point);

private:
	Node *_head, *_tail, *_searchNode;
};

class SweepContext {
public:
	explicit SweepContext(std::vector<Point *> polyline);
	~SweepContext();

	/** What the current edge event works on (the edge and its direction). */
	struct EdgeEventState {
		Edge *constrainedEdge = nullptr;
		bool right = false;
	};
	/** The basin that is being filled. */
	struct Basin {
		Node *leftNode = nullptr;
		Node *bottomNode = nullptr;
		Node *rightNode = nullptr;
		double width = 0.0;
		bool leftHighest = false;
	};

	EdgeEventState edgeEvent;
	Basin basin;

	void AddHole(std::vector<Point *> polyline);
	void AddPoint(Point *point);
	AdvancingFront *front() {
		return _front;
	}
	int pointCount() const {
		return (int)_points.size();
	}
	Point *GetPoint(const int &index);
	std::vector<Triangle *> GetTriangles();
	std::list<Triangle *> GetMap();

	void InitTriangulation();
	void CreateAdvancingFront(std::vector<Node *> nodes);
	void AddToMap(Triangle *triangle);
	Node &LocateNode(Point &point);
	void RemoveNode(Node *node);
	void MapTriangleToNodes(Triangle &t);
	void RemoveFromMap(Triangle *triangle);
	void MeshClean(Triangle &triangle);

private:
	void InitEdges(std::vector<Point *> polyline);

	std::vector<Edge *> _edgeList;
	std::vector<Triangle *> _triangles;
	std::list<Triangle *> _map;
	std::vector<Point *> _points;
	AdvancingFront *_front;
	Point *_head;
	Point *_tail;
	Node *_afHead, *_afMiddle, *_afTail;
};

class Sweep {
public:
	~Sweep();

	void Triangulate(SweepContext &tcx);

private:
	void SweepPoints(SweepContext &tcx);
	Node &PointEvent(SweepContext &tcx, Point &point);
	void EdgeEvent(SweepContext &tcx, Edge *edge, Node *node);
	void EdgeEvent(SweepContext &tcx, Point &ep, Point &eq, Triangle *triangle, Point &point);
	Node &NewFrontTriangle(SweepContext &tcx, Point &point, Node &node);
	void FillAdvancingFront(SweepContext &tcx, Node &n);
	double HoleAngle(Node &node);
	double BasinAngle(Node &node);
	void Fill(SweepContext &tcx, Node &node);
	void FillBasin(SweepContext &tcx, Node &node);
	void FillBasinReq(SweepContext &tcx, Node *node);
	bool IsShallow(SweepContext &tcx, Node &node);
	bool Legalize(SweepContext &tcx, Triangle &t);
	bool Incircle(Point &pa, Point &pb, Point &pc, Point &pd);
	void RotateTrianglePair(Triangle &t, Point &p, Triangle &ot, Point &op);
	void FinalizationPolygon(SweepContext &tcx);
	bool IsEdgeSideOfTriangle(Triangle &triangle, Point &ep, Point &eq);
	void FillEdgeEvent(SweepContext &tcx, Edge *edge, Node *node);
	void FillRightAboveEdgeEvent(SweepContext &tcx, Edge *edge, Node *node);
	void FillRightBelowEdgeEvent(SweepContext &tcx, Edge *edge, Node &node);
	void FillRightConcaveEdgeEvent(SweepContext &tcx, Edge *edge, Node &node);
	void FillRightConvexEdgeEvent(SweepContext &tcx, Edge *edge, Node &node);
	void FillLeftAboveEdgeEvent(SweepContext &tcx, Edge *edge, Node *node);
	void FillLeftBelowEdgeEvent(SweepContext &tcx, Edge *edge, Node &node);
	void FillLeftConcaveEdgeEvent(SweepContext &tcx, Edge *edge, Node &node);
	void FillLeftConvexEdgeEvent(SweepContext &tcx, Edge *edge, Node &node);
	void FlipEdgeEvent(SweepContext &tcx, Point &ep, Point &eq, Triangle *t, Point &p);
	Triangle &NextFlipTriangle(SweepContext &tcx, int o, Triangle &t, Triangle &ot, Point &p, Point &op);
	Point &NextFlipPoint(Point &ep, Point &eq, Triangle &ot, Point &op);
	void FlipScanEdgeEvent(SweepContext &tcx, Point &ep, Point &eq, Triangle &flipTriangle, Triangle &t, Point &p);

	std::vector<Node *> _nodes;
};

/** The constrained Delaunay triangulation of a polygon (and its holes). */
class CDT {
public:
	explicit CDT(std::vector<Point *> polyline);
	~CDT();

	void AddHole(std::vector<Point *> polyline);
	void AddPoint(Point *point);
	void Triangulate();
	std::vector<Triangle *> GetTriangles();
	std::list<Triangle *> GetMap();

private:
	SweepContext *_sweepContext;
	Sweep *_sweep;
};

enum Orientation {
	CW,
	CCW,
	COLLINEAR
};

/** Which way pa, pb, pc turn (with a 1e-12 tolerance for collinear). */
Orientation Orient2d(Point &pa, Point &pb, Point &pc);
bool InScanArea(Point &pa, Point &pb, Point &pc, Point &pd);
/** The points' sort order: by y, then by x. */
bool cmp(const Point *a, const Point *b);

}

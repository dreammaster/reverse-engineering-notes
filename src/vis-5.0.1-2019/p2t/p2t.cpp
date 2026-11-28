#include "p2t/p2t.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <stdexcept>

namespace p2t {

static const double kEpsilon = 1e-12;
static const double kAlpha = 0.3;
static const double kPi = 3.14159265358979323846;
static const double kPiDiv2 = 1.57079632679489661923;
static const double kPi3Div4 = 3 * kPi / 4;

Orientation Orient2d(Point &pa, Point &pb, Point &pc) {
	double detleft = (pa.x - pc.x) * (pb.y - pc.y);
	double detright = (pa.y - pc.y) * (pb.x - pc.x);
	double val = detleft - detright;

	if (val > -kEpsilon && val < kEpsilon)
		return COLLINEAR;
	if (val > 0)
		return CCW;
	return CW;
}

bool InScanArea(Point &pa, Point &pb, Point &pc, Point &pd) {
	double pdx = pd.x;
	double pdy = pd.y;
	double adx = pa.x - pdx;
	double ady = pa.y - pdy;
	double bdx = pb.x - pdx;
	double bdy = pb.y - pdy;

	double oabd = adx * bdy - bdx * ady;
	if (oabd <= kEpsilon)
		return false;

	double cdx = pc.x - pdx;
	double cdy = pc.y - pdy;

	double ocad = cdx * ady - adx * cdy;
	if (ocad <= kEpsilon)
		return false;

	return true;
}

bool cmp(const Point *a, const Point *b) {
	if (a->y < b->y)
		return true;
	if (a->y == b->y)
		return a->x < b->x;
	return false;
}

Edge::Edge(Point &p1, Point &p2) : p(&p1), q(&p2) {
	if (p1.y > p2.y) {
		q = &p1;
		p = &p2;
	} else if (p1.y == p2.y) {
		if (p1.x > p2.x) {
			q = &p1;
			p = &p2;
		} else if (p1.x == p2.x) {
			// repeated point
			assert(false);
		}
	}

	q->edgeList.push_back(this);
}

Triangle::Triangle(Point &a, Point &b, Point &c) : _interior(false) {
	_points[0] = &a;
	_points[1] = &b;
	_points[2] = &c;
	_neighbors[0] = _neighbors[1] = _neighbors[2] = nullptr;
	constrainedEdge[0] = constrainedEdge[1] = constrainedEdge[2] = false;
	delaunayEdge[0] = delaunayEdge[1] = delaunayEdge[2] = false;
}

Point *Triangle::GetPoint(int index) {
	return _points[index];
}

Triangle *Triangle::GetNeighbor(int index) {
	return _neighbors[index];
}

bool Triangle::Contains(Point *p) {
	return p == _points[0] || p == _points[1] || p == _points[2];
}

bool Triangle::Contains(Point *p, Point *q) {
	return Contains(p) && Contains(q);
}

void Triangle::MarkNeighbor(Point *p1, Point *p2, Triangle *t) {
	if ((p1 == _points[2] && p2 == _points[1]) || (p1 == _points[1] && p2 == _points[2]))
		_neighbors[0] = t;
	else if ((p1 == _points[0] && p2 == _points[2]) || (p1 == _points[2] && p2 == _points[0]))
		_neighbors[1] = t;
	else if ((p1 == _points[0] && p2 == _points[1]) || (p1 == _points[1] && p2 == _points[0]))
		_neighbors[2] = t;
	else
		assert(false);
}

void Triangle::MarkNeighbor(Triangle &t) {
	if (t.Contains(_points[1], _points[2])) {
		_neighbors[0] = &t;
		t.MarkNeighbor(_points[1], _points[2], this);
	} else if (t.Contains(_points[0], _points[2])) {
		_neighbors[1] = &t;
		t.MarkNeighbor(_points[0], _points[2], this);
	} else if (t.Contains(_points[0], _points[1])) {
		_neighbors[2] = &t;
		t.MarkNeighbor(_points[0], _points[1], this);
	}
}

void Triangle::ClearNeighbors() {
	_neighbors[0] = _neighbors[1] = _neighbors[2] = nullptr;
}

void Triangle::ClearDelunayEdges() {
	delaunayEdge[0] = delaunayEdge[1] = delaunayEdge[2] = false;
}

Point *Triangle::OppositePoint(Triangle &t, Point &p) {
	Point *cw = t.PointCW(p);
	return PointCW(*cw);
}

void Triangle::Legalize(Point &point) {
	_points[1] = _points[0];
	_points[0] = _points[2];
	_points[2] = &point;
}

void Triangle::Legalize(Point &opoint, Point &npoint) {
	if (&opoint == _points[0]) {
		_points[1] = _points[0];
		_points[0] = _points[2];
		_points[2] = &npoint;
	} else if (&opoint == _points[1]) {
		_points[2] = _points[1];
		_points[1] = _points[0];
		_points[0] = &npoint;
	} else if (&opoint == _points[2]) {
		_points[0] = _points[2];
		_points[2] = _points[1];
		_points[1] = &npoint;
	} else {
		assert(false);
	}
}

int Triangle::Index(const Point *p) {
	if (p == _points[0])
		return 0;
	if (p == _points[1])
		return 1;
	if (p == _points[2])
		return 2;

	assert(false);
	return -1;
}

int Triangle::EdgeIndex(const Point *p1, const Point *p2) {
	if (_points[0] == p1) {
		if (_points[1] == p2)
			return 2;
		if (_points[2] == p2)
			return 1;
	} else if (_points[1] == p1) {
		if (_points[2] == p2)
			return 0;
		if (_points[0] == p2)
			return 2;
	} else if (_points[2] == p1) {
		if (_points[0] == p2)
			return 1;
		if (_points[1] == p2)
			return 0;
	}
	return -1;
}

void Triangle::MarkConstrainedEdge(int index) {
	constrainedEdge[index] = true;
}

void Triangle::MarkConstrainedEdge(Edge &edge) {
	MarkConstrainedEdge(edge.p, edge.q);
}

void Triangle::MarkConstrainedEdge(Point *p, Point *q) {
	if ((q == _points[0] && p == _points[1]) || (q == _points[1] && p == _points[0]))
		constrainedEdge[2] = true;
	else if ((q == _points[0] && p == _points[2]) || (q == _points[2] && p == _points[0]))
		constrainedEdge[1] = true;
	else if ((q == _points[1] && p == _points[2]) || (q == _points[2] && p == _points[1]))
		constrainedEdge[0] = true;
}

Point *Triangle::PointCW(Point &point) {
	if (&point == _points[0])
		return _points[2];
	if (&point == _points[1])
		return _points[0];
	if (&point == _points[2])
		return _points[1];

	assert(false);
	return nullptr;
}

Point *Triangle::PointCCW(Point &point) {
	if (&point == _points[0])
		return _points[1];
	if (&point == _points[1])
		return _points[2];
	if (&point == _points[2])
		return _points[0];

	assert(false);
	return nullptr;
}

Triangle *Triangle::NeighborCW(Point &point) {
	if (&point == _points[0])
		return _neighbors[1];
	if (&point == _points[1])
		return _neighbors[2];
	return _neighbors[0];
}

Triangle *Triangle::NeighborCCW(Point &point) {
	if (&point == _points[0])
		return _neighbors[2];
	if (&point == _points[1])
		return _neighbors[0];
	return _neighbors[1];
}

bool Triangle::GetConstrainedEdgeCCW(Point &p) {
	if (&p == _points[0])
		return constrainedEdge[2];
	if (&p == _points[1])
		return constrainedEdge[0];
	return constrainedEdge[1];
}

bool Triangle::GetConstrainedEdgeCW(Point &p) {
	if (&p == _points[0])
		return constrainedEdge[1];
	if (&p == _points[1])
		return constrainedEdge[2];
	return constrainedEdge[0];
}

void Triangle::SetConstrainedEdgeCCW(Point &p, bool ce) {
	if (&p == _points[0])
		constrainedEdge[2] = ce;
	else if (&p == _points[1])
		constrainedEdge[0] = ce;
	else
		constrainedEdge[1] = ce;
}

void Triangle::SetConstrainedEdgeCW(Point &p, bool ce) {
	if (&p == _points[0])
		constrainedEdge[1] = ce;
	else if (&p == _points[1])
		constrainedEdge[2] = ce;
	else
		constrainedEdge[0] = ce;
}

bool Triangle::GetDelunayEdgeCCW(Point &p) {
	if (&p == _points[0])
		return delaunayEdge[2];
	if (&p == _points[1])
		return delaunayEdge[0];
	return delaunayEdge[1];
}

bool Triangle::GetDelunayEdgeCW(Point &p) {
	if (&p == _points[0])
		return delaunayEdge[1];
	if (&p == _points[1])
		return delaunayEdge[2];
	return delaunayEdge[0];
}

void Triangle::SetDelunayEdgeCCW(Point &p, bool e) {
	if (&p == _points[0])
		delaunayEdge[2] = e;
	else if (&p == _points[1])
		delaunayEdge[0] = e;
	else
		delaunayEdge[1] = e;
}

void Triangle::SetDelunayEdgeCW(Point &p, bool e) {
	if (&p == _points[0])
		delaunayEdge[1] = e;
	else if (&p == _points[1])
		delaunayEdge[2] = e;
	else
		delaunayEdge[0] = e;
}

Triangle &Triangle::NeighborAcross(Point &opoint) {
	if (&opoint == _points[0])
		return *_neighbors[0];
	if (&opoint == _points[1])
		return *_neighbors[1];
	return *_neighbors[2];
}

AdvancingFront::AdvancingFront(Node &head, Node &tail) : _head(&head), _tail(&tail), _searchNode(&head) {
}

Node *AdvancingFront::LocateNode(double x) {
	Node *node = _searchNode;

	if (x < node->value) {
		while ((node = node->prev) != nullptr) {
			if (x >= node->value) {
				_searchNode = node;
				return node;
			}
		}
	} else {
		while ((node = node->next) != nullptr) {
			if (x < node->value) {
				_searchNode = node->prev;
				return node->prev;
			}
		}
	}
	return nullptr;
}

Node *AdvancingFront::LocatePoint(const Point *point) {
	const double px = point->x;
	Node *node = _searchNode;
	const double nx = node->point->x;

	if (px == nx) {
		if (point != node->point) {
			// there may be two nodes with the same x for a short time
			if (point == node->prev->point)
				node = node->prev;
			else if (point == node->next->point)
				node = node->next;
			else
				assert(false);
		}
	} else if (px < nx) {
		while ((node = node->prev) != nullptr) {
			if (point == node->point)
				break;
		}
	} else {
		while ((node = node->next) != nullptr) {
			if (point == node->point)
				break;
		}
	}

	if (node)
		_searchNode = node;
	return node;
}

SweepContext::SweepContext(std::vector<Point *> polyline)
	: _points(polyline), _front(nullptr), _head(nullptr), _tail(nullptr), _afHead(nullptr), _afMiddle(nullptr),
	  _afTail(nullptr) {
	InitEdges(_points);
}

SweepContext::~SweepContext() {
	delete _head;
	delete _tail;
	delete _front;
	delete _afHead;
	delete _afMiddle;
	delete _afTail;

	for (Triangle *triangle : _map)
		delete triangle;
	for (Edge *edge : _edgeList)
		delete edge;
}

void SweepContext::AddHole(std::vector<Point *> polyline) {
	InitEdges(polyline);
	for (size_t i = 0; i < polyline.size(); i++)
		_points.push_back(polyline[i]);
}

void SweepContext::AddPoint(Point *point) {
	_points.push_back(point);
}

std::vector<Triangle *> SweepContext::GetTriangles() {
	return _triangles;
}

std::list<Triangle *> SweepContext::GetMap() {
	return _map;
}

void SweepContext::InitTriangulation() {
	double xmax(_points[0]->x), xmin(_points[0]->x);
	double ymax(_points[0]->y), ymin(_points[0]->y);

	for (size_t i = 0; i < _points.size(); i++) {
		Point &p = *_points[i];

		if (p.x > xmax)
			xmax = p.x;
		if (p.x < xmin)
			xmin = p.x;
		if (p.y > ymax)
			ymax = p.y;
		if (p.y < ymin)
			ymin = p.y;
	}

	double dx = kAlpha * (xmax - xmin);
	double dy = kAlpha * (ymax - ymin);
	_head = new Point(xmax + dx, ymin - dy);
	_tail = new Point(xmin - dx, ymin - dy);

	// sort the points along the y axis
	std::sort(_points.begin(), _points.end(), cmp);
}

void SweepContext::InitEdges(std::vector<Point *> polyline) {
	int numPoints = (int)polyline.size();

	for (int i = 0; i < numPoints; i++) {
		int j = i < numPoints - 1 ? i + 1 : 0;
		_edgeList.push_back(new Edge(*polyline[i], *polyline[j]));
	}
}

Point *SweepContext::GetPoint(const int &index) {
	return _points[index];
}

void SweepContext::AddToMap(Triangle *triangle) {
	_map.push_back(triangle);
}

Node &SweepContext::LocateNode(Point &point) {
	return *_front->LocateNode(point.x);
}

void SweepContext::CreateAdvancingFront(std::vector<Node *> /*nodes*/) {
	// the initial triangle
	Triangle *triangle = new Triangle(*_points[0], *_tail, *_head);

	_map.push_back(triangle);

	_afHead = new Node(*triangle->GetPoint(1), *triangle);
	_afMiddle = new Node(*triangle->GetPoint(0), *triangle);
	_afTail = new Node(*triangle->GetPoint(2));
	_front = new AdvancingFront(*_afHead, *_afTail);

	_afHead->next = _afMiddle;
	_afMiddle->next = _afTail;
	_afMiddle->prev = _afHead;
	_afTail->prev = _afMiddle;
}

void SweepContext::RemoveNode(Node *node) {
	delete node;
}

void SweepContext::MapTriangleToNodes(Triangle &t) {
	for (int i = 0; i < 3; i++) {
		if (!t.GetNeighbor(i)) {
			Node *n = _front->LocatePoint(t.PointCW(*t.GetPoint(i)));
			if (n)
				n->triangle = &t;
		}
	}
}

void SweepContext::RemoveFromMap(Triangle *triangle) {
	_map.remove(triangle);
}

void SweepContext::MeshClean(Triangle &triangle) {
	if (!triangle.IsInterior()) {
		triangle.IsInterior(true);
		_triangles.push_back(&triangle);

		for (int i = 0; i < 3; i++) {
			if (!triangle.constrainedEdge[i] && triangle.GetNeighbor(i))
				MeshClean(*triangle.GetNeighbor(i));
		}
	}
}

Sweep::~Sweep() {
	for (Node *node : _nodes)
		delete node;
}

void Sweep::Triangulate(SweepContext &tcx) {
	tcx.InitTriangulation();
	tcx.CreateAdvancingFront(_nodes);

	// sweep the points, building the mesh
	SweepPoints(tcx);

	FinalizationPolygon(tcx);
}

void Sweep::SweepPoints(SweepContext &tcx) {
	for (int i = 1; i < tcx.pointCount(); i++) {
		Point &point = *tcx.GetPoint(i);
		Node *node = &PointEvent(tcx, point);

		for (size_t j = 0; j < point.edgeList.size(); j++)
			EdgeEvent(tcx, point.edgeList[j], node);
	}
}

void Sweep::FinalizationPolygon(SweepContext &tcx) {
	// start with an interior triangle
	Triangle *t = tcx.front()->head()->next->triangle;
	Point *p = tcx.front()->head()->next->point;

	while (!t->GetConstrainedEdgeCW(*p))
		t = t->NeighborCCW(*p);

	// collect the interior triangles bounded by constrained edges
	tcx.MeshClean(*t);
}

Node &Sweep::PointEvent(SweepContext &tcx, Point &point) {
	Node &node = tcx.LocateNode(point);
	Node &newNode = NewFrontTriangle(tcx, point, node);

	// only +epsilon has to be checked: a point never has a smaller x than the node
	if (point.x <= node.point->x + kEpsilon)
		Fill(tcx, node);

	FillAdvancingFront(tcx, newNode);
	return newNode;
}

void Sweep::EdgeEvent(SweepContext &tcx, Edge *edge, Node *node) {
	tcx.edgeEvent.constrainedEdge = edge;
	tcx.edgeEvent.right = (edge->p->x > edge->q->x);

	if (IsEdgeSideOfTriangle(*node->triangle, *edge->p, *edge->q))
		return;

	// all the filling that is needed is done here; the flips follow
	FillEdgeEvent(tcx, edge, node);

	EdgeEvent(tcx, *edge->p, *edge->q, node->triangle, *edge->q);
}

void Sweep::EdgeEvent(SweepContext &tcx, Point &ep, Point &eq, Triangle *triangle, Point &point) {
	if (IsEdgeSideOfTriangle(*triangle, ep, eq))
		return;

	Point *p1 = triangle->PointCCW(point);
	Orientation o1 = Orient2d(eq, *p1, ep);
	if (o1 == COLLINEAR) {
		if (triangle->Contains(&eq, p1)) {
			triangle->MarkConstrainedEdge(&eq, p1);
			// the constraint is modified here
			tcx.edgeEvent.constrainedEdge->q = p1;
			triangle = &triangle->NeighborAcross(point);
			EdgeEvent(tcx, ep, *p1, triangle, *p1);
		} else {
			throw std::runtime_error("EdgeEvent - collinear points not supported");
		}
		return;
	}

	Point *p2 = triangle->PointCW(point);
	Orientation o2 = Orient2d(eq, *p2, ep);
	if (o2 == COLLINEAR) {
		if (triangle->Contains(&eq, p2)) {
			triangle->MarkConstrainedEdge(&eq, p2);
			tcx.edgeEvent.constrainedEdge->q = p2;
			triangle = &triangle->NeighborAcross(point);
			EdgeEvent(tcx, ep, *p2, triangle, *p2);
		} else {
			throw std::runtime_error("EdgeEvent - collinear points not supported");
		}
		return;
	}

	if (o1 == o2) {
		// rotate clockwise or counter-clockwise to a triangle that crosses the edge
		if (o1 == CW)
			triangle = triangle->NeighborCCW(point);
		else
			triangle = triangle->NeighborCW(point);
		EdgeEvent(tcx, ep, eq, triangle, point);
	} else {
		// this triangulation crosses the constraint: flip
		FlipEdgeEvent(tcx, ep, eq, triangle, point);
	}
}

bool Sweep::IsEdgeSideOfTriangle(Triangle &triangle, Point &ep, Point &eq) {
	int index = triangle.EdgeIndex(&ep, &eq);

	if (index != -1) {
		triangle.MarkConstrainedEdge(index);
		Triangle *t = triangle.GetNeighbor(index);
		if (t)
			t->MarkConstrainedEdge(&ep, &eq);
		return true;
	}
	return false;
}

Node &Sweep::NewFrontTriangle(SweepContext &tcx, Point &point, Node &node) {
	Triangle *triangle = new Triangle(point, *node.point, *node.next->point);

	triangle->MarkNeighbor(*node.triangle);
	tcx.AddToMap(triangle);

	Node *newNode = new Node(point);
	_nodes.push_back(newNode);

	newNode->next = node.next;
	newNode->prev = &node;
	node.next->prev = newNode;
	node.next = newNode;

	if (!Legalize(tcx, *triangle))
		tcx.MapTriangleToNodes(*triangle);

	return *newNode;
}

void Sweep::Fill(SweepContext &tcx, Node &node) {
	Triangle *triangle = new Triangle(*node.prev->point, *node.point, *node.next->point);

	// (the constrained edges are copied in Legalize)
	triangle->MarkNeighbor(*node.prev->triangle);
	triangle->MarkNeighbor(*node.triangle);

	tcx.AddToMap(triangle);

	// update the advancing front
	node.prev->next = node.next;
	node.next->prev = node.prev;

	// a legalised triangle has already been mapped
	if (!Legalize(tcx, *triangle))
		tcx.MapTriangleToNodes(*triangle);
}

void Sweep::FillAdvancingFront(SweepContext &tcx, Node &n) {
	// fill the holes to the right
	Node *node = n.next;
	while (node->next) {
		double angle = HoleAngle(*node);
		if (angle > kPiDiv2 || angle < -kPiDiv2)
			break;
		Fill(tcx, *node);
		node = node->next;
	}

	// ... and to the left
	node = n.prev;
	while (node->prev) {
		double angle = HoleAngle(*node);
		if (angle > kPiDiv2 || angle < -kPiDiv2)
			break;
		Fill(tcx, *node);
		node = node->prev;
	}

	// fill the basin to the right
	if (n.next && n.next->next) {
		double angle = BasinAngle(n);
		if (angle < kPi3Div4)
			FillBasin(tcx, n);
	}
}

double Sweep::BasinAngle(Node &node) {
	double ax = node.point->x - node.next->next->point->x;
	double ay = node.point->y - node.next->next->point->y;
	return atan2(ay, ax);
}

double Sweep::HoleAngle(Node &node) {
	// the angle between the two edges as the argument of the complex product
	double ax = node.next->point->x - node.point->x;
	double ay = node.next->point->y - node.point->y;
	double bx = node.prev->point->x - node.point->x;
	double by = node.prev->point->y - node.point->y;
	return atan2(ax * by - ay * bx, ax * bx + ay * by);
}

bool Sweep::Legalize(SweepContext &tcx, Triangle &t) {
	// a triangle is legalised by looking for edges that violate the Delaunay condition
	for (int i = 0; i < 3; i++) {
		if (t.delaunayEdge[i])
			continue;

		Triangle *ot = t.GetNeighbor(i);
		if (ot) {
			Point *p = t.GetPoint(i);
			Point *op = ot->OppositePoint(t, *p);
			int oi = ot->Index(op);

			// constrained edges (and Delaunay edges, during recursive legalisation) are left alone
			if (ot->constrainedEdge[oi] || ot->delaunayEdge[oi]) {
				t.constrainedEdge[i] = ot->constrainedEdge[oi];
				continue;
			}

			bool inside = Incircle(*p, *t.PointCCW(*p), *t.PointCW(*p), *op);
			if (inside) {
				// mark the shared edge as Delaunay
				t.delaunayEdge[i] = true;
				ot->delaunayEdge[oi] = true;

				// rotate the shared edge one vertex clockwise
				RotateTrianglePair(t, *p, *ot, *op);

				// the two triangles now share a valid Delaunay edge; the four new
				// edges have to be checked. A triangle is mapped to the nodes only once.
				bool notLegalized = !Legalize(tcx, t);
				if (notLegalized)
					tcx.MapTriangleToNodes(t);

				notLegalized = !Legalize(tcx, *ot);
				if (notLegalized)
					tcx.MapTriangleToNodes(*ot);

				// the Delaunay edges are only valid until a new triangle or point is added
				t.delaunayEdge[i] = false;
				ot->delaunayEdge[oi] = false;

				// the recursive legalisation has handled the other edges
				return true;
			}
		}
	}
	return false;
}

bool Sweep::Incircle(Point &pa, Point &pb, Point &pc, Point &pd) {
	double adx = pa.x - pd.x;
	double ady = pa.y - pd.y;
	double bdx = pb.x - pd.x;
	double bdy = pb.y - pd.y;

	double adxbdy = adx * bdy;
	double bdxady = bdx * ady;
	double oabd = adxbdy - bdxady;

	if (oabd <= 0)
		return false;

	double cdx = pc.x - pd.x;
	double cdy = pc.y - pd.y;

	double cdxady = cdx * ady;
	double adxcdy = adx * cdy;
	double ocad = cdxady - adxcdy;

	if (ocad <= 0)
		return false;

	double bdxcdy = bdx * cdy;
	double cdxbdy = cdx * bdy;

	double alift = adx * adx + ady * ady;
	double blift = bdx * bdx + bdy * bdy;
	double clift = cdx * cdx + cdy * cdy;

	double det = alift * (bdxcdy - cdxbdy) + blift * ocad + clift * oabd;

	return det > 0;
}

void Sweep::RotateTrianglePair(Triangle &t, Point &p, Triangle &ot, Point &op) {
	Triangle *n1, *n2, *n3, *n4;
	n1 = t.NeighborCCW(p);
	n2 = t.NeighborCW(p);
	n3 = ot.NeighborCCW(op);
	n4 = ot.NeighborCW(op);

	bool ce1, ce2, ce3, ce4;
	ce1 = t.GetConstrainedEdgeCCW(p);
	ce2 = t.GetConstrainedEdgeCW(p);
	ce3 = ot.GetConstrainedEdgeCCW(op);
	ce4 = ot.GetConstrainedEdgeCW(op);

	bool de1, de2, de3, de4;
	de1 = t.GetDelunayEdgeCCW(p);
	de2 = t.GetDelunayEdgeCW(p);
	de3 = ot.GetDelunayEdgeCCW(op);
	de4 = ot.GetDelunayEdgeCW(op);

	t.Legalize(p, op);
	ot.Legalize(op, p);

	// remap the Delaunay edges
	ot.SetDelunayEdgeCCW(p, de1);
	t.SetDelunayEdgeCW(p, de2);
	t.SetDelunayEdgeCCW(op, de3);
	ot.SetDelunayEdgeCW(op, de4);

	// remap the constrained edges
	ot.SetConstrainedEdgeCCW(p, ce1);
	t.SetConstrainedEdgeCW(p, ce2);
	t.SetConstrainedEdgeCCW(op, ce3);
	ot.SetConstrainedEdgeCW(op, ce4);

	// remap the neighbours
	t.ClearNeighbors();
	ot.ClearNeighbors();
	if (n1)
		ot.MarkNeighbor(*n1);
	if (n2)
		t.MarkNeighbor(*n2);
	if (n3)
		t.MarkNeighbor(*n3);
	if (n4)
		ot.MarkNeighbor(*n4);
	t.MarkNeighbor(ot);
}

void Sweep::FillBasin(SweepContext &tcx, Node &node) {
	if (Orient2d(*node.point, *node.next->point, *node.next->next->point) == CCW)
		tcx.basin.leftNode = node.next->next;
	else
		tcx.basin.leftNode = node.next;

	// find the bottom and the right node
	tcx.basin.bottomNode = tcx.basin.leftNode;
	while (tcx.basin.bottomNode->next && tcx.basin.bottomNode->point->y >= tcx.basin.bottomNode->next->point->y)
		tcx.basin.bottomNode = tcx.basin.bottomNode->next;
	if (tcx.basin.bottomNode == tcx.basin.leftNode) {
		// no valid basin
		return;
	}

	tcx.basin.rightNode = tcx.basin.bottomNode;
	while (tcx.basin.rightNode->next && tcx.basin.rightNode->point->y < tcx.basin.rightNode->next->point->y)
		tcx.basin.rightNode = tcx.basin.rightNode->next;
	if (tcx.basin.rightNode == tcx.basin.bottomNode) {
		// no valid basin
		return;
	}

	tcx.basin.width = tcx.basin.rightNode->point->x - tcx.basin.leftNode->point->x;
	tcx.basin.leftHighest = tcx.basin.leftNode->point->y > tcx.basin.rightNode->point->y;

	FillBasinReq(tcx, tcx.basin.bottomNode);
}

void Sweep::FillBasinReq(SweepContext &tcx, Node *node) {
	// stop at a shallow basin
	if (IsShallow(tcx, *node))
		return;

	Fill(tcx, *node);

	if (node->prev == tcx.basin.leftNode && node->next == tcx.basin.rightNode) {
		return;
	} else if (node->prev == tcx.basin.leftNode) {
		Orientation o = Orient2d(*node->point, *node->next->point, *node->next->next->point);
		if (o == CW)
			return;
		node = node->next;
	} else if (node->next == tcx.basin.rightNode) {
		Orientation o = Orient2d(*node->point, *node->prev->point, *node->prev->prev->point);
		if (o == CCW)
			return;
		node = node->prev;
	} else {
		// continue with the neighbour with the lowest y
		if (node->prev->point->y < node->next->point->y)
			node = node->prev;
		else
			node = node->next;
	}

	FillBasinReq(tcx, node);
}

bool Sweep::IsShallow(SweepContext &tcx, Node &node) {
	double height;

	if (tcx.basin.leftHighest)
		height = tcx.basin.leftNode->point->y - node.point->y;
	else
		height = tcx.basin.rightNode->point->y - node.point->y;

	// stop filling a shallow basin
	return tcx.basin.width > height;
}

void Sweep::FillEdgeEvent(SweepContext &tcx, Edge *edge, Node *node) {
	if (tcx.edgeEvent.right)
		FillRightAboveEdgeEvent(tcx, edge, node);
	else
		FillLeftAboveEdgeEvent(tcx, edge, node);
}

void Sweep::FillRightAboveEdgeEvent(SweepContext &tcx, Edge *edge, Node *node) {
	while (node->next->point->x < edge->p->x) {
		// is the next node below the edge?
		if (Orient2d(*edge->q, *node->next->point, *edge->p) == CCW)
			FillRightBelowEdgeEvent(tcx, edge, *node);
		else
			node = node->next;
	}
}

void Sweep::FillRightBelowEdgeEvent(SweepContext &tcx, Edge *edge, Node &node) {
	if (node.point->x < edge->p->x) {
		if (Orient2d(*node.point, *node.next->point, *node.next->next->point) == CCW) {
			// concave
			FillRightConcaveEdgeEvent(tcx, edge, node);
		} else {
			// convex
			FillRightConvexEdgeEvent(tcx, edge, node);
			// try this one again
			FillRightBelowEdgeEvent(tcx, edge, node);
		}
	}
}

void Sweep::FillRightConcaveEdgeEvent(SweepContext &tcx, Edge *edge, Node &node) {
	Fill(tcx, *node.next);

	if (node.next->point != edge->p) {
		// is the next node above or below the edge?
		if (Orient2d(*edge->q, *node.next->point, *edge->p) == CCW) {
			// below
			if (Orient2d(*node.point, *node.next->point, *node.next->next->point) == CCW) {
				// the next one is concave
				FillRightConcaveEdgeEvent(tcx, edge, node);
			}
			// (otherwise it is convex)
		}
	}
}

void Sweep::FillRightConvexEdgeEvent(SweepContext &tcx, Edge *edge, Node &node) {
	// is the next one concave or convex?
	if (Orient2d(*node.next->point, *node.next->next->point, *node.next->next->next->point) == CCW) {
		// concave
		FillRightConcaveEdgeEvent(tcx, edge, *node.next);
	} else {
		// convex: is the next one above or below the edge?
		if (Orient2d(*edge->q, *node.next->next->point, *edge->p) == CCW) {
			// below
			FillRightConvexEdgeEvent(tcx, edge, *node.next);
		}
		// (above: nothing to do)
	}
}

void Sweep::FillLeftAboveEdgeEvent(SweepContext &tcx, Edge *edge, Node *node) {
	while (node->prev->point->x > edge->p->x) {
		// is the previous node below the edge?
		if (Orient2d(*edge->q, *node->prev->point, *edge->p) == CW)
			FillLeftBelowEdgeEvent(tcx, edge, *node);
		else
			node = node->prev;
	}
}

void Sweep::FillLeftBelowEdgeEvent(SweepContext &tcx, Edge *edge, Node &node) {
	if (node.point->x > edge->p->x) {
		if (Orient2d(*node.point, *node.prev->point, *node.prev->prev->point) == CW) {
			// concave
			FillLeftConcaveEdgeEvent(tcx, edge, node);
		} else {
			// convex
			FillLeftConvexEdgeEvent(tcx, edge, node);
			// try this one again
			FillLeftBelowEdgeEvent(tcx, edge, node);
		}
	}
}

void Sweep::FillLeftConvexEdgeEvent(SweepContext &tcx, Edge *edge, Node &node) {
	// is the previous one concave or convex?
	if (Orient2d(*node.prev->point, *node.prev->prev->point, *node.prev->prev->prev->point) == CW) {
		// concave
		FillLeftConcaveEdgeEvent(tcx, edge, *node.prev);
	} else {
		// convex: above or below the edge?
		if (Orient2d(*edge->q, *node.prev->prev->point, *edge->p) == CW) {
			// below
			FillLeftConvexEdgeEvent(tcx, edge, *node.prev);
		}
		// (above: nothing to do)
	}
}

void Sweep::FillLeftConcaveEdgeEvent(SweepContext &tcx, Edge *edge, Node &node) {
	Fill(tcx, *node.prev);

	if (node.prev->point != edge->p) {
		// above or below the edge?
		if (Orient2d(*edge->q, *node.prev->point, *edge->p) == CW) {
			// below
			if (Orient2d(*node.point, *node.prev->point, *node.prev->prev->point) == CW) {
				// the next one is concave
				FillLeftConcaveEdgeEvent(tcx, edge, node);
			}
			// (otherwise it is convex)
		}
	}
}

void Sweep::FlipEdgeEvent(SweepContext &tcx, Point &ep, Point &eq, Triangle *t, Point &p) {
	Triangle &ot = t->NeighborAcross(p);
	Point &op = *ot.OppositePoint(*t, p);

	if (InScanArea(p, *t->PointCCW(p), *t->PointCW(p), op)) {
		// rotate the shared edge one vertex clockwise
		RotateTrianglePair(*t, p, ot, op);

		tcx.MapTriangleToNodes(*t);
		tcx.MapTriangleToNodes(ot);

		if (p == eq && op == ep) {
			if (eq == *tcx.edgeEvent.constrainedEdge->q && ep == *tcx.edgeEvent.constrainedEdge->p) {
				t->MarkConstrainedEdge(&ep, &eq);
				ot.MarkConstrainedEdge(&ep, &eq);
				Legalize(tcx, *t);
				Legalize(tcx, ot);
			}
		} else {
			Orientation o = Orient2d(eq, op, ep);
			t = &NextFlipTriangle(tcx, (int)o, *t, ot, p, op);
			FlipEdgeEvent(tcx, ep, eq, t, p);
		}
	} else {
		Point &newP = NextFlipPoint(ep, eq, ot, op);
		FlipScanEdgeEvent(tcx, ep, eq, *t, ot, newP);
		EdgeEvent(tcx, ep, eq, t, p);
	}
}

Triangle &Sweep::NextFlipTriangle(SweepContext &tcx, int o, Triangle &t, Triangle &ot, Point &p, Point &op) {
	if (o == CCW) {
		// ot is not crossing the edge after the flip
		int edgeIndex = ot.EdgeIndex(&p, &op);
		ot.delaunayEdge[edgeIndex] = true;
		Legalize(tcx, ot);
		ot.ClearDelunayEdges();
		return t;
	}

	// t is not crossing the edge after the flip
	int edgeIndex = t.EdgeIndex(&p, &op);
	t.delaunayEdge[edgeIndex] = true;
	Legalize(tcx, t);
	t.ClearDelunayEdges();
	return ot;
}

Point &Sweep::NextFlipPoint(Point &ep, Point &eq, Triangle &ot, Point &op) {
	Orientation o2d = Orient2d(eq, op, ep);

	if (o2d == CW) {
		// right
		return *ot.PointCCW(op);
	} else if (o2d == CCW) {
		// left
		return *ot.PointCW(op);
	}

	throw std::runtime_error("[Unsupported] Opposing point on constrained edge");
}

void Sweep::FlipScanEdgeEvent(SweepContext &tcx, Point &ep, Point &eq, Triangle &flipTriangle, Triangle &t,
                              Point &p) {
	Triangle &ot = t.NeighborAcross(p);
	Point &op = *ot.OppositePoint(t, p);

	if (InScanArea(eq, *flipTriangle.PointCCW(eq), *flipTriangle.PointCW(eq), op)) {
		// flip with the new edge op -> eq
		FlipEdgeEvent(tcx, eq, op, &ot, op);
	} else {
		Point &newP = NextFlipPoint(ep, eq, ot, op);
		FlipScanEdgeEvent(tcx, ep, eq, flipTriangle, ot, newP);
	}
}

CDT::CDT(std::vector<Point *> polyline) {
	_sweepContext = new SweepContext(polyline);
	_sweep = new Sweep;
}

CDT::~CDT() {
	delete _sweepContext;
	delete _sweep;
}

void CDT::AddHole(std::vector<Point *> polyline) {
	_sweepContext->AddHole(polyline);
}

void CDT::AddPoint(Point *point) {
	_sweepContext->AddPoint(point);
}

void CDT::Triangulate() {
	_sweep->Triangulate(*_sweepContext);
}

std::vector<Triangle *> CDT::GetTriangles() {
	return _sweepContext->GetTriangles();
}

std::list<Triangle *> CDT::GetMap() {
	return _sweepContext->GetMap();
}

}

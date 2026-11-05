// Checks ObjectModel against reversing/analysis/xanth_objects.tsv (extracted from the original executable by an independent Python script, which the lifted original code
// also matched: 11,454 attribute checks, 414 locations, 484 prop_a values) and the structural invariants of the containment tree.
// XANTH_OBJECTS=<path of xanth_objects.tsv>; skipped without it.
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <map>
#include <set>
#include <sstream>

#include "xanth/objects.h"

static int g_failures = 0, g_checks = 0;

#define CHECK(cond, ...)                                                      \
	do {                                                                      \
		g_checks++;                                                           \
		if (!(cond)) {                                                        \
			g_failures++;                                                     \
			if (g_failures < 30) {                                            \
				printf("FAIL %s:%d: ", __FILE__, __LINE__);                   \
				printf(__VA_ARGS__);                                          \
				printf("\n");                                                 \
			}                                                                 \
		}                                                                     \
	} while (0)

static std::vector<std::string> split(const std::string &line) {
	std::vector<std::string> f;
	std::string cur;
	for (size_t i = 0; i < line.size(); i++) {
		if (line[i] == '\t') {
			f.push_back(cur);
			cur.clear();
		} else if (line[i] != '\r') {
			cur += line[i];
		}
	}
	f.push_back(cur);
	return f;
}

static int num(const std::string &s) {
	return s.empty() ? 0 : atoi(s.c_str());
}

// the children of (parent, slot) by walking the list
static std::vector<int> children(const Xanth::ObjectModel &m, int parent, int slot) {
	std::vector<int> c;
	for (int o = m.listHead(parent, slot); o != 0 && c.size() < 1000; o = m.nextSibling(o))
		c.push_back(o);
	return c;
}

// every object with a parent is in its parent's list for its slot, and every list member points back
static void checkTree(const Xanth::ObjectModel &m, const char *when) {
	std::map<int, int> listed;      // object -> parent whose list contains it
	for (int p = 1; p <= m.count(); p++) {
		int slots = m.type(p) == Xanth::OT_CONTAINER ? 2 : m.type(p) == Xanth::OT_CHARACTER ? 3 : 1;
		for (int s = 0; s < slots; s++) {
			std::vector<int> c = children(m, p, s);
			for (size_t i = 0; i < c.size(); i++) {
				CHECK(listed.find(c[i]) == listed.end(), "%s: object %d is in two lists", when, c[i]);
				listed[c[i]] = p;
				CHECK(m.location(c[i]) == p && m.slot(c[i]) == s, "%s: object %d is listed under %d/%d but says %d/%d", when, c[i], p, s, m.location(c[i]), m.slot(c[i]));
			}
		}
	}
	for (int o = 1; o <= m.count(); o++) {
		int p = m.location(o);
		if (p == 0)
			continue;
		int slots = m.type(p) == Xanth::OT_CONTAINER ? 2 : m.type(p) == Xanth::OT_CHARACTER ? 3 : 1;
		if (m.slot(o) >= slots)
			continue;     // the original data has one such object (97 sits in slot 1 of the item 95, which has one slot): no list can hold it, the original code behaves the same
		CHECK(listed.find(o) != listed.end(), "%s: object %d has parent %d but is in no list", when, o, p);
	}
}

int main() {
	Xanth::ObjectModel m;
	CHECK(m.count() == 498, "498 objects");

	const char *path = getenv("XANTH_OBJECTS");
	if (path) {
		std::ifstream in(path);
		CHECK(in.good(), "cannot open %s", path);
		std::string line;
		std::getline(in, line);       // header: id type name alt location slot next handler default_verb points prop_a attrs record_hex
		int rows = 0, attrChecks = 0;
		while (std::getline(in, line)) {
			std::vector<std::string> f = split(line);
			if (f.size() < 13)
				continue;
			int id = num(f[0]);
			rows++;
			CHECK(m.type(id) == num(f[1]), "type of %d", id);
			CHECK(m.location(id) == num(f[4]), "location of %d: %d vs %d", id, m.location(id), num(f[4]));
			CHECK(m.slot(id) == num(f[5]), "slot of %d", id);
			CHECK(m.nextSibling(id) == num(f[6]), "next sibling of %d", id);
			CHECK(m.handlerIndex(id) == num(f[7]), "handler of %d", id);
			CHECK(m.defaultVerb(id) == num(f[8]), "default verb of %d: %d vs %d", id, m.defaultVerb(id), num(f[8]));
			CHECK(m.points(id) == num(f[9]), "points of %d", id);
			CHECK(m.propA(id) == num(f[10]), "prop_a of %d", id);
			std::set<int> attrs;
			std::stringstream ss(f[11]);
			std::string a;
			while (std::getline(ss, a, ','))
				if (!a.empty())
					attrs.insert(atoi(a.c_str()));
			for (int attr = 1; attr <= 24; attr++, attrChecks++)
				CHECK(m.hasAttr(id, attr) == (attrs.count(attr) != 0), "attribute %d of object %d", attr, id);
		}
		printf("compared %d objects (%d attribute checks)\n", rows, attrChecks);
		CHECK(rows == 498, "498 rows");
	} else {
		printf("test_objects: XANTH_OBJECTS not set, table comparison skipped\n");
	}

	checkTree(m, "initial state");

	// moves keep the tree consistent: take everything out of the world and put it back somewhere else
	Xanth::ObjectModel w;
	int moved = 0;
	for (int o = 1; o <= w.count(); o++) {
		if (w.type(o) != Xanth::OT_ITEM)
			continue;
		int from = w.location(o);
		w.move(o, 0, 0);
		CHECK(w.location(o) == 0, "item %d leaves the world", o);
		w.move(o, from, 0);
		CHECK(w.location(o) == from, "item %d comes back", o);
		moved++;
	}
	checkTree(w, "after moving every item out and back");
	printf("moved %d items\n", moved);

	// moving into a container and out again through the slots
	int container = 0, item = 0;
	for (int o = 1; o <= m.count() && (!container || !item); o++) {
		if (m.type(o) == Xanth::OT_CONTAINER && !container)
			container = o;
		if (m.type(o) == Xanth::OT_ITEM && !item)
			item = o;
	}
	w.reset();
	w.move(item, container, 1);
	CHECK(w.isAt(item, container, 1) && w.isAt(item, container, -1) && !w.isAt(item, container, 0), "isAt in slot 1");
	CHECK(w.listHead(container, 1) == item, "item heads the list of slot 1");
	CHECK(w.isWithin(item, container) && !w.isWithin(container, item), "isWithin");
	w.reset();
	CHECK(w.location(item) == m.location(item), "reset restores the initial state");

	CHECK(!w.hasAttr(0, 7) && !w.hasAttr(100000, 7), "invalid ids read as 0");

	printf("test_objects: %d checks, %d failures\n", g_checks, g_failures);
	return g_failures ? 1 : 0;
}

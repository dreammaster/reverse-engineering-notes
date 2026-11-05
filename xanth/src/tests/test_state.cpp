// GameState: the save game format. Round trips, rejects bad data, and (with XANTH_RESTART=<restart.dat written by the original's init_game>) loads the original's own file,
// writes it back byte for byte and lists which regions differ between a new game and the original's state at the end of its start-up.
#include <cstdio>
#include <cstdlib>

#include "common/files.h"
#include "xanth/gamestate.h"

static int g_failures = 0, g_checks = 0;

#define CHECK(cond, ...)                                                      \
	do {                                                                      \
		g_checks++;                                                           \
		if (!(cond)) {                                                        \
			g_failures++;                                                     \
			printf("FAIL %s:%d: ", __FILE__, __LINE__);                       \
			printf(__VA_ARGS__);                                              \
			printf("\n");                                                     \
		}                                                                     \
	} while (0)

int main() {
	Xanth::GameState a;
	Common::Bytes saved;
	a.save(saved);
	CHECK(saved.size() == Xanth::GameState::saveSize(0), "size of a new game's save: %u", (unsigned)saved.size());
	CHECK(saved.size() == 17964, "17,964 bytes like the original's restart.dat (got %u)", (unsigned)saved.size());

	// round trip with changes in every part
	a.objects.move(35, 90, 0);
	a.v.score = 5;
	a.v.current_room = 51;
	a.music[2] = 100;
	Xanth::ScheduledEvent e = {3, 12};
	a.events.push_back(e);
	a.save(saved);
	Xanth::GameState b;
	CHECK(b.load(saved), "load");
	Common::Bytes again;
	b.save(again);
	CHECK(again == saved, "save -> load -> save is the identity");
	CHECK(b.v.score == 5 && b.v.current_room == 51 && b.objects.location(35) == 90 && b.music[2] == 100 && b.events.size() == 1 && b.events[0].b == 12, "values survive");

	// bad data is rejected and leaves the state alone
	Common::Bytes bad(saved);
	bad[3] = 0;
	CHECK(!b.load(bad), "bad header");
	bad = saved;
	bad.resize(bad.size() - 1);
	CHECK(!b.load(bad), "truncated");
	bad = saved;
	bad.push_back(0);
	CHECK(!b.load(bad), "trailing byte");
	CHECK(b.v.score == 5, "state unchanged after a rejected load");

	const char *path = getenv("XANTH_RESTART");
	if (path) {
		Common::GameFiles gf;
		std::string p(path);
		size_t slash = p.find_last_of("/\\");
		Common::Bytes orig;
		CHECK(gf.setFolder(slash == std::string::npos ? "." : p.substr(0, slash)) && gf.load(p.substr(slash == std::string::npos ? 0 : slash + 1), orig), "read %s", path);
		Xanth::GameState o;
		CHECK(o.load(orig), "the original's restart.dat loads");
		Common::Bytes back;
		o.save(back);
		CHECK(back == orig, "the original's file is written back byte for byte");
		// what the original's start-up changes compared with a new game's initial state
		Xanth::GameState fresh;
		int diffs = 0;
		for (int i = 0; i < ARRAYSIZE(Xanth::kVarRegions); i++) {
			const Xanth::VarRegion &r = Xanth::kVarRegions[i];
			if (memcmp(reinterpret_cast<const byte *>(&o.v) + r.offset, reinterpret_cast<const byte *>(&fresh.v) + r.offset, r.size) != 0) {
				printf("  region %s differs after the original's start-up\n", r.name);
				diffs++;
			}
		}
		for (int id = 1; id <= fresh.objects.count(); id++)
			if (memcmp(o.objects.record(id), fresh.objects.record(id), fresh.objects.recordSize(id)) != 0) {
				printf("  object %d differs after the original's start-up\n", id);
				diffs++;
			}
		printf("regions/objects changed by the original's start-up: %d; events %u; music words %04X %04X %04X %04X\n", diffs, (unsigned)o.events.size(), o.music[0], o.music[1], o.music[2], o.music[3]);
	} else {
		printf("test_state: XANTH_RESTART not set, comparison with the original's file skipped\n");
	}

	printf("test_state: %d checks, %d failures\n", g_checks, g_failures);
	return g_failures ? 1 : 0;
}

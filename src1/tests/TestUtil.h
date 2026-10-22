/* Tiny test harness: CHECK()/CHECK_EQ() count failures, SKIP() for tests that
 * need the original game files, which are not part of the repository. */

#ifndef YENDOR1_TESTS_TESTUTIL_H
#define YENDOR1_TESTS_TESTUTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace TestUtil {

extern int g_failures;
extern int g_checks;

inline void fail(const char *file, int line, const char *expr) {
	printf("  FAIL %s:%d: %s\n", file, line, expr);
	g_failures++;
}

/** The game directory: $YENDOR1_GAME_DIR, else ../../yendor1/game relative to tests/. */
inline const char *gameDir() {
	const char *e = getenv("YENDOR1_GAME_DIR");
	return e ? e : "../../yendor1/game";
}

inline bool haveGameFiles() {
	char p[600];
	snprintf(p, sizeof(p), "%s/FILE00.16", gameDir());
	FILE *f = fopen(p, "rb");
	if (!f)
		return false;
	fclose(f);
	return true;
}

} // namespace TestUtil

#define CHECK(expr) do { TestUtil::g_checks++; if (!(expr)) TestUtil::fail(__FILE__, __LINE__, #expr); } while (0)
#define CHECK_EQ(a, b) do { TestUtil::g_checks++; if (!((a) == (b))) { \
	printf("  FAIL %s:%d: %s == %s (%ld vs %ld)\n", __FILE__, __LINE__, #a, #b, (long)(a), (long)(b)); TestUtil::g_failures++; } } while (0)

#define TEST_MAIN_BEGIN(name) \
	int TestUtil::g_failures = 0; int TestUtil::g_checks = 0; \
	int main() { printf("%s\n", name);
#define TEST_MAIN_END() \
	printf("%d checks, %d failures\n", TestUtil::g_checks, TestUtil::g_failures); \
	return TestUtil::g_failures ? 1 : 0; }

#endif

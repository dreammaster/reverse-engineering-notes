// Unit tests for the C ports of hand-written assembly routines whose behaviour was verified against the original
// ULTIMA.EXE disassembly (see docs/audit.md).  Built and run by `mingw32-make unit`.

#include "common/common.h"
#include "vars.h"
#include "funcs.h"

#include <stdio.h>
#include <string.h>

static int s_failures;

#define CHECK(cond)                                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(cond))                                                                                                   \
        {                                                                                                              \
            printf("FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                                     \
            s_failures++;                                                                                              \
        }                                                                                                              \
    } while (0)

// ULTIMA_2092: x = rol-less 16-bit generator: ax = ror(seed + 0x9248, 3) ^ 0x9248, + 0x11; result = (ax & 0x7fff) % n.
// Reference values were produced independently from the disassembly (seed 0x1234, range 0..99).
static void TestRandom(void)
{
    static const int expected[] = {52, 81, 53, 94, 75, 4, 37, 63};

    ULTIMA_207e_srand(0x1234);
    for (int i = 0; i < 8; i++)
        CHECK(ULTIMA_2092_RandomRange(0, 99) == expected[i]);
    CHECK(D_5420 == 0x2623);

    // the same seed reproduces the same sequence (town.c relies on this: srand(D_587e))
    ULTIMA_207e_srand(0x1234);
    CHECK(ULTIMA_2092_RandomRange(0, 99) == 52);

    // range offset
    ULTIMA_207e_srand(0x1234);
    CHECK(ULTIMA_2092_RandomRange(10, 109) == 62);
}

// ULTIMA_6f1e: case-insensitive substring search (index or -1), with the original's skip-ahead quirk.
static void TestSubstring(void)
{
    CHECK(ULTIMA_6f1e((byte*)"AN", "AN YLEM") == 0);
    CHECK(ULTIMA_6f1e((byte*)"ylem", "cast an Ylem now") == 8);
    CHECK(ULTIMA_6f1e((byte*)"Horse", "a HORSE!") == 2);
    CHECK(ULTIMA_6f1e((byte*)"XYZ", "an ylem") == -1);
    CHECK(ULTIMA_6f1e((byte*)"LONGER", "SHORT") == -1);
    CHECK(ULTIMA_6f1e((byte*)"A", "A") == 0);
    // quirk: after a partial match of k characters the scan resumes k + 1 characters on, so this is missed
    CHECK(ULTIMA_6f1e((byte*)"AAB", "AAAB") == -1);
    CHECK(ULTIMA_6f1e((byte*)"AAB", "XAAB") == 1);
    // high bits are ignored (Apple-style text with bit 7 set)
    CHECK(ULTIMA_6f1e((byte*)"\xc1\xce", "an") == 0);
}

int main(void)
{
    TestRandom();
    TestSubstring();

    if (s_failures)
    {
        printf("%d failure(s)\n", s_failures);
        return 1;
    }

    printf("all asm-port tests passed\n");
    return 0;
}

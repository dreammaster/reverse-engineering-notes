// Standalone driver for tools/verify_cga_anim.py: applies the CGA animation port to a tileset and prints a digest after every step.
//   test_animate_cga TILESET.bin SCRIPT.txt
// script lines: T | M <mode> <hour> <minute> | G <rows> <floor>   (G builds the moongate scratch tile, prints it, then restores it)
#include "graphics/animate_cga.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static unsigned long long Hash(const unsigned char* p, int n)
{
    unsigned long long h = 1469598103934665603ULL;
    for (int i = 0; i < n; i++)
        h = (h ^ p[i]) * 1099511628211ULL;
    return h;
}

int main(int argc, char** argv)
{
    static unsigned char ts[0x8000];
    FILE* f = fopen(argv[1], "rb");
    if (!f || fread(ts, 1, sizeof(ts), f) != sizeof(ts))
        return 2;
    fclose(f);
    f = fopen(argv[2], "r");
    char op[8];
    while (fscanf(f, "%7s", op) == 1)
    {
        int a, b, c;
        if (!strcmp(op, "T"))
            AnimateCga_Tileset(ts);
        else if (!strcmp(op, "M"))
        {
            fscanf(f, "%d %d %d", &a, &b, &c);
            AnimateCga_TimeTileset(ts, a, (unsigned char)b, (unsigned char)c);
        }
        else if (!strcmp(op, "G"))
        {
            unsigned char backup[0x40];
            fscanf(f, "%d %d", &a, &b);
            AnimateCga_BuildMoongateTile(ts, a, (unsigned char)b, backup);
            printf("G scratch %llx\n", Hash(&ts[0x4580], 0x40));
            AnimateCga_RestoreMoongateTile(ts, backup);
        }
        else
            return 3;
        printf("%s %llx %04x\n", op, Hash(ts, sizeof(ts)), AnimateCga_GetSeed());
    }
    return 0;
}

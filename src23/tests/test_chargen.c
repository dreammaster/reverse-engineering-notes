/*
 * Build and run (from src23/tests):
 *   gcc -Wall -Wextra -std=c99 -I .. -o test_chargen test_chargen.c ../chargen.c ../party.c ../item.c ../bcd4.c ../effect.c ../savegame.c ../random.c && ./test_chargen
 *
 * The vectors below were produced by emulating the original routines from both games' disassembly
 * (the Roll vectors agree between the games; derived vectors are per game).
 */
#include <stdio.h>
#include <string.h>

#include "chargen.h"

static int g_failureCount = 0;

static void check(const char *label, bool ok) {
    if (ok) {
        printf("PASS %s\n", label);
    } else {
        g_failureCount++;
        printf("FAIL %s\n", label);
    }
}

typedef struct {
    int game;
    int cls;
    uint16_t attrs[6];     /* Strength, Dexterity, Stamina, Intelligence, Wisdom, Charisma */
    uint16_t expected[12]; /* 0x58 .. 0x70 except 0x62 */
} DerivedVector;

static const uint16_t kDerivedOffsets[12] = {0x58, 0x5A, 0x5C, 0x5E, 0x60, 0x64, 0x66, 0x68, 0x6A, 0x6C, 0x6E, 0x70};

static const DerivedVector kDerived[] = {
    {2, 0, {65, 64, 52, 78, 51, 54}, {57, 65, 64, 65, 65, 75, 64, 58, 67, 64, 70, 57}},
    {2, 0, {55, 48, 88, 45, 56, 86}, {73, 54, 49, 55, 52, 47, 48, 80, 47, 48, 49, 54}},
    {2, 0, {919, 850, 702, 815, 969, 913}, {768, 250, 209, 919, 885, 175, 850, 243, 188, 850, 862, 283}},
    {2, 1, {77, 53, 90, 97, 90, 63}, {83, 78, 62, 82, 71, 40, 53, 40, 64, 40, 0, 0}},
    {2, 1, {92, 64, 95, 83, 53, 49}, {90, 92, 74, 97, 83, 40, 64, 40, 71, 40, 0, 0}},
    {2, 1, {642, 966, 870, 946, 996, 644}, {881, 712, 250, 647, 809, 40, 966, 40, 309, 40, 0, 0}},
    {2, 2, {70, 60, 65, 42, 57, 75}, {68, 68, 62, 70, 65, 44, 63, 75, 61, 53, 41, 0}},
    {2, 2, {40, 67, 53, 63, 99, 74}, {60, 45, 62, 40, 54, 67, 70, 77, 72, 60, 69, 0}},
    {2, 2, {836, 626, 783, 666, 710, 966}, {746, 138, 668, 836, 731, 670, 629, 271, 639, 619, 674, 0}},
    {2, 3, {50, 87, 90, 68, 78, 99}, {88, 62, 80, 50, 69, 40, 90, 92, 82, 92, 0, 0}},
    {2, 3, {51, 72, 82, 51, 42, 47}, {79, 60, 68, 51, 62, 40, 75, 46, 66, 77, 0, 0}},
    {2, 3, {641, 817, 923, 644, 673, 758}, {866, 681, 782, 641, 730, 40, 820, 739, 781, 822, 0, 0}},
    {2, 4, {79, 40, 64, 61, 48, 73}, {58, 66, 41, 72, 53, 60, 40, 40, 40, 0, 40, 50}},
    {2, 4, {65, 52, 97, 90, 43, 80}, {81, 57, 48, 58, 52, 85, 52, 40, 40, 0, 40, 52}},
    {2, 4, {991, 664, 781, 955, 843, 628}, {767, 265, 722, 984, 821, 288, 664, 40, 40, 0, 40, 210}},
    {2, 5, {48, 87, 95, 85, 78, 89}, {88, 50, 73, 41, 61, 40, 87, 0, 40, 87, 40, 84}},
    {2, 5, {87, 49, 78, 66, 47, 46}, {71, 75, 49, 80, 62, 40, 49, 0, 40, 49, 40, 56}},
    {2, 5, {674, 706, 887, 763, 994, 855}, {811, 675, 693, 667, 683, 40, 706, 0, 40, 706, 40, 298}},
    {2, 6, {91, 54, 94, 56, 47, 94}, {82, 84, 56, 86, 68, 40, 59, 88, 0, 40, 40, 44}},
    {2, 6, {92, 63, 51, 83, 44, 72}, {60, 87, 63, 87, 73, 40, 68, 73, 0, 40, 40, 47}},
    {2, 6, {710, 793, 804, 772, 804, 797}, {792, 727, 771, 705, 747, 40, 798, 138, 0, 40, 40, 792}},
    {2, 7, {66, 92, 63, 74, 45, 51}, {73, 66, 80, 59, 72, 77, 92, 40, 0, 40, 67, 40}},
    {2, 7, {96, 42, 99, 63, 86, 94}, {82, 80, 46, 89, 62, 71, 42, 40, 0, 40, 71, 40}},
    {2, 7, {935, 715, 934, 968, 759, 654}, {869, 231, 752, 928, 819, 297, 715, 40, 0, 40, 251, 40}},
    {2, 8, {95, 84, 86, 60, 53, 88}, {87, 88, 79, 88, 83, 62, 84, 40, 79, 0, 63, 40}},
    {2, 8, {91, 85, 83, 85, 55, 64}, {85, 85, 79, 84, 82, 86, 85, 40, 85, 0, 82, 40}},
    {2, 8, {815, 731, 635, 857, 750, 950}, {682, 793, 741, 808, 767, 194, 731, 40, 756, 0, 830, 40}},
    {2, 9, {95, 60, 90, 70, 74, 48}, {84, 88, 62, 90, 73, 73, 65, 0, 40, 60, 66, 40}},
    {2, 9, {99, 97, 56, 67, 96, 58}, {75, 98, 93, 94, 94, 73, 102, 0, 40, 97, 71, 40}},
    {2, 9, {794, 947, 947, 909, 916, 941}, {933, 824, 256, 789, 866, 258, 952, 0, 40, 947, 906, 40}},
    {2, 10, {85, 68, 52, 64, 61, 96}, {60, 82, 71, 85, 77, 64, 68, 92, 67, 68, 63, 62}},
    {2, 10, {52, 62, 40, 82, 62, 84}, {48, 54, 60, 52, 57, 80, 62, 83, 66, 62, 76, 66}},
    {2, 10, {687, 927, 733, 936, 813, 980}, {787, 735, 223, 687, 808, 268, 927, 318, 273, 927, 244, 837}},
    {3, 0, {88, 95, 85, 54, 66, 95}, {89, 89, 94, 88, 92, 56, 95, 89, 87, 95, 58, 0}},
    {3, 0, {95, 62, 82, 65, 40, 47}, {78, 88, 69, 95, 79, 63, 62, 50, 63, 62, 58, 0}},
    {3, 0, {944, 982, 685, 861, 827, 705}, {800, 296, 319, 944, 963, 203, 982, 728, 302, 982, 851, 0}},
    {3, 1, {56, 87, 94, 87, 66, 69}, {93, 67, 86, 61, 77, 80, 87, 67, 87, 77, 0, 0}},
    {3, 1, {75, 73, 80, 79, 89, 80}, {83, 80, 78, 80, 80, 75, 73, 75, 74, 63, 0, 0}},
    {3, 1, {978, 610, 653, 757, 638, 897}, {678, 254, 689, 983, 799, 85, 610, 216, 639, 600, 0, 0}},
    {3, 2, {42, 43, 66, 59, 63, 75}, {61, 43, 42, 42, 43, 59, 46, 78, 51, 43, 55, 0}},
    {3, 2, {80, 85, 77, 76, 58, 52}, {84, 81, 84, 80, 83, 74, 88, 60, 88, 85, 65, 0}},
    {3, 2, {688, 735, 835, 768, 665, 666}, {795, 697, 726, 688, 712, 103, 738, 686, 747, 735, 733, 0}},
    {3, 3, {97, 68, 95, 94, 84, 84}, {90, 97, 73, 97, 83, 93, 71, 85, 76, 73, 0, 0}},
    {3, 3, {64, 66, 75, 85, 51, 63}, {74, 69, 66, 64, 65, 82, 69, 67, 73, 71, 0, 0}},
    {3, 3, {958, 795, 694, 845, 893, 920}, {754, 275, 828, 958, 877, 194, 798, 254, 808, 800, 0, 0}},
    {3, 4, {96, 98, 70, 82, 50, 44}, {81, 92, 90, 89, 90, 79, 98, 40, 84, 0, 72, 0}},
    {3, 4, {74, 55, 69, 73, 40, 47}, {65, 65, 52, 67, 58, 70, 55, 40, 49, 0, 63, 0}},
    {3, 4, {975, 602, 891, 948, 872, 740}, {814, 240, 670, 968, 782, 285, 602, 40, 662, 0, 270, 0}},
    {3, 5, {80, 40, 77, 60, 66, 56}, {66, 67, 41, 73, 53, 56, 40, 57, 44, 40, 62, 0}},
    {3, 5, {50, 48, 77, 98, 43, 65}, {65, 45, 41, 43, 42, 87, 48, 70, 58, 48, 82, 0}},
    {3, 5, {986, 749, 681, 824, 629, 956}, {733, 278, 789, 979, 861, 144, 749, 281, 764, 749, 766, 0}},
    {3, 6, {89, 66, 54, 91, 52, 47}, {62, 84, 66, 84, 73, 82, 71, 54, 0, 66, 75, 0}},
    {3, 6, {71, 50, 57, 57, 82, 63}, {57, 67, 49, 66, 56, 54, 55, 63, 0, 50, 60, 0}},
    {3, 6, {688, 959, 781, 934, 873, 641}, {827, 742, 245, 683, 819, 267, 964, 685, 0, 959, 911, 0}},
    {3, 7, {87, 83, 47, 86, 48, 92}, {62, 82, 76, 80, 79, 87, 83, 40, 83, 78, 77, 0}},
    {3, 7, {48, 49, 74, 45, 42, 88}, {64, 43, 42, 41, 42, 50, 49, 40, 48, 44, 48, 0}},
    {3, 7, {967, 718, 914, 911, 989, 912}, {860, 257, 760, 960, 836, 269, 718, 40, 756, 713, 938, 0}},
    {3, 8, {50, 61, 89, 43, 97, 61}, {76, 47, 52, 43, 49, 52, 61, 58, 58, 0, 64, 0}},
    {3, 8, {91, 80, 67, 52, 59, 51}, {73, 84, 75, 84, 79, 56, 80, 51, 74, 0, 59, 0}},
    {3, 8, {974, 606, 718, 792, 949, 649}, {710, 240, 673, 967, 783, 155, 606, 671, 643, 0, 844, 0}},
    {3, 9, {90, 90, 55, 76, 68, 86}, {71, 90, 85, 85, 85, 78, 95, 0, 82, 90, 68, 0}},
    {3, 9, {90, 78, 95, 96, 45, 96}, {91, 88, 75, 85, 79, 94, 83, 0, 76, 78, 76, 0}},
    {3, 9, {996, 827, 791, 729, 760, 901}, {825, 306, 200, 991, 907, 80, 832, 0, 147, 827, 733, 0}},
    {3, 10, {42, 77, 65, 41, 59, 50}, {66, 49, 70, 42, 60, 43, 77, 49, 70, 77, 47, 0}},
    {3, 10, {48, 40, 96, 58, 91, 75}, {75, 46, 42, 48, 44, 61, 40, 73, 44, 40, 68, 0}},
    {3, 10, {637, 620, 781, 683, 910, 949}, {719, 634, 623, 637, 629, 706, 620, 253, 633, 620, 751, 0}},
};

typedef struct {
    int cls;
    uint8_t rolls[6];
    /* hp, mp, carry, casting, casting max, hp max, mp max, carry max, str, dex, sta, int, wis, cha */
    uint16_t expected[14];
} RollVector;

static const RollVector kRolls[] = {
    {0, {8, 11, 0, 14, 7, 1}, {12, 0, 530, 0, 0, 12, 0, 530, 53, 56, 46, 45, 59, 52}},
    {0, {5, 3, 11, 15, 7, 12}, {14, 0, 500, 0, 0, 14, 0, 500, 50, 48, 57, 56, 60, 52}},
    {0, {3, 7, 0, 6, 13, 8}, {13, 0, 480, 0, 0, 13, 0, 480, 48, 52, 53, 45, 51, 58}},
    {1, {5, 12, 5, 2, 4, 14}, {15, 0, 500, 0, 0, 15, 0, 500, 50, 57, 59, 50, 47, 49}},
    {1, {4, 4, 0, 0, 6, 6}, {13, 0, 490, 0, 0, 13, 0, 490, 49, 49, 51, 45, 45, 51}},
    {1, {5, 5, 9, 10, 6, 6}, {13, 0, 500, 0, 0, 13, 0, 500, 50, 50, 51, 54, 55, 51}},
    {2, {5, 6, 12, 9, 0, 11}, {14, 0, 500, 0, 0, 14, 0, 500, 50, 51, 56, 57, 54, 45}},
    {2, {13, 5, 4, 8, 2, 10}, {14, 0, 580, 0, 0, 14, 0, 580, 58, 50, 55, 49, 53, 47}},
    {2, {9, 0, 10, 2, 9, 11}, {14, 0, 540, 0, 0, 14, 0, 540, 54, 45, 56, 55, 47, 54}},
    {3, {9, 15, 10, 5, 15, 15}, {15, 0, 540, 0, 0, 15, 0, 540, 54, 60, 60, 55, 50, 60}},
    {3, {5, 1, 8, 0, 11, 12}, {14, 0, 500, 0, 0, 14, 0, 500, 50, 46, 57, 53, 45, 56}},
    {3, {0, 13, 11, 12, 0, 14}, {15, 0, 450, 0, 0, 15, 0, 450, 45, 58, 59, 56, 57, 45}},
    {4, {1, 5, 6, 3, 7, 14}, {15, 12, 460, 58, 58, 15, 12, 460, 46, 50, 59, 51, 48, 52}},
    {4, {11, 11, 8, 14, 3, 11}, {14, 14, 560, 69, 69, 14, 14, 560, 56, 56, 56, 53, 59, 48}},
    {4, {9, 1, 13, 2, 6, 10}, {14, 11, 540, 57, 57, 14, 11, 540, 54, 46, 55, 58, 47, 51}},
    {5, {11, 4, 10, 8, 2, 9}, {14, 13, 560, 59, 59, 14, 13, 560, 56, 49, 54, 55, 53, 47}},
    {5, {10, 9, 5, 2, 4, 9}, {14, 12, 550, 53, 53, 14, 12, 550, 55, 54, 54, 50, 47, 49}},
    {5, {15, 5, 1, 2, 12, 1}, {12, 11, 600, 52, 52, 12, 11, 600, 60, 50, 46, 46, 47, 57}},
    {6, {7, 11, 8, 14, 13, 4}, {12, 7, 520, 59, 59, 12, 7, 520, 52, 56, 49, 53, 59, 58}},
    {6, {1, 1, 15, 10, 6, 4}, {12, 7, 460, 55, 55, 12, 7, 460, 46, 46, 49, 60, 55, 51}},
    {6, {4, 13, 3, 5, 13, 11}, {14, 6, 490, 50, 50, 14, 6, 490, 49, 58, 56, 48, 50, 58}},
    {7, {4, 1, 13, 9, 4, 14}, {15, 14, 490, 68, 68, 15, 14, 490, 49, 46, 59, 58, 54, 49}},
    {7, {5, 14, 15, 10, 15, 8}, {13, 15, 500, 70, 70, 13, 15, 500, 50, 59, 53, 60, 55, 60}},
    {7, {9, 15, 12, 4, 3, 12}, {14, 14, 540, 67, 67, 14, 14, 540, 54, 60, 57, 57, 49, 48}},
    {8, {5, 15, 10, 5, 2, 15}, {15, 13, 500, 59, 59, 15, 13, 500, 50, 60, 60, 55, 50, 47}},
    {8, {8, 11, 2, 11, 1, 9}, {14, 12, 530, 54, 54, 14, 12, 530, 53, 56, 54, 47, 56, 46}},
    {8, {11, 8, 15, 8, 9, 10}, {14, 14, 560, 63, 63, 14, 14, 560, 56, 53, 55, 60, 53, 54}},
    {9, {5, 0, 15, 8, 10, 8}, {13, 7, 500, 59, 59, 13, 7, 500, 50, 45, 53, 60, 53, 55}},
    {9, {14, 9, 11, 11, 8, 11}, {14, 7, 590, 56, 56, 14, 7, 590, 59, 54, 56, 56, 56, 53}},
    {9, {13, 11, 5, 14, 11, 10}, {14, 6, 580, 49, 49, 14, 6, 580, 58, 56, 55, 50, 59, 56}},
    {10, {4, 5, 6, 11, 15, 9}, {14, 12, 490, 61, 61, 14, 12, 490, 49, 50, 54, 51, 56, 60}},
    {10, {2, 13, 5, 13, 9, 8}, {13, 12, 470, 60, 60, 13, 12, 470, 47, 58, 53, 50, 58, 54}},
    {10, {0, 6, 5, 14, 5, 7}, {13, 12, 450, 60, 60, 13, 12, 450, 45, 51, 52, 50, 59, 50}},
    {11, {5, 1, 15, 7, 5, 1}, {12, 15, 500, 70, 70, 12, 15, 500, 50, 46, 46, 60, 52, 50}},
    {11, {4, 3, 10, 5, 15, 6}, {13, 13, 490, 65, 65, 13, 13, 490, 49, 48, 51, 55, 50, 60}},
    {11, {1, 13, 14, 11, 12, 2}, {12, 14, 460, 69, 69, 12, 14, 460, 46, 58, 47, 59, 56, 57}},
};

static void testDerived(void) {
    unsigned bad = 0, compared = 0;
    for (unsigned v = 0; v < sizeof(kDerived) / sizeof(kDerived[0]); v++) {
        const DerivedVector *d = &kDerived[v];
        GameKind game = d->game == 3 ? GameYendor3 : GameYendor2;
        uint8_t record[PartyRecordSize];
        memset(record, 0, sizeof(record));
        partySetU16(record, PartyFieldClass, (uint16_t)d->cls);
        for (unsigned i = 0; i < 6; i++) {
            partySetU16(record, PartyFieldStats + i * 2, d->attrs[i]);
        }
        partyComputeDerivedStats(record, game);
        for (unsigned i = 0; i < 12; i++) {
            if (game == GameYendor3 && kDerivedOffsets[i] == 0x70) {
                if (partyGetU16(record, 0x70) != 0 || partyGetU16(record, 0xB0) != 0) {
                    bad++;
                    printf("  Chapter 3 wrote Chemistry\n");
                }
                continue;
            }
            compared++;
            uint16_t cur = partyGetU16(record, kDerivedOffsets[i]);
            uint16_t mx = partyGetU16(record, kDerivedOffsets[i] + PartyMaxStatOffset);
            if (cur != d->expected[i] || mx != d->expected[i]) {
                bad++;
                printf("  game %d class %d offset 0x%02X: got %u/%u, want %u\n", d->game, d->cls, kDerivedOffsets[i], cur, mx,
                       d->expected[i]);
            }
        }
    }
    printf("compared %u derived values\n", compared);
    check("every derived stat matches the emulated original (both games, all classes)", bad == 0 && compared > 600);
}

static void testRolls(void) {
    unsigned bad = 0;
    for (unsigned v = 0; v < sizeof(kRolls) / sizeof(kRolls[0]); v++) {
        const RollVector *r = &kRolls[v];
        uint8_t record[PartyRecordSize];
        memset(record, 0xEE, sizeof(record));
        partySetU16(record, PartyFieldClass, (uint16_t)r->cls);
        partyApplyRolledAttributes(record, r->rolls);
        uint16_t got[14] = {partyGetU16(record, 0x52), partyGetU16(record, 0x54), partyGetU16(record, 0x56), partyGetU16(record, 0x62),
                            partyGetU16(record, 0xA2), partyGetU16(record, 0x92), partyGetU16(record, 0x94), partyGetU16(record, 0x96),
                            partyGetU16(record, 0x3C), partyGetU16(record, 0x3E), partyGetU16(record, 0x40), partyGetU16(record, 0x42),
                            partyGetU16(record, 0x44), partyGetU16(record, 0x46)};
        for (unsigned i = 0; i < 14; i++) {
            if (got[i] != r->expected[i]) {
                bad++;
                printf("  class %d rolls %u,%u,%u,%u,%u,%u field %u: got %u, want %u\n", r->cls, r->rolls[0], r->rolls[1], r->rolls[2],
                       r->rolls[3], r->rolls[4], r->rolls[5], i, got[i], r->expected[i]);
            }
        }
        if (partyGetU16(record, PartyFieldEquipRatingBase1) != 0 || partyGetU16(record, PartyFieldEquipRatingBase3Max) != 0) {
            bad++;
        }
    }
    check("every roll vector matches the emulated original (and resets the equipment baselines)", bad == 0);
}

static void testRollFromRng(void) {
    RandomState rng, peek;
    randomStart(&rng, 12, 34);
    peek = rng;
    uint8_t expected[6];
    for (unsigned i = 0; i < 6; i++) {
        expected[i] = (uint8_t)randomInRange(&peek, 15);
    }
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));
    partySetU16(record, PartyFieldClass, 7);
    partyRollAttributes(record, &rng);
    check("the six rolls are drawn in order Str, Dex, Int, Wis, Cha, Sta",
          partyGetU16(record, 0x3C) == 45u + expected[0] && partyGetU16(record, 0x3E) == 45u + expected[1] &&
              partyGetU16(record, 0x42) == 45u + expected[2] && partyGetU16(record, 0x44) == 45u + expected[3] &&
              partyGetU16(record, 0x46) == 45u + expected[4] && partyGetU16(record, 0x40) == 45u + expected[5]);
}

static void testStartingAbilities(void) {
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));
    partySetU16(record, PartyFieldClass, 4);
    check("a class with no magic points gets nothing", partyApplyStartingAbilities(record, GameYendor2) == 0 &&
                                                          !flagBankTest(record + PartyFieldFlagBankCA, 16, 1));
    partySetStatMax(record, PartyStatMagicPoints, 12);
    check("a MONK with magic gets flags 1 and 3", partyApplyStartingAbilities(record, GameYendor2) == 2 &&
                                                    flagBankTest(record + PartyFieldFlagBankCA, 16, 1) &&
                                                    flagBankTest(record + PartyFieldFlagBankCA, 16, 3) &&
                                                    !flagBankTest(record + PartyFieldFlagBankCA, 16, 2));
    check("Chapter 2 records the secondary-class bit", (partyGetU16(record, PartyFieldStatusFlags) & 0x20) != 0);

    memset(record, 0, sizeof(record));
    partySetU16(record, PartyFieldClass, 4);
    partySetStatMax(record, PartyStatMagicPoints, 12);
    partyApplyStartingAbilities(record, GameYendor3);
    check("Chapter 3 does not", partyGetU16(record, PartyFieldStatusFlags) == 0 && flagBankTest(record + PartyFieldFlagBankCA, 16, 3));

    memset(record, 0, sizeof(record));
    partySetU16(record, PartyFieldClass, 6);
    partySetStatMax(record, PartyStatMagicPoints, 3);
    check("a PALADIN gets one flag (the zero entry ends the pair)", partyApplyStartingAbilities(record, GameYendor2) == 1 &&
                                                                      flagBankTest(record + PartyFieldFlagBankCA, 16, 1) &&
                                                                      (partyGetU16(record, PartyFieldStatusFlags) & 0x08));
    memset(record, 0, sizeof(record));
    partySetU16(record, PartyFieldClass, 2);
    partySetStatMax(record, PartyStatMagicPoints, 3);
    check("a MERCHANT with magic points still gets nothing", partyApplyStartingAbilities(record, GameYendor2) == 0);
}

static void testClassSelection(void) {
    static ItemCatalog catalog; /* no items: the equipment ratings come only from their baselines */
    memset(&catalog, 0, sizeof(catalog));
    uint8_t record[PartyRecordSize];
    memset(record, 0, sizeof(record));
    partySetU16(record, PartyFieldStatusFlags, 0x4000 | 0x003F);
    flagBankSet(record + PartyFieldFlagBankCA, 16, 7);
    partyBeginClassSelection(record);
    check("entering the screen clears the six secondary-class bits and the ability flags",
          partyGetU16(record, PartyFieldStatusFlags) == 0x4000 && !flagBankTest(record + PartyFieldFlagBankCA, 16, 7));

    RandomState rng;
    randomStart(&rng, 1, 2);
    partyChooseClass(record, 7, GameYendor2, &catalog, &rng);
    check("choosing a class sets it and level 1", partyGetU16(record, PartyFieldClass) == 7 && partyGetU16(record, PartyFieldLevel) == 1);
    check("...and rolls the attributes (45-60) and the derived skills",
          partyGetStat(record, PartyStatStrength) >= 45 && partyGetStat(record, PartyStatStrength) <= 60 &&
              partyGetStat(record, PartyStatMapping) >= 40 && partyGetStat(record, PartyStatMagicPoints) > 0);
    check("a MAGE's casting skill is its Intelligence + 10", partyGetStat(record, PartyStatCasting) ==
                                                              partyGetStat(record, PartyStatIntelligence) + 10);
    uint16_t before = partyGetStat(record, PartyStatStrength);
    unsigned changed = 0;
    for (int i = 0; i < 8; i++) {
        partyRerollAttributes(record, GameYendor2, &catalog, &rng);
        changed += partyGetStat(record, PartyStatStrength) != before;
    }
    check("rerolling gives new values", changed > 0);
}

int main(void) {
    testClassSelection();
    testStartingAbilities();
    testDerived();
    testRolls();
    testRollFromRng();

    if (g_failureCount == 0) {
        printf("\nAll tests passed.\n");
        return 0;
    }
    printf("\n%d test(s) failed.\n", g_failureCount);
    return 1;
}

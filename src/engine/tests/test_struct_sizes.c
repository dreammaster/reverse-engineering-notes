/* Step 0 verification (see src/PLAN.md). Every AGS_STATIC_ASSERT in
 * ags/*.h already fires at COMPILE time when this file is built --
 * simply compiling successfully is this milestone's primary proof.
 * This also prints every struct's runtime sizeof() against its
 * struct-layout-drift.md-confirmed value, as a human-readable summary.
 */
#include "ags/all.h"

#include <stdio.h>

#define CHECK(type, expected) \
    do { \
        size_t actual = sizeof(struct type); \
        int ok = (actual == (size_t)(expected)); \
        printf("%-24s sizeof=0x%-6zx expected=0x%-6x %s\n", \
               #type, actual, (unsigned)(expected), ok ? "OK" : "MISMATCH"); \
        if (!ok) all_ok = 0; \
    } while (0)

int main(void)
{
    int all_ok = 1;

    printf("=== AGS struct size verification (Step 0) ===\n");

    CHECK(EventBlock, 0x94);
    CHECK(AnimationStruct, 0x18);
    CHECK(FullAnimation, 0xF4);
    CHECK(sprstruc, 0x0A);
    CHECK(PolyPoints, 0xF4);
    CHECK(RoomObject, 0x20);
    CHECK(RoomStatus, 0x1390);

    CHECK(CharacterInfo, 0x140);

    CHECK(ccScript, 0x1C50);
    CHECK(ccInstance, 0x9A8);
    CHECK(ExecutingScript, 0x6C);

    CHECK(DialogTopic, 0x484);
    CHECK(WordsDictionary, 0xBB84);

    CHECK(MouseCursor, 0x18);
    CHECK(InterfaceElement, 0x334);
    CHECK(InventoryItemInfo, 0x44);
    CHECK(GameSetupStructBase, 0xBF84);

    CHECK(ScreenOverlay, 0x14);
    CHECK(GameState, 0x964);

    CHECK(MoveList, 0x200);
    CHECK(ViewFrame272, 0x1C);
    CHECK(ViewStruct272, 0x8D4);

    CHECK(SOUNDCLIP, 0x08);
    CHECK(MYWAVE, 0x10);
    CHECK(MYMP3, 0x18);
    CHECK(MYSTATICMP3, 0x18);

    CHECK(SpriteCache, 0x30);
    CHECK(SpriteListEntry, 0x14);

    CHECK(TreeMap, 0x10);
    CHECK(EventHappened, 0x14);
    CHECK(OnScreenWindow, 0x10);

    CHECK(GUIMain, 0x184);
    CHECK(GUIButton, 0x84);
    CHECK(GUITextBox, 0xF4);
    CHECK(GUILabel, 0xF4);
    CHECK(GUIListBox, 0x1DC);
    CHECK(GUIInv, 0x24);
    CHECK(GUISlider, 0x30);

    printf("\n%s\n", all_ok ? "ALL STRUCT SIZES OK" : "SOME STRUCT SIZES MISMATCHED");
    return all_ok ? 0 : 1;
}
END_OF_MAIN()

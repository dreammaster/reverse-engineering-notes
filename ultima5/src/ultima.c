#include <stdlib.h>

#include "common/common.h"
#include "backend/backend.h"
#include "vars.h"
#include "funcs.h"

#if !defined(TARGET_DOS16)
#include "common/file.h"
#endif

#if defined(ENABLE_TRANSLATION)
#include "translate.h"
#endif

#include "dungeon.h"
#include "endgame.h"
#include "intro.h"
#include "mainout.h"
#include "outsubs.h"
#include "town.h"
#include "talk.h"

#if !defined(TARGET_DOS16)
extern int g_enableDebugOverlay;
#endif

#if defined(BACKEND_SDL)
#include <SDL3/SDL_main.h>
#endif

// 0000
int CDECL main(int argc, char** argv/*, char** envp*/)
{
    u16 local_8;
    u8 local_6; // hard drive letter (0xff for floppy)
    u8 local_4;
    u16 local_2;

    local_4 = 0x20;

#if !defined(TARGET_DOS16)
    if (!FILE_CheckGameFiles())
    {
        DEBUG_ShowError("Required Ultima V game data could not be found.");
        return 1;
    }
#endif

#if defined(ENABLE_TRANSLATION)
    TRS_Initialize();
#endif

#if !defined(TARGET_DOS16)
    BACKEND_Initialize();
    atexit(BACKEND_Cleanup);
#endif

    if (argc > 1)
    {
        local_4 = ULTIMA_2032_ToUpper((unsigned char)argv[1][0]);
    }

    // 0021
    D_52ba_vdp._52ba_forceCga = local_4 == 'C';
    D_52f3_forceHerc = local_4 == 'H';
    D_52f1_forceTandy = local_4 == 'T';
    D_52ef_forceEga = local_4 == 'E';

    // 0061
    D_5394_fn = ULTIMA_2322_DiskSwapMessage;

    D_a9bd[0] = 0;
    D_a9bd[1] = 0;
    D_a9c2 = 1;

    if ((D_a9c8[0] = ULTIMA_16a6_GetDefaultDrive()) >= 'C')
        local_6 = D_a9c8[0];
    else
        local_6 = 0xff;

    // 0090
    D_a9cd = local_6;
    D_a9ca = local_6;
    D_a9c9 = local_6;

    D_a9cb = 0xff;
    D_b11c = D_b21e;
    D_538c = 1;

    INTRO_0986_Main(); // 00ad
    ULTIMA_2900_UpdateVitalsDisplay();

#if !defined(TARGET_DOS16)
    g_enableDebugOverlay = 1;
#endif

#if !defined(TARGET_DOS16)
    // test hook (not in the original): U5_ENDGAME=1 plays the ending sequence straight away
    if (getenv("U5_ENDGAME"))
    {
        // the party arrives from the final combat, so start its actors at combat-map positions
        for (int i = 0; i < 8; i++)
        {
            D_5c5a[i]._2_x = 5;
            D_5c5a[i]._3_y = 9;
        }
        D_5893_map_id = 0xff; // the combat map, as set by the dungeon code before the final fight
        D_58a1 = 0x82;
        D_5896_map_x = 5;
        D_5897_map_y = 9;
        ENDGAME_0648_EndgameMain();
        return 0;
    }
#endif

    local_8 = 0;

    // 00b8
    // main game loop
    do
    {
        local_2 = 0;

        if (D_5893_map_id == 0)
        {
            MAINOUT_0d22_Entry();
            local_2 = 1;
            local_8 = 0;
        }
        
        // 00d1
        if (D_5893_map_id != 0)
        {
            // 00db
            if (D_5893_map_id < 0x21)
            {
                TOWN_11f0_Entry(local_2 != 0 || local_8 != 0);
#if !defined(TARGET_DOS16)
                // test hook (not in the original): U5_MERCHANT=n starts the shop conversation of dialog n on arrival
                if (getenv("U5_MERCHANT") && *getenv("U5_MERCHANT"))
                {
                    TALK_TestMerchant(atoi(getenv("U5_MERCHANT")));
                    _putenv("U5_MERCHANT=");
                }
#endif
                TOWN_141e_MainLoop();
                local_8 = 0;
            }
            else
            {
                // 0104
                ULTIMA_251e_SwitchDisks(2);
                DUNGEON_0e2e_MainLoop(local_2);
                local_8 = 1;
            }

            // 0116
            local_2 = 0;
            ULTIMA_251e_SwitchDisks(1);

            // 0122
            while (!ULTIMA_1674_TestOpenFile(/*0x1393*/ "BRIT.DAT")) {}
            ULTIMA_256e_ReadFileFromDisk(OUTSUBS_0368_GetWorldSavefile(), D_5c5a, 0x100, 0);

            if (D_5893_map_id == 0 && D_5895_map_level != 0)
            {
                ULTIMA_251e_SwitchDisks(5);

                // 0154
                while (!ULTIMA_1674_TestOpenFile(/*0x139c*/ "UNDER.DAT")) {}
                ULTIMA_25d8_WriteFileToDisk(OUTSUBS_0368_GetWorldSavefile(), D_5c5a, 0x100);
            }
        }
        // 016e
    } while (local_2 == 0);

    ULTIMA_0878_RestoreVideoMode();
}

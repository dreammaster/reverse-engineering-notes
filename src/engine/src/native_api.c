/* ags/native_api.h's own implementation. See that header for the
 * complete calling convention and real-vs-stubbed scope.
 */
#include "ags/native_api.h"
#include "ags/audio.h"
#include "ags/inventory.h"
#include "ags/invscreen.h"
#include "ags/room_loader.h"
#include "ags/stub.h"

#include <allegro.h>
#include <stdio.h>
#include <string.h>

static struct AgsNativeApiContext *s_ctx = NULL;

void ags_native_api_set_context(struct AgsNativeApiContext *ctx)
{
    s_ctx = ctx;
}

static const char *arg_str(const long *args, int i)
{
    return (const char *)(size_t)(unsigned)args[i];
}

static long native_Display(const long *args, int numargs)
{
    /* Simplified: shows args[0] verbatim, no printf-style %d/%s
     * substitution against any further pushed arguments -- a
     * documented scope decision (see this file's own dispatch table
     * comment), not a claim that this build's real Display() is
     * single-argument only. */
    if (numargs < 1 || !s_ctx || !s_ctx->game_ctx || !s_ctx->game_ctx->rst) {
        return 0;
    }
    ags_display_text_box(s_ctx->screen_w, s_ctx->screen_h, NULL, arg_str(args, 0));
    return 0;
}

static long native_DisplayMessage(const long *args, int numargs)
{
    if (numargs < 1 || !s_ctx || !s_ctx->game_ctx || !s_ctx->game_ctx->rst) {
        return 0;
    }
    ags_display_message(s_ctx->game_ctx->rst, (int)args[0]);
    return 0;
}

static long native_DisplaySpeech(const long *args, int numargs)
{
    int who;
    const char *name = NULL;

    if (numargs < 2 || !s_ctx) {
        return 0;
    }
    who = (int)args[0];
    if (s_ctx->chars && who >= 0 && who < s_ctx->numcharacters) {
        name = s_ctx->chars[who].name;
    }
    ags_display_text_box(s_ctx->screen_w, s_ctx->screen_h, name, arg_str(args, 1));
    return 0;
}

static long native_NewRoomEx(const long *args, int numargs)
{
    /* Same SCOPED real reload as DCMD_NEWROOM (ags/dialog_run.c) --
     * ags_load_room only, no fade/RoomStatus-persistence machinery. */
    char roomfile[32];
    if (numargs < 3 || !s_ctx || !s_ctx->game_ctx || !s_ctx->game_ctx->rst) {
        return 0;
    }
    sprintf(roomfile, "room%d.crm", (int)args[0]);
    if (ags_load_room(roomfile, s_ctx->game_ctx->rst) == AGS_ROOM_LOAD_OK && s_ctx->game_ctx->player) {
        s_ctx->game_ctx->player->room = (int)args[0];
        s_ctx->game_ctx->player->x = (int)args[1];
        s_ctx->game_ctx->player->y = (int)args[2];
    }
    return 0;
}

static long native_ProcessClick(const long *args, int numargs)
{
    int hs;
    if (numargs < 3 || !s_ctx || !s_ctx->game_ctx || !s_ctx->game_ctx->rst) {
        return 0;
    }
    hs = ags_get_hotspot_at(s_ctx->game_ctx->rst, (int)args[0], (int)args[1]);
    if (hs > 0) {
        ags_run_hotspot_interaction(s_ctx->game_ctx, hs, (int)args[2]);
    }
    return 0;
}

static long native_QuitGame(const long *args, int numargs)
{
    /* Real ConfirmQuit-dialog confirmation step (CSCI, out of scope)
     * is skipped -- QuitGame(0) and QuitGame(1) both request a quit
     * unconditionally here, a documented simplification. */
    (void)args; (void)numargs;
    if (s_ctx && s_ctx->game_ctx) {
        s_ctx->game_ctx->quit_requested = 1;
    }
    return 0;
}

static long native_InventoryScreen(const long *args, int numargs)
{
    (void)args; (void)numargs;
    if (s_ctx && s_ctx->game_ctx && s_ctx->game) {
        ags_run_inventory_screen(s_ctx->game_ctx, s_ctx->game, s_ctx->sprites,
                                  s_ctx->screen_w, s_ctx->screen_h);
    }
    return 0;
}

static long native_IsGamePaused(const long *args, int numargs)
{
    (void)args; (void)numargs;
    return s_ctx ? s_ctx->game_paused : 0;
}

static long native_GetCursorMode(const long *args, int numargs)
{
    (void)args; (void)numargs;
    return s_ctx ? s_ctx->cur_cursor : 0;
}

static long native_SetCursorMode(const long *args, int numargs)
{
    if (numargs < 1 || !s_ctx) {
        return 0;
    }
    s_ctx->cur_cursor = (int)args[0];
    return 0;
}

static long native_GetPlayerCharacter(const long *args, int numargs)
{
    /* This game's own pre-object-oriented script API: GetPlayerCharacter()
     * returns the player's plain character INDEX (not a pointer/handle --
     * that convention is a later AGS addition this build predates). */
    (void)args; (void)numargs;
    return (s_ctx && s_ctx->game) ? s_ctx->game->playercharacter : 0;
}

static long native_AddInventory(const long *args, int numargs)
{
    if (numargs < 1 || !s_ctx || !s_ctx->game_ctx || !s_ctx->game_ctx->player || !s_ctx->game_ctx->play) {
        return 0;
    }
    ags_add_inventory(s_ctx->game_ctx->player, s_ctx->game_ctx->play, (int)args[0]);
    return 0;
}

static long native_GetCurrentMusic(const long *args, int numargs)
{
    (void)args; (void)numargs;
    return (s_ctx && s_ctx->game_ctx && s_ctx->game_ctx->play) ? ags_get_current_music(s_ctx->game_ctx->play) : -1;
}

static long native_SaveScreenShot(const long *args, int numargs)
{
    if (numargs < 1) {
        return 0;
    }
    /* NULL palette -- Allegro's own "use whatever hardware palette is
     * currently active" convention, a real option, not a placeholder. */
    return save_bitmap(arg_str(args, 0), screen, NULL) == 0 ? 1 : 0;
}

static long native_SetGlobalInt(const long *args, int numargs)
{
    int idx;
    if (numargs < 2 || !s_ctx || !s_ctx->game_ctx || !s_ctx->game_ctx->play) {
        return 0;
    }
    idx = (int)args[0];
    if (idx >= 0 && idx < 300) {
        s_ctx->game_ctx->play->globalscriptvars[idx] = (int)args[1];
    }
    return 1;
}

static long native_StartCutscene(const long *args, int numargs)
{
    (void)args; (void)numargs;
    if (s_ctx && s_ctx->game_ctx && s_ctx->game_ctx->play) {
        s_ctx->game_ctx->play->in_cutscene = 1;
    }
    return 0;
}

static long native_EndCutscene(const long *args, int numargs)
{
    (void)args; (void)numargs;
    if (s_ctx && s_ctx->game_ctx && s_ctx->game_ctx->play) {
        s_ctx->game_ctx->play->in_cutscene = 0;
    }
    return 0;
}

static long native_InterfaceOff(const long *args, int numargs)
{
    /* DisableInterface's own confirmed real semantics: a NESTING
     * COUNTER, not a boolean (matches.json's own entry, four
     * independent call sites). */
    (void)args; (void)numargs;
    if (s_ctx && s_ctx->game_ctx && s_ctx->game_ctx->play) {
        s_ctx->game_ctx->play->disabled_user_interface++;
    }
    return 0;
}

static long native_InterfaceOn(const long *args, int numargs)
{
    (void)args; (void)numargs;
    if (s_ctx && s_ctx->game_ctx && s_ctx->game_ctx->play &&
        s_ctx->game_ctx->play->disabled_user_interface > 0) {
        s_ctx->game_ctx->play->disabled_user_interface--;
    }
    return 0;
}

struct AgsNativeApiEntry {
    const char *name;
    long (*handler)(const long *args, int numargs);
};

static const struct AgsNativeApiEntry s_dispatch[] = {
    { "Display", native_Display },
    { "DisplayMessage", native_DisplayMessage },
    { "DisplaySpeech", native_DisplaySpeech },
    { "NewRoomEx", native_NewRoomEx },
    { "ProcessClick", native_ProcessClick },
    { "QuitGame", native_QuitGame },
    { "InventoryScreen", native_InventoryScreen },
    { "IsGamePaused", native_IsGamePaused },
    { "GetCursorMode", native_GetCursorMode },
    { "SetCursorMode", native_SetCursorMode },
    { "GetPlayerCharacter", native_GetPlayerCharacter },
    { "AddInventory", native_AddInventory },
    { "GetCurrentMusic", native_GetCurrentMusic },
    { "SaveScreenShot", native_SaveScreenShot },
    { "SetGlobalInt", native_SetGlobalInt },
    { "StartCutscene", native_StartCutscene },
    { "EndCutscene", native_EndCutscene },
    { "InterfaceOff", native_InterfaceOff },
    { "InterfaceOn", native_InterfaceOn },
};
#define AGS_NATIVE_API_DISPATCH_COUNT (sizeof(s_dispatch) / sizeof(s_dispatch[0]))

long ags_native_api_call(const char *name, const long *args, int numargs)
{
    size_t i;

    if (name) {
        for (i = 0; i < AGS_NATIVE_API_DISPATCH_COUNT; i++) {
            if (strcmp(s_dispatch[i].name, name) == 0) {
                return s_dispatch[i].handler(args, numargs);
            }
        }
    }
    ags_stub_hit(name ? name : "<unresolved import>", "<script CALLEXT>", 0);
    return 0;
}

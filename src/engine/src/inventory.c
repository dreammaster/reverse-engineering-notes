/* ags/inventory.h's own implementation. See that header for the
 * full evidence and real-vs-not-ported split.
 */
#include "ags/inventory.h"

void ags_add_inventory(struct CharacterInfo *playerchar, struct GameState *play, int inum)
{
    int i;

    if (inum < 0 || inum >= 100) {
        return; /* source's own "!AddInventory: invalid invnetory number" quit() */
    }
    playerchar->inv[inum]++;

    for (i = 0; i < play->inv_numorder; i++) {
        if (play->play_invorder[i] == inum) {
            return; /* already present */
        }
    }
    if (play->inv_numorder < 100) {
        play->play_invorder[play->inv_numorder] = (short)inum;
        play->inv_numorder++;
    }
}

void ags_lose_inventory(struct CharacterInfo *playerchar, struct GameState *play, int inum)
{
    int i, j;

    if (inum < 0 || inum >= 100) {
        return;
    }
    if (playerchar->inv[inum] > 0) {
        playerchar->inv[inum]--;
    }
    if (playerchar->activeinv == inum && playerchar->inv[inum] <= 0) {
        playerchar->activeinv = -1;
    }
    if (playerchar->inv[inum] <= 0) {
        for (i = 0; i < play->inv_numorder; i++) {
            if (play->play_invorder[i] == inum) {
                play->inv_numorder--;
                for (j = i; j < play->inv_numorder; j++) {
                    play->play_invorder[j] = play->play_invorder[j + 1];
                }
                break;
            }
        }
    }
}

void ags_update_invorder(struct CharacterInfo *playerchar, struct GameState *play, int numinvitems)
{
    int ff;

    play->inv_numorder = 0;
    for (ff = 0; ff < numinvitems && ff < 100; ff++) {
        if (playerchar->inv[ff] > 0 && play->inv_numorder < 100) {
            play->play_invorder[play->inv_numorder] = (short)ff;
            play->inv_numorder++;
        }
    }
}

int ags_set_active_inventory(struct CharacterInfo *playerchar, int iit)
{
    if (iit == -1) {
        playerchar->activeinv = -1;
        return 0; /* AGS_MODE_WALK */
    }
    if (iit < 1 || iit >= 100) {
        return 0; /* source's own "!SetActiveInventory: invalid inventory number" quit() */
    }
    if (playerchar->inv[iit] < 1) {
        return 0; /* source's own "...player doesn't have this item" quit() */
    }
    playerchar->activeinv = iit;
    return 4; /* AGS_MODE_USE */
}

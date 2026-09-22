/* ags/gui_popup.h's own implementation. See that header for the
 * complete evidence and scope decision.
 */
#include "ags/gui_popup.h"

#define AGS_POPUP_MOUSEY 1

void ags_gui_update_popups(struct AgsGuiSet *set, struct AgsGuiPopupState *state, int mouse_y)
{
    int i;
    int newpopped = -1;

    for (i = 0; i < set->numgui; i++) {
        if (set->guis[i].popup == AGS_POPUP_MOUSEY && mouse_y < set->guis[i].popupyp) {
            newpopped = i; /* first match in array order -- only one at a time, matching the real single `ifacepopped` global */
            break;
        }
    }

    for (i = 0; i < set->numgui; i++) {
        if (set->guis[i].popup == AGS_POPUP_MOUSEY) {
            set->guis[i].on = (i == newpopped) ? 1 : 0;
        }
    }

    state->ifacepopped = newpopped;
}

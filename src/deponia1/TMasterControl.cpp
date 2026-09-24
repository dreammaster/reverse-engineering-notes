#include "TMasterControl.h"

#include "vsplayer/control/gameController.h"

TGameController* TMasterControl::GetGameController() {
    if (!m_gameController) {
        m_gameController = new TGameController();
    }
    return m_gameController;
}

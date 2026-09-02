// The interactive "Training Grounds" -- character creation + editing.
// Ports SEGMENT PROCEDURE ROLLER (P010B01) from the Apple source; rules live
// in wiz/roller.h, this is the menu flow.
#pragma once
#include "wiz/platform.h"
#include "wiz/textscreen.h"
#include "wiz/font.h"
#include "wiz/rng.h"
#include "wiz/roster.h"

namespace wiz {

class Scenario;

// A thin console: renders a TextScreen through the Font and reads the
// keyboard the way GETKEY / GETLINE / GETPASS do.
class Ui {
public:
    Ui(Platform &p, const Font &font) : p_(p), font_(font) {}

    TextScreen &ts() { return ts_; }
    void refresh();
    int  getKey();                            // blocking; letters upper-cased
    std::string getLine(int maxLen);          // echoed entry, ends on RETURN
    std::string getPass(int maxLen, Rng &rng);// echoes 1-2 'X' per keystroke
    int  menu(const char *valid);             // wait for one of `valid`
    void pressAnyKey(const char *msg = "PRESS ANY KEY TO CONTINUE");
    bool quit() const { return quit_; }

private:
    Platform &p_;
    const Font &font_;
    TextScreen ts_;
    Surface surf_;
    bool quit_ = false;
};

// Show the "Wizardry" logo (200/400.TITLE bytes) + welcome line; wait for a
// key.  Returns false if the window was closed.
bool showTitle(Platform &p, const Font &font, Bytes titleData);

// Runs the Training Grounds until the player leaves for the castle (empty
// name) or closes the window.  `rosterPath` is written on any change.
void runRoller(Ui &ui, Roster &roster, const Scenario &sc, Rng &rng,
               const std::string &rosterPath);

} // namespace wiz

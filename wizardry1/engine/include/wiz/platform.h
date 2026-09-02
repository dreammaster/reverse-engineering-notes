// Platform abstraction -- input + presenting a Surface.  The standalone
// engine talks only to this; backends: NullPlatform (headless, PPM dumps)
// and SdlPlatform (window).  A future ScummVM engine backs it with OSystem.
#pragma once
#include "wiz/surface.h"
#include <memory>

namespace wiz {

// Key codes the game logic sees.  Printable keys map to their ASCII value;
// the arrows use the codes SYSTEM.INTERP's keyboard SBIOS produces (sub_741).
enum Key : int {
    KEY_NONE  = 0,
    KEY_LEFT  = 0x09,
    KEY_DOWN  = 0x0A,
    KEY_UP    = 0x0B,
    KEY_RIGHT = 0x0C,
    KEY_RETURN = 0x0D,
    KEY_ESC   = 0x1B,
    KEY_BACKSPACE = 0x08,
    KEY_QUIT  = -1,           // window closed
};

struct Platform {
    virtual ~Platform() = default;

    // Copy `s` (interpreted through `palette`) to the display.
    virtual void present(const Surface &s, const Color *palette, int paletteLen) = 0;

    // Non-blocking: KEY_NONE if nothing pending.
    virtual int pollKey() = 0;
    // Blocking: pumps events, returns the next key (KEY_QUIT if closed).
    virtual int waitKey() = 0;

    virtual void delayMs(int ms) = 0;
    virtual bool running() const = 0;
};

// Headless.  `keyScript` feeds waitKey()/pollKey() one char at a time (with
// "\r"=RETURN, "\x1b"=ESC, "\b"=BACKSPACE, "\x09..\x0c"=arrows); when it runs
// out, keys return KEY_QUIT.  `dumpDir` (if set) writes each present() as a PPM.
std::unique_ptr<Platform> makeNullPlatform(const std::string &keyScript = "",
                                           const std::string &dumpDir = "");
std::unique_ptr<Platform> makeSdlPlatform(const char *title, int scale);  // nullptr if no SDL

} // namespace wiz

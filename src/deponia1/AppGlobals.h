// Global state referenced by main(). Names follow the original binary's
// symbol table where one exists (e.g. VSPlayerWindow, g_logfile,
// isProgramLooping); the anonymous IDA byte_11F8B01/byte_11F8B02 flags have
// no recovered name so they keep their IDA labels until their purpose is
// identified.
#pragma once

#include <cstdio>

#include "SdlStub.h"
#include "TStandardPaths.h"
#include "WxStub.h"
#include "graphicslib/vector3d.h"

class TMasterControl;

extern SDL_Window *VSPlayerWindow;
extern SDL_GLContext VSPlayerContext;

extern wxString strAppName;
extern wxSize surfaceSize;
extern wxSize renderSize;

// Confirmed referenced by TGameControl::UpdateAspectRatio (Deponia_Linux.asm
// line 457430): when set, the aspect ratio is forced to renderSize instead
// of the game data's configured aspect point. Not yet seen written anywhere.
extern bool g_unlockAspect;

extern TStandardPaths standardPaths;

// wxFileName, not wxString: main() calls GetFullPath() on it, which is a
// wxFileName member (see NOTES.md for how the ICF-folded symbol names
// obscured this).
extern wxFileName g_logfile;

extern std::FILE *LogFile;

// Confirmed real, named globals of the Lua object `graphics` (graphics_newindex, asm 438488): the matrices that the scripts set
// (9 numbers each, or none: nothing is drawn through a matrix - see matricesActive) and the font and shader numbers. The
// scripts that draw (luaDrawBeforeScene and the others) are lists of the names of Lua functions that TMasterControl::Draw()
// runs before the scene, after it and after the interfaces.
extern std::vector<float> matrix1;
extern std::vector<float> matrix2;
extern std::vector<float> textMatrix;
extern std::vector<float> invMatrix1;

/** The point, moved through invMatrix1 (as the row vector x, y, 1 times the matrix; the numbers cut to whole ones). Not
 *  called unless invMatrix1 is set - the callers test hasInverseMatrix(). */
wxPoint transformByInverseMatrix(const wxPoint &point);
/** invMatrix1 is set (it has the 9 numbers). */
bool hasInverseMatrix();
/** matrix1 is set (it has the 9 numbers). */
bool hasMatrix();
/** The point, moved through matrix1 (as transformByInverseMatrix(), for the drawing; the numbers are not cut). */
idVec3 transformByMatrix(const wxPoint &point);
extern std::vector<int> fontShaderIndizes;
extern int fontShader;
extern std::vector<std::string> luaDrawBeforeScene;
extern std::vector<std::string> luaDrawAfterScene;
extern std::vector<std::string> luaDrawAfterInterfaces;
// Confirmed real, named globals (graphics.box2DOffset, asm 440260): where the origin of the Box2D world is in the game.
extern float b2xoffset;
extern float b2yoffset;

extern int AppStatus;
extern int isProgramLooping;
extern int eMouseMessage;
extern unsigned char byte_11F8B01;
extern unsigned char byte_11F8B02;

// Confirmed real, named globals of the main loop (ShowFrame, Deponia_Linux.asm lines 497745-498889): the position of
// the mouse in the game's coordinates, the number of fingers on a touch screen, whether the left button is down and
// has not been counted as a long click yet, and the time of the last multi-finger gesture.
extern wxPoint mousePos;
extern int numFingers;
extern int bLeftButtonPressed;
extern unsigned int lastMultigestureTicks;

extern TMasterControl *g_pGameControl;
// Confirmed an exported, recovered global (asm line 26933): the part of the window where the game is drawn
// (the window less the bars of the aspect ratio); set by Init() and CreateWindowGL() (mainSDL, not reconstructed).
extern wxRect g_displayedArea;
// Confirmed an exported, recovered global (asm line 5295779): the game settings'
// "hold time" - set from the game object's field when it changes (THGameControl).
extern int GameMinDownTime;
// Confirmed an exported, recovered global (asm line 5295800): a bit mask of the
// kinds of trace logging that are switched on (written by the command line flag
// parsing, asm lines 234566-234769). Bit 0 (1) is the animation tracing of
// TGAnimation (used together with wxLog::loglevel > 1); bits 1 (2) and 2 (4)
// are tested by other code that is not reconstructed.
extern int g_traceFlags;

// Confirmed a real, named global (recovered symbol) - what the engine is doing right now, as the
// text the loading screen shows ("Loading", "Preparing Scene", "Lua Script"): written by
// TGScene::Prepare() and by the Lua script runner (which sets it back to "Loading"), never read by
// any code reversed so far.
extern const wchar_t *g_loadingState;

// Confirmed a real, named global (recovered symbol; IDA types it wxFileName
// but every use, TMSavegame::SetActive()/CheckVisPaths(), passes it where a
// `wxString const&` is expected - the ICF-folded-type pattern described in
// NOTES.md) - the empty "no password" string savegame loading tries after
// its own SAVEGAMEPWD30 attempt.
extern wxString passwd;

// Set by TGameController::ControllerAxisMouseMove/ControllerAxisCharacterMove
// (TGameController.cpp) for the game loop to consume as per-frame cursor /
// character movement deltas.
extern int movex;
extern int movey;
extern int charmovex;
extern int charmovey;
extern int stopped_char;

// Confirmed real, named globals (not anonymous dword_XXX symbols) used by
// TGameControl::MoveScene's scroll-easing physics, Deponia_Linux.asm lines
// 459288-460538+ - eased horizontal/vertical scroll speed, their
// distance-clamped maximums, and the fixed ease factor (confirmed 0.1,
// matching TMasterControl::ScrollUpdate's own already-approximated
// kEaseFactor). Nothing writes startspeed anywhere reversed so far, despite
// it being a real mutable global rather than a true constant in the binary.
extern float xspeed;
extern float yspeed;
extern float speedDownX;
extern float speedDownY;
extern float startspeed;

// Confirmed set/restored around each TGText::Draw() call (TGameControl::
// DisplayTexts, Deponia_Linux.asm lines 455890-456037) - presumably gates
// whether text rendering pushes its own transform matrix; real meaning not
// resolved.
extern bool matricesActive;

// Confirmed a real, named global (TGameControl::PreLoad, Deponia_Linux.asm
// lines 464021/464281/464473) - the password passed to
// TComposedFileManager's container-init calls. It begins as the engine's own
// password (the static initialization in main, asm 500255) and is replaced by
// the Password of config.ini or the `-p` option of the command line.
extern wxString passw;

// Confirmed recovered globals (asm 5295755, 5295702, 5295736), set by Init() and read by CreateWindowGL() and main():
// the flags of the window of the player (the SDL window flags: 1 full screen, 2 OpenGL, 4 shown, 0x20 resizable, 0x2000
// high DPI; 0x2004 to begin with), the scene `-sc` asks to start in, and the title of the window ("Visionaire Player",
// else the name of the game).
extern unsigned int Vflags;
extern wxString FirstSceneName;
extern wxString VSPlayerTitle;

// Confirmed a real, named global (TGameControl::HandleMouseMove,
// Deponia_Linux.asm line 472208) - gates whether the hovered-interface-object
// set is rebuilt every call when it's already empty. The scripts set it with
// `system.pauseEngineUpdate = true` (see luaSystem.cpp).
extern bool EngineUpdatePaused;

// Confirmed a real, named global, distinct from EngineUpdatePaused above
// (TGameControl::Update, Deponia_Linux.asm line 470381) - specifically gates
// the per-frame "mainLoop" Lua handler dispatch. The scripts set it with
// `system.pauseMainLoops = true` (see luaSystem.cpp).
extern bool MainLoopsPaused;

// Confirmed real, named globals (Deponia_Linux.asm lines 5250598, 5251151, 5294355, 5294362; read and written by
// the `system` object of the scripts, luaSystem.cpp): whether the game pauses when the window loses the focus (the
// script sets it with `system.pauseOnFocusLost`), whether a movie may be paused (`system.moviePauseAllowed`), the
// frames per second and the time the last frame took in milliseconds (`system.frameTime`, `system.lastFrameTime`).
extern bool CanLoseFocus;
extern bool g_bMoviePauseAllowed;

// Confirmed a real, named global (asm: g_loadingScreenLock): the loading screen draws from a thread of its own while the
// game loads; whoever else changes the matrices of the graphics at that time holds this (TMovie::Finish()).
extern wxCriticalSection g_loadingScreenLock;
extern float fps;
extern int lastFrameTime;

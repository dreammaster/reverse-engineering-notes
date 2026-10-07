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

extern int AppStatus;
extern int isProgramLooping;
extern int eMouseMessage;
extern unsigned char byte_11F8B01;
extern unsigned char byte_11F8B02;

extern TMasterControl *g_pGameControl;
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
// TComposedFileManager's container-init calls. Never seen written anywhere
// reversed so far; presumably set during earlier static/game
// initialization, like g_pGameControl itself.
extern wxString passw;

// Confirmed a real, named global (TGameControl::HandleMouseMove,
// Deponia_Linux.asm line 472208) - gates whether the hovered-interface-object
// set is rebuilt every call when it's already empty. Never seen written
// anywhere reversed so far.
extern bool EngineUpdatePaused;

// Confirmed a real, named global, distinct from EngineUpdatePaused above
// (TGameControl::Update, Deponia_Linux.asm line 470381) - specifically gates
// the per-frame "mainLoop" Lua handler dispatch; never seen written anywhere
// reversed so far.
extern bool MainLoopsPaused;

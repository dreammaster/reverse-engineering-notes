#include "AppGlobals.h"

#include "vsplayer/control/gameControl.h"

SDL_Window *VSPlayerWindow = nullptr;
SDL_GLContext VSPlayerContext = nullptr;

wxString strAppName;
wxSize surfaceSize;
wxSize renderSize;
bool g_unlockAspect = false;

TStandardPaths standardPaths;

wxFileName g_logfile;

std::FILE *LogFile = nullptr;

int AppStatus = 0;
int isProgramLooping = 0;
int eMouseMessage = 0;
unsigned char byte_11F8B01 = 0;
unsigned char byte_11F8B02 = 0;

// The real binary sets this up during earlier static/game initialization;
// for the stub build we just give main() a live object to call through.
// TMasterControl is abstract; TGameControl (98 methods, not yet
// reconstructed - see vsplayer/control/gameControl.h) is the real concrete
// class the binary instantiates here.
TMasterControl *g_pGameControl = new TGameControl();

int movex = 0;
int movey = 0;
int charmovex = 0;
int charmovey = 0;
int stopped_char = 0;

float xspeed = 0.0f;
float yspeed = 0.0f;
float speedDownX = 0.0f;
float speedDownY = 0.0f;
float startspeed = 0.1f;

bool matricesActive = false;

wxString passw;

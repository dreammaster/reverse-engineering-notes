#include "AppGlobals.h"

#include "TMasterControl.h"

SDL_Window* VSPlayerWindow = nullptr;
SDL_GLContext VSPlayerContext = nullptr;

wxString strAppName;
wxSize surfaceSize;
wxSize renderSize;

TStandardPaths standardPaths;

wxFileName g_logfile;

std::FILE* LogFile = nullptr;

int AppStatus = 0;
int isProgramLooping = 0;
int eMouseMessage = 0;
unsigned char byte_11F8B01 = 0;
unsigned char byte_11F8B02 = 0;

// The real binary sets this up during earlier static/game initialization;
// for the stub build we just give main() a live object to call through.
TMasterControl* g_pGameControl = new TMasterControl();

int movex = 0;
int movey = 0;
int charmovex = 0;
int charmovey = 0;
int stopped_char = 0;

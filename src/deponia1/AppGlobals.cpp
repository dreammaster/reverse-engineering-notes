#include "AppGlobals.h"

#include "THGameControl.h"
#include "vsplayer/control/gameControl.h"

SDL_Window *VSPlayerWindow = nullptr;
SDL_GLContext VSPlayerContext = nullptr;

wxString strAppName;
wxSize surfaceSize;
wxSize renderSize;
bool g_unlockAspect = false;
const wchar_t *g_loadingState = L"Loading";
wxString passwd;

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
TMasterControl *g_pGameControl = new THGameControl();
int GameMinDownTime = 0;
int g_traceFlags = 0;

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

std::vector<float> matrix1;
std::vector<float> matrix2;
std::vector<float> textMatrix;
std::vector<float> invMatrix1;
std::vector<int> fontShaderIndizes;
int fontShader = 0;
std::vector<std::string> luaDrawBeforeScene;
std::vector<std::string> luaDrawAfterScene;
std::vector<std::string> luaDrawAfterInterfaces;
float b2xoffset = 0.0f;
float b2yoffset = 0.0f;

wxString passw;

bool EngineUpdatePaused = false;

bool MainLoopsPaused = false;
wxPoint mousePos;
int numFingers = 0;
int bLeftButtonPressed = 0;
unsigned int lastMultigestureTicks = 0xFFFFFFFFu;
bool CanLoseFocus = true;
bool g_bMoviePauseAllowed = true;
float fps = 0.0f;
int lastFrameTime = 0;

wxRect g_displayedArea;

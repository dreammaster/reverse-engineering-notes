#include "AppGlobals.h"

#include "THGameControl.h"
#include "graphicslib/vector3d.h"
#include "vsplayer/control/gameControl.h"

SDL_Window *VSPlayerWindow = nullptr;
SDL_GLContext VSPlayerContext = nullptr;

wxString strAppName;
// 1280 x 720 to begin with (the static initialization in main, asm 500281-500290).
wxSize surfaceSize = {0x500, 0x2D0};
wxSize renderSize = {0x500, 0x2D0};
bool g_unlockAspect = false;
const wchar_t *g_loadingState = L"Loading";
wxString passwd(L"SAVEGAMEPWD30");

TStandardPaths standardPaths;

wxFileName g_logfile;

std::FILE *LogFile = nullptr;

int AppStatus = 0;
int isProgramLooping = 0;
int eMouseMessage = 0;
unsigned char byte_11F8B01 = 0;
unsigned char byte_11F8B02 = 0;

// Made by Init() (THGameControl) and deleted by CleanUp().
TMasterControl *g_pGameControl = nullptr;
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

wxString passw("ykgT6QsrRNv9XR");
unsigned int Vflags = 0;
wxString FirstSceneName;
wxString VSPlayerTitle(L"Visionaire Player");

bool EngineUpdatePaused = false;

bool MainLoopsPaused = false;
wxPoint mousePos;
int numFingers = 0;
int bLeftButtonPressed = 0;
unsigned int lastMultigestureTicks = 0xFFFFFFFFu;
bool CanLoseFocus = true;
bool g_bMoviePauseAllowed = true;
wxCriticalSection g_loadingScreenLock;
unsigned int lastFrameEnd = 0;
float fps = 0.0f;
int lastFrameTime = 0;

wxRect g_displayedArea;

bool hasInverseMatrix() {
	return invMatrix1.size() == 9;
}

static idVec3 transformThrough(const std::vector<float> &numbers, const wxPoint &point) {
	idMat3 matrix;

	for (int i = 0; i < 9; i++)
		matrix._m[i] = numbers[i];

	idVec3 vector;
	vector.x = (float)point.x;
	vector.y = (float)point.y;
	vector.z = 1.0f;

	return matrix * vector;
}

wxPoint transformByInverseMatrix(const wxPoint &point) {
	idVec3 result = transformThrough(invMatrix1, point);
	wxPoint transformed;

	transformed.x = (int)result.x;
	transformed.y = (int)result.y;
	return transformed;
}

bool hasMatrix() {
	return matrix1.size() == 9;
}

idVec3 transformByMatrix(const wxPoint &point) {
	return transformThrough(matrix1, point);
}

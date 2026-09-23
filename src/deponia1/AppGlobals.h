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

extern SDL_Window* VSPlayerWindow;
extern SDL_GLContext VSPlayerContext;

extern wxString strAppName;
extern wxSize surfaceSize;
extern wxSize renderSize;

extern TStandardPaths standardPaths;

// wxFileName, not wxString: main() calls GetFullPath() on it, which is a
// wxFileName member (see NOTES.md for how the ICF-folded symbol names
// obscured this).
extern wxFileName g_logfile;

extern std::FILE* LogFile;

extern int AppStatus;
extern int isProgramLooping;
extern int eMouseMessage;
extern unsigned char byte_11F8B01;
extern unsigned char byte_11F8B02;

extern TMasterControl* g_pGameControl;

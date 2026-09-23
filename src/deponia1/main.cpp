// Reconstructed from Deponia_Linux.asm, main proc at 0x62E450 (asm lines
// 499508-500213). See NOTES.md for the reverse-engineering notes behind the
// non-obvious choices below (control flow, g_logfile's real type, the
// retry-load lambda, string literals).
//
// Control flow has been restructured from the raw jump graph into ordinary
// if/return statements; the original's many duplicated
// wxCmdLineParser::~wxCmdLineParser() calls at every exit path are an
// artifact of -O2 inlining destructors at each return site instead of
// sharing one epilogue, so plain RAII (cmdLineParser going out of scope)
// reproduces the same behavior here.
//
// int __cdecl main(int argc, const char **argv, const char **envp)

#include <cstdlib>
#include <string>

#include "AppFunctions.h"
#include "AppGlobals.h"
#include "SdlStub.h"
#include "TComposedFile.h"
#include "TGameController.h"
#include "TMasterControl.h"
#include "TPictureIO.h"
#include "WxStub.h"

int main(int argc, char** argv, char** /*envp*/) {
    if (VSPlayerWindow != nullptr) {
        CleanUp(false);
    }

    // main::{lambda(void)#1} in the disassembly; its _M_invoke thunk is a
    // plain jmp into TPictureIO::RetryFailedPicturesLoad.
    TComposedFile::onRetryLoad = [] { TPictureIO::RetryFailedPicturesLoad(); };

    wxInitialize();

    // Default log file: "<LogFileDir>/messages.log".
    {
        wxFileName fn(standardPaths.GetLogFileDir() + L"/messages.log");
        g_logfile = fn;
    }
    LogFile = std::fopen(static_cast<const char*>(g_logfile.GetFullPath().mb_str()), "w");
    wxLog::SetVerbose(true);
    wxLog::SetActiveTarget(new wxLogStderr(LogFile));

    wxCmdLineParser cmdLineParser;
    if (!ParseCommandLine(argc, argv, cmdLineParser)) {
        return -1;
    }

    if (argc > 1) {
        wxString value;
        if (cmdLineParser.Found(wxString(L"l"), &value)) {
            // -l <path>: caller-specified log file overrides the default.
            wxFileName fn(value.ToStdWstring());
            g_logfile = fn;
            LogFile = std::fopen(static_cast<const char*>(g_logfile.GetFullPath().mb_str()), "w");
            wxLog::SetVerbose(true);
            wxLog::SetActiveTarget(new wxLogStderr(LogFile));
        }
    }

    // Both the "-l" and "no -l" paths converge here in the original.
    wxLog::SetVerbose(true);
    wxLog::SetActiveTarget(new wxLogStderr(LogFile));
    if (argc > 0) {
        strAppName = wxConvertMB2WX(argv[0]);
    }

    VSPlayerWindow = nullptr;
    VSPlayerContext = nullptr;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) < 0) {
        if (wxLog::loglevel >= 0) {
            wxLog::logexpanded(L"Unable to open SDL: %s", SDL_GetError());
        }
        ShowMessageBox(wxString(L"SDL "), wxString(L"Couldn't init SDL. App will quit now."));
        return -1;
    }

    SDL_ShowCursor(SDL_DISABLE);
    SDL_EventState(SDL_TEXTINPUT, SDL_ENABLE);
    std::atexit(SDL_Quit);

    if (!Init(strAppName, surfaceSize, renderSize, argc, argv, cmdLineParser)) {
        if (wxLog::loglevel >= 0) {
            wxLog::logexpanded(L"Init failed, could not load game");
        }
        ShowMessageBox(wxString(L"Init failed"),
                        wxString(L"Init failed, could not load game.\n"
                                 L"There is no game file or the game file is corrupted.\n"
                                 L"For more info view messages.log."));
        CleanUp(false);
        return -1;
    }

    AppStatus = 1;
    byte_11F8B02 = 1;
    byte_11F8B01 = 1;
    eMouseMessage = 0;
    isProgramLooping = 1;

    std::wstring gcdbPath = standardPaths.GetConfigDir() + L"gamecontrollerdb.txt";
    if (wxFile::Exists(wxString(gcdbPath))) {
        SDL_RWops* rw = SDL_RWFromFile(static_cast<const char*>(wxString(gcdbPath).mb_str()), "rb");
        SDL_GameControllerAddMappingsFromRW(rw, 1);
        if (wxLog::loglevel > 1) {
            wxLog::logexpanded(L"Added Controller mappings from File %s", gcdbPath.c_str());
        }
    }

    for (int i = 0; i < SDL_NumJoysticks(); ++i) {
        g_pGameControl->GetGameController()->AddGameController(i);
    }

    while (isProgramLooping) {
        ShowFrame(nullptr);
    }

    return 0;
}

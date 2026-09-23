#include "AppFunctions.h"

#include <cstdio>
#include <cwchar>

#include "AppGlobals.h"

void CleanUp(bool forceExit) {
    std::printf("[stub] CleanUp(%s)\n", forceExit ? "true" : "false");
}

bool ParseCommandLine(int /*argc*/, char** /*argv*/, wxCmdLineParser& /*parser*/) {
    // Stub: pretend the command line always parses successfully.
    return true;
}

bool Init(const wxString& /*appName*/, wxSize& /*surfaceSize*/, wxSize& /*renderSize*/, int /*argc*/,
          char** /*argv*/, wxCmdLineParser& /*parser*/) {
    // Stub: pretend startup always succeeds so the reconstructed main() can
    // be smoke-tested end to end.
    return true;
}

void ShowMessageBox(const wxString& title, const wxString& message) {
    std::fwprintf(stderr, L"[%ls] %ls\n", title.wc_str(), message.wc_str());
}

void ShowFrame(void* /*userData*/) {
    // Stub: stop the "game loop" after one iteration so a smoke-test run of
    // main() actually terminates instead of spinning forever.
    isProgramLooping = 0;
}

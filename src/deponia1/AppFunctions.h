#pragma once

#include "WxStub.h"

void CleanUp(bool forceExit);
bool ParseCommandLine(int argc, char** argv, wxCmdLineParser& parser);
bool Init(const wxString& appName, wxSize& surfaceSize, wxSize& renderSize, int argc, char** argv,
          wxCmdLineParser& parser);
void ShowMessageBox(const wxString& title, const wxString& message);
void ShowFrame(void* userData);

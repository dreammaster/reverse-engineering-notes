#include "WxStub.h"

#include <cstdarg>
#include <cstdio>
#include <sys/stat.h>

int wxLog::loglevel = 0;

void wxLog::SetVerbose(bool /*verbose*/) {
}

void wxLog::SetActiveTarget(wxLogStderr* /*target*/) {
    // Real wxWidgets takes ownership of the previous target and deletes it;
    // the stub intentionally does nothing with the pointer.
}

void wxLog::logexpanded(const wchar_t* fmt, ...) {
    va_list args;
    va_start(args, fmt);
    std::vfwprintf(stderr, fmt, args);
    va_end(args);
    std::fputws(L"\n", stderr);
}

bool wxInitialize() {
    return true;
}

wxString wxConvertMB2WX(const char* s) {
    return wxString(s);
}

bool wxFile::Exists(const wxString& path) {
    struct stat st;
    return ::stat(static_cast<const char*>(path.mb_str()), &st) == 0;
}

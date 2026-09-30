#include "Diagnostics.h"

#include <cstdio>

void x_assert(bool condition, const char *expression, const char *file, int line) {
	if (!condition) {
		std::fprintf(stderr, "assertion failed: %s (%s:%d)\n", expression, file, line);
	}
}

void TDiagnostic::BeginFixedRegion(wxString /*name*/) {
}

void TDiagnostic::EndFixedRegion() {
}

void TCPDebuggerClient::BeginArea(ProfileArea /*area*/, const std::string &/*name*/, int /*frame*/) {
}

void TCPDebuggerClient::EndArea(ProfileArea /*area*/, int /*frame*/) {
}

TCPDebuggerClient debugger;

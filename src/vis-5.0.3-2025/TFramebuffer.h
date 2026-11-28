// Not yet assert-confirmed to a specific file; stays at the top level.
#pragma once

class TFramebuffer {
public:
	TFramebuffer() = default;

	int width = 0;   // +0x08 (read by TPictureIO::CreateFromFramebuffer())
	int height = 0;  // +0x0C
};

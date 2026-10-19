// Minimal PNG writer (stored deflate blocks, no compression).
#pragma once
#include "common.h"

namespace shannara {

struct Image {
	int w = 0, h = 0;
	Bytes rgb; // w*h*3, 8 bits per component
};

void writePng(const std::string &path, const Image &img);

} // namespace shannara

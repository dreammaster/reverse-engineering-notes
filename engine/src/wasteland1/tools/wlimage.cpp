// wlimage <WL.EXE> <WLA.BIN> <out.bin>: unpacks the EXEPACKed WL.EXE load module (segment 1000h) and overlays WLA.BIN at
// offset 0, which is what the program does itself at start-up. The result is the image tools/recomp/recomp.py translates.
#include "wasteland1/exe.h"
#include "shared/bytes.h"
#include <cstdio>
#include <cstring>

int main(int argc, char **argv) {
	if (argc < 4) { std::fprintf(stderr, "usage: wlimage <WL.EXE> <WLA.BIN> <out.bin>\n"); return 1; }
	try {
		wl::ExeImage img = wl::loadExePack(shared::readFile(argv[1]), 0x1000);
		const shared::Bytes wla = shared::readFile(argv[2]);
		if (wla.size() > img.image.size()) { std::fprintf(stderr, "WLA.BIN is larger than the image\n"); return 1; }
		std::memcpy(img.image.data(), wla.data(), wla.size());
		FILE *f = std::fopen(argv[3], "wb");
		if (!f) { std::fprintf(stderr, "cannot write %s\n", argv[3]); return 1; }
		std::fwrite(img.image.data(), 1, img.image.size(), f);
		std::fclose(f);
		std::printf("%zu bytes\n", img.image.size());
	} catch (const std::exception &e) {
		std::fprintf(stderr, "error: %s\n", e.what());
		return 1;
	}
	return 0;
}

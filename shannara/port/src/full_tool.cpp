// full_tool EXE GAMEDIR PICDIR SAVEDIR [BUDGET_MILLIONS] [OUT.png]
// Runs the whole recompiled program (port/gen_full) on the host layer headlessly until it exits or the instruction budget is used up, then writes the
// 640x480 frame store with the VGA DAC palette to OUT.png and prints what the host had to answer for.
#include "host.h"
#include "png.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace shannara;

int main(int argc, char **argv) {
	setvbuf(stderr, nullptr, _IOLBF, 0);
	if (argc < 5) {
		fprintf(stderr, "usage: full_tool EXE GAMEDIR PICDIR SAVEDIR [BUDGET_MILLIONS] [OUT.png]\n");
		return 2;
	}
	try {
		Host h(argv[1], argv[2], argv[3], argv[4]);
		h.verbose = getenv("HOST_VERBOSE") != nullptr;
		h.loadNames("reversing/analysis/retail_function_map.tsv");
		if (getenv("HOST_WATCH")) // HOST_WATCH=lo,hi (hex)
			h.watch((uint32_t)strtoul(getenv("HOST_WATCH"), nullptr, 16), (uint32_t)strtoul(strchr(getenv("HOST_WATCH"), ',') + 1, nullptr, 16), 400, getenv("HOST_WATCH_FILTER") ? getenv("HOST_WATCH_FILTER") : "");
		if (getenv("HOST_CALLADDR")) // HOST_CALLADDR=hex[,count]
			h.traceAddress((uint32_t)strtoul(getenv("HOST_CALLADDR"), nullptr, 16), strchr(getenv("HOST_CALLADDR"), ',') ? atoi(strchr(getenv("HOST_CALLADDR"), ',') + 1) : 6);
		if (getenv("HOST_CALLNAME")) // HOST_CALLNAME=function_name[,count]
			h.traceAddress(h.addrOf(std::string(getenv("HOST_CALLNAME")).substr(0, std::string(getenv("HOST_CALLNAME")).find(','))), strchr(getenv("HOST_CALLNAME"), ',') ? atoi(strchr(getenv("HOST_CALLNAME"), ',') + 1) : 6);
		if (getenv("HOST_CALLTRACE"))
			h.traceCalls((uint64_t)atoll(getenv("HOST_CALLTRACE")), getenv("HOST_CALLCOUNT") ? (uint64_t)atoll(getenv("HOST_CALLCOUNT")) : 100);
		int64_t budget = (argc > 5 ? atoll(argv[5]) : 200) * 1000000LL;
		if (getenv("HOST_INSNTRACE"))
			h.machine().tracing = h.machine().traceRing = true;
		int rc = -2;
		std::string err;
		try {
			rc = h.run(budget);
		} catch (const std::exception &x) {
			err = x.what();
		}
		fprintf(stderr, "result: %s (rc %d)%s%s\n", h.budgetExhausted ? "instruction budget used up" : "program ended", rc, err.empty() ? "" : "; exception: ", err.c_str());
		if (!h.machine().trace.empty()) {
			fprintf(stderr, "call stack at the stop (outermost first):\n");
			for (uint32_t a : h.machine().trace)
				fprintf(stderr, "   %s\n", h.describe(a).c_str());
		}
		if (getenv("HOST_INSNTRACE")) {
			auto &tl = h.machine().traceLog;
			size_t from = tl.size() > 60 ? tl.size() - 60 : 0;
			fprintf(stderr, "last instructions (ea eax ecx edx ebx esi edi ebp esp):\n");
			for (size_t i = from; i < tl.size(); i++) {
				unsigned ea = 0;
				sscanf(tl[i].c_str(), "%x", &ea);
				fprintf(stderr, "   %s   %s\n", tl[i].c_str(), h.describe(ea).c_str());
			}
		}
		fprintf(stderr, "no-op shims called:");
		for (auto &kv : h.machine().unknownShimCalls)
			fprintf(stderr, " %s(%llu)", kv.first.c_str(), (unsigned long long)kv.second);
		fprintf(stderr, "\n");
		if (argc > 6) {
			h.flushVram();
			Image img;
			img.w = 640;
			img.h = 480;
			img.rgb.resize(640 * 480 * 3);
			const uint8_t *f = h.frame(), *p = h.palette();
			for (int i = 0; i < 640 * 480; i++)
				for (int c = 0; c < 3; c++) {
					uint8_t v = p[f[i] * 3 + c] & 63;
					img.rgb[i * 3 + c] = (uint8_t)(v * 4 + (v >> 4));
				}
			writePng(argv[6], img);
		}
	} catch (const std::exception &x) {
		fprintf(stderr, "error: %s\n", x.what());
		return 1;
	}
	return 0;
}

// engine_tool EXE GAMEDIR OPSFILE: replays "cmd VERB DOBJ IOBJ" lines on the transpiled engine (state carries over between lines) and prints
// "i result statehash [text]" - the format tests/engine_diff.py produces for the unicorn oracle.
#include "engine.h"

#include <cstdio>
#include <cstdlib>
#include <cstring>

using namespace shannara;

static std::string esc(const std::string &s) {
	std::string o;
	char b[8];
	for (unsigned char c : s) {
		if (c >= 0x20 && c < 0x7F && c != '\\')
			o.push_back((char)c);
		else if (c == '\n')
			o += "\\n";
		else {
			snprintf(b, sizeof b, "\\x%02x", c);
			o += b;
		}
	}
	return o;
}

int main(int argc, char **argv) {
	setvbuf(stdout, nullptr, _IOLBF, 0);
	if (argc < 4) {
		fprintf(stderr, "usage: engine_tool EXE GAMEDIR OPSFILE\n");
		return 2;
	}
	try {
		Engine e(argv[1], argv[2]);
		if (!strcmp(argv[3], "--peek") && argc >= 6) {
			uint32_t a = (uint32_t)strtoul(argv[4], nullptr, 0);
			int n = atoi(argv[5]);
			for (int k = 0; k < n; k++)
				printf("%02x", e.machine().r8(a + k));
			printf("\n");
			return 0;
		}
		if (getenv("SHAN_COVERAGE"))
			e.machine().enableCoverage();
		e.snapshot();
		FILE *evf = getenv("SHAN_EVENTS") ? fopen(getenv("SHAN_EVENTS"), "w") : nullptr; // engine-boundary event stream: "op name eax edx ebx ecx args" per no-op shim call
		e.machine().logEvents = evf != nullptr;
		int traceFrom = getenv("ENGINE_TRACE_FROM") ? atoi(getenv("ENGINE_TRACE_FROM")) : 0;
		FILE *f = fopen(argv[3], "r");
		if (!f)
			return 1;
		char op[32];
		int v, d, i, k = 0;
		while (fscanf(f, "%31s %d %d %d", op, &v, &d, &i) == 4) {
			std::string err;
			uint32_t r = 0;
			e.machine().tracing = getenv("ENGINE_TRACE_FILE") && k >= traceFrom;
			try {
				if (!strcmp(op, "reset")) {
					e.restore();
				} else if (!strcmp(op, "cmd")) {
					r = e.command(v, d, i);
				} else if (!strcmp(op, "flag")) { // flag N 0|1: set / clear game flag N
					uint32_t addr = 0xCD8AC + (uint32_t)((v - 1) >> 3);
					uint8_t bit = (uint8_t)(1 << ((v - 1) & 7));
					e.machine().w8(addr, d ? (e.machine().r8(addr) | bit) : (e.machine().r8(addr) & ~bit));
				} else if (!strcmp(op, "move")) { // move OBJ PARENT SLOT (object_move?)
					e.callFunction(0x1B470, (uint32_t)v, (uint32_t)d, (uint32_t)i);
				} else if (!strcmp(op, "goto")) { // goto ROOM (go_to_room?)
					r = e.callFunction(0x1EE10, (uint32_t)v);
				} else if (!strcmp(op, "call")) { // call ADDR EAX EDX: any game-layer function (overlay condition callbacks...)
						r = e.callFunction((uint32_t)v, (uint32_t)d, (uint32_t)i);
					} else if (!strcmp(op, "attr")) { // attr OBJ ATTR 0|1
					e.callFunction(i ? 0x1A1B0 : 0x1A1D8, (uint32_t)v, (uint32_t)d);
				}
			} catch (const std::exception &x) {
				err = std::string("EXC ") + x.what();
				if (getenv("ENGINE_TRACE")) {
					for (uint32_t a : e.machine().trace)
						err += " " + std::to_string(a);
					fprintf(stderr, "exception trace:");
					for (uint32_t a : e.machine().trace)
						fprintf(stderr, " %X", a);
					fprintf(stderr, "\n");
				}
				e.machine().trace.clear();
			}
			if (evf) {
				fprintf(evf, "%d @region %016llx\n", k, (unsigned long long)e.regionHash());
				for (auto &ev : e.machine().events)
					fprintf(evf, "%d %s\n", k, ev.c_str());
				e.machine().events.clear();
			}
			printf("%d %u %016llx [%s]\n", k++, r, (unsigned long long)e.stateHash(), esc(e.takeText() + err).c_str());
		}
		fclose(f);
		if (evf)
			fclose(evf);
		if (getenv("ENGINE_TRACE_FILE")) {
			FILE *t = fopen(getenv("ENGINE_TRACE_FILE"), "w");
			for (auto &l : e.machine().traceLog)
				fprintf(t, "%s\n", l.c_str());
			fclose(t);
		}
		if (getenv("SHAN_COVERAGE")) {
			FILE *c = fopen(getenv("SHAN_COVERAGE"), "w");
			for (size_t a = 0; a < e.machine().coverage.size(); a++)
				if (e.machine().coverage[a])
					fprintf(c, "%zx\n", a);
			fclose(c);
		}
		fprintf(stderr, "no-op shims called:");
		for (auto &kv : e.machine().unknownShimCalls)
			fprintf(stderr, " %s(%llu)", kv.first.c_str(), (unsigned long long)kv.second);
		fprintf(stderr, "\n");
	} catch (const std::exception &x) {
		fprintf(stderr, "error: %s\n", x.what());
		return 1;
	}
	return 0;
}

// The headless game engine: the transpiled game logic (port/gen, generated from the user's own SHANNARA.EXE) running on the Machine, with the
// UI / audio / library boundary provided by shims. It reproduces the original's responses to commands (text and state changes) and is
// differentially tested against the unicorn oracle (tests/engine_diff.py).
#pragma once
#include "machine.h"
#include "objectdat.h"
#include "shanstr.h"

namespace shannara {

void registerAllGenerated(Machine &m); // generated

class Engine {
public:
	Engine(const std::string &exePath, const std::string &gameDir);
	Machine &machine() { return *_m; }
	// run_command?(verb, dobj, iobj) as the UI issues it; `actor` is the acting character (Jak = 3). Returns the handler's result.
	uint32_t command(int verb, int dobj = 0, int iobj = 0, int actor = 3);
	// text printed since the last call ("\v" -> newline like the Python oracle's text())
	// direct calls of engine functions with register arguments (for state set-up and tests); returns eax
	uint32_t callFunction(uint32_t addr, uint32_t eax = 0, uint32_t edx = 0, uint32_t ebx = 0, uint32_t ecx = 0);
	std::string takeText();
	std::string rawOutput() const { return _out; }
	// hash of the saved state: every object record prefix and the 100-byte flag set
	uint64_t stateHash();
	void snapshot();
	void restore();

private:
	std::string format(const std::string &fmt, int argi);
	std::string message(uint32_t v);
	void installShims();

	LeExe *_exe = nullptr;
	Machine *_m = nullptr;
	ShanStr _str;
	std::string _out;
	std::vector<uint8_t> _snapMem;
	uint32_t _snapHeap = 0;
	size_t _snapOut = 0;
	uint32_t _clock = 0, _clockStep = 0; // see SHAN_CLOCK_STEP
};

} // namespace shannara

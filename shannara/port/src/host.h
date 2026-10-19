// Host layer for the full-program build: everything the original got from DOS/4GW, the DPMI host, the VESA BIOS, the mouse driver and the VGA ports.
// The recompiled program (port/gen_full, transpile_handlers.py --full) runs on a Machine; its `int N`, `in`/`out`, a few C-runtime internals and the hardware glue
// (AIL sound drivers, DPMI real-mode calls, setjmp) are implemented here. See analysis section 36.
#pragma once
#include "machine.h"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace shannara {

struct LongJump {
	uint32_t resume; // address to continue at (the instruction after the matching _setjmp_ call)
};

struct ProgramExit {
	int code;
};

class Host {
public:
	static const uint32_t kMemSize = 0x4000000;      // 64 MB flat image
	static const uint32_t kBase = 0x400000;          // the program image is loaded this far above its link-time addresses (transpile_handlers.py --reloc 0x400000)
	// "real-mode" (below 1 MB) memory for INT 21h AH=48h / DPMI 0100h: everything under the VGA window (the image lives above 4 MB, so conventional memory is free)
	static const uint32_t kDosArenaA = 0x1000, kDosArenaAEnd = 0xA0000;    // 636 KB
	static const uint32_t kDosArenaB = 0xA0000, kDosArenaBEnd = 0xA0000;   // (unused)
	static const uint32_t kHeapBase = 0x1000000;     // sbrk / DPMI memory blocks
	static const uint32_t kHeapEnd = 0x3C00000;
	static const uint32_t kVramWindow = 0xA0000;     // the 64 KB VESA window at its real linear address: the gx code addresses it with literal constants
	static const uint32_t kFrameStore = 0x3E00000;   // 1 MB: the whole 640x480 frame, bank by bank

	Host(const std::string &exePath, const std::string &gameDir, const std::string &picDir, const std::string &saveDir);
	Machine &machine() { return *_m; }

	// runs main_ (never returns normally); returns the exit code if the program calls exit, or when the instruction budget is used up (budgetExhausted set)
	int run(int64_t budget);
	bool budgetExhausted = false;

	// video: the frame as 8-bit indices plus the VGA DAC (6-bit components)
	int width() const { return _width; }
	int height() const { return _height; }
	const uint8_t *frame() const { return _m->ptr(kFrameStore); }
	const uint8_t *palette() const { return _dac; }
	void flushVram(); // writes the current VESA window back into the frame store
	std::string describe(uint32_t addr); // function containing/near addr (for diagnostics)
	void loadNames(const std::string &functionMapTsv);
	uint32_t addrOf(const std::string &name) const { auto it = _byName.find(name); return it == _byName.end() ? 0 : it->second; }
	void traceCalls(uint64_t first, uint64_t count); // print calls number first .. first+count with their register arguments
	void watch(uint32_t lo, uint32_t hi, int maxReports, const std::string &chainFilter = std::string());
	void traceAddress(uint32_t addr, int count); // print the first `count` calls to one address with registers and the first stack words // report who writes [lo, hi) (call chain and value)

	// input
	void setMouse(int x, int y, int buttons) { _mx = x; _my = y; _mb = buttons; }
	void pushKey(int ascii, int scan) { _keys.push_back((ascii & 0xFF) | ((scan & 0xFF) << 8)); }

	std::string logPath;       // when set, the host writes a line per interrupt / unhandled call
	bool verbose = false;

private:
	void installShims();
	void interrupt(int n);
	void dos21();
	void dpmi31();
	void int10();
	void mouse33();
	uint32_t portIn(uint16_t port, int bytes);
	void portOut(uint16_t port, uint32_t v, int bytes);
	void vbe(uint32_t rmcs);
	uint32_t dosAlloc(uint32_t bytes);
	uint32_t dosLargest() const; // bytes of the largest free real-mode block
	uint32_t heapAlloc(uint32_t bytes);
	std::string dosPath(uint32_t addr);
	std::string resolve(const std::string &dosName, bool forWrite);
	void log(const std::string &s);

	LeExe *_exe = nullptr;
	Machine *_m = nullptr;
	std::string _gameDir, _picDir, _saveDir;
	std::map<std::string, std::string> _dirIndex; // lowercase base name -> host path
	struct OpenFile { FILE *f = nullptr; std::string name; };
	std::vector<OpenFile> _files; // DOS handle = index + 5
	uint32_t _dosNextA = kDosArenaA, _dosNextB = kDosArenaB, _heapNext = kHeapBase, _brk = kHeapBase + 0x1000000, _dta = 0;
	std::map<uint32_t, uint32_t> _blocks; // DPMI block address -> size
	// video state
	int _width = 640, _height = 480, _bank = 0, _mode = 3;
	uint8_t _dac[768] = {};
	int _dacIndex = 0, _dacComp = 0, _dacRead = 0, _dacReadComp = 0;
	uint32_t _retrace = 0;
	// mouse
	int _mx = 0, _my = 0, _mb = 0;
	std::vector<int> _keys;
	// setjmp
	struct JmpBuf { uint32_t esp, ebp, ebx, ecx, esi, edi, resume; };
	std::map<uint32_t, JmpBuf> _jmp;
	std::map<uint32_t, std::string> _names; // function start -> name (diagnostics)
	std::map<std::string, uint32_t> _byName;
};

} // namespace shannara

// Host for the lifted Xanth game logic: a 1 MB memory image of the original executable's data, the lifted functions (gen/lifted_*.cpp) and the engine
// functions the original code calls but that are not game logic (screen, mouse, sound, files), implemented here.
#pragma once

#include <cstdint>
#include <cstdio>
#include <deque>
#include <initializer_list>
#include <map>
#include <string>
#include <vector>

#include "lifted_api.h"
#include "lifted_data.h"
#include "../legend_formats.h"

struct Event {
	std::string name;         // engine function called (original name)
	uint16_t w[8];            // its first eight stack words (arguments)
	std::string text;         // decoded text for message output
};

struct GameExit { int code; };         // thrown by DOS "terminate" (int 21h/4Ch)

// The PC under the lifted program (host_machine.cpp)
struct Machine {
	struct OpenFile { std::string name, path; FILE *fp = nullptr; size_t pos = 0; bool writable = false; };
	struct Block { uint16_t seg, paras; bool used; };
	struct MouseEvent { int x, y, buttons; };
	struct EmsMap { int handle, page; };

	std::string gameDir;
	std::map<std::string, std::string> gameFiles;          // lower-case name -> real name in the game folder
	std::map<std::string, std::vector<uint8_t>> vfs;       // files the program created (save games)
	std::map<int, OpenFile> files;
	int nextHandle = 5;
	std::string stdoutText;
	std::vector<std::string> openLog;                      // names of the files the program opened, in order
	std::vector<Block> blocks;
	std::map<int, std::vector<std::vector<uint8_t>>> ems;  // handle -> 16 KB pages
	EmsMap emsMap[4] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}};
	int nextEms = 1, emsUsed = 0;
	std::deque<MouseEvent> mouseScript;                    // successive mouse states returned by the driver (one per poll)
	std::deque<uint16_t> keys;                             // BIOS key codes (scan << 8 | ascii)
	int mouseX = 160, mouseY = 100, mouseButtons = 0, mouseVisible = 0;
	int pressCount[2] = {0, 0}, releaseCount[2] = {0, 0};
	uint8_t palette[768];                                  // 6-bit VGA DAC contents written through ports 3C8/3C9
	int dacRead = 0, dacWrite = 0, dacSub = 0;
	long paletteWrites = 0;
	uint32_t vectors[256];
	uint32_t dta = 0;
	unsigned clockCs = 0;
	uint16_t pitCount = 0;
	int pitFlip = 0;
	uint8_t picMask = 0, port61 = 0, retrace = 0;
	int videoMode = 3;

	void reset();
	void install(Cpu &c);
	bool inIsr = false;
	long irqCount = 0;
	void timerTick(Cpu &c);
	void scanGameDir();
	bool load(const std::string &name, std::vector<uint8_t> &out);
	int openFile(const std::string &name, bool create, bool writable);
	int readFile(int h, uint8_t *buf, int count);
	int writeFile(int h, const uint8_t *buf, int count);
	long seekFile(int h, long off, int whence);
	void closeFile(int h);
	void initArena();
	int allocBlock(int paras);
	int largestFree() const;
	bool freeBlock(int seg);
	bool resizeBlock(int seg, int paras);
	void emsSync(Cpu &c);
	void emsInt(Cpu &c);
	void mouseInt(Cpu &c);
	void pollMouse();
	void keyInt(Cpu &c);
	void dosInt(Cpu &c);
	void intr(Cpu &c, int number);
	uint16_t in(Cpu &c, uint16_t port, int bits);
	void out(Cpu &c, uint16_t port, uint16_t value, int bits);
};

struct Host {
	Cpu cpu;
	uint8_t mem[1 << 21];                             // 2 MB: the 1 MB real-mode space plus room for far pointers above it
	Machine machine;
	std::vector<uint8_t> image;                       // the pristine image (image.bin written by dump_image.py)
	Legend::MessageFile *msgs = nullptr;              // XANTHSTR.DAT
	std::vector<uint8_t> objectDat;                   // OBJECT.DAT (object name pool), loaded at 0x80740 like game_init does
	std::vector<Event> events;                        // everything the engine functions were asked to do
	std::vector<LongJmp> stray;                       // longjmp calls that found no setjmp frame (the call that made them returned 0)
	std::vector<std::string> problems;                // argument checks that failed
	std::vector<Event> calls;                         // optional trace of every call (name + arguments) when tracing is on
	bool tracing = false;
	void (*onText)(Host &) = nullptr;                  // called when the game prints text (tests use it to capture the screen at that moment)
	unsigned scratch = 0;
	std::string recent[6];                            // the last calls made (for diagnostics)
	unsigned recentPos = 0;

	std::map<uint32_t, void (*)(Cpu &)> liftedAt;     // linear address -> lifted function (also RTLink stubs)
	std::map<std::string, void (*)(Cpu &)> liftedByName, hostByName;
	std::map<uint32_t, std::pair<std::string, std::string>> nameAt;   // linear address -> (name, stub target)

	void init(const std::string &imagePath, const std::string &xanthstr);
	void reset();                                     // reload the image, registers, stack
	// call a lifted function by its original name; args[0] is the first C argument. Returns dx:ax.
	uint32_t call(const char *name, std::initializer_list<uint16_t> args, int retWords = 2);
	uint16_t w(uint16_t off) const { return cpu.rd16(SEG_dseg_para, off); }          // word in the data segment
	void setw(uint16_t off, uint16_t v) { cpu.wr16(SEG_dseg_para, off, v); }
	std::string str(uint16_t seg, uint16_t off, size_t max = 400, bool *ok = nullptr) const;   // NUL-terminated string at seg:off
	uint32_t putString(const std::string &s);          // copy into scratch memory, returns seg:off as (seg << 16 | off)
	std::string message(uint32_t id, bool *ok = nullptr) const;   // text of an 0xF0BBIIII message id
	void problem(const std::string &p) { if (problems.size() < 2000) problems.push_back(p); }
};

Host &host();
void callThroughHook(Cpu &c, uint16_t seg, uint16_t off);   // call a function by its far address as the machine would (used for interrupt service routines)
void hostUnimplemented(const char *name, Cpu &c);

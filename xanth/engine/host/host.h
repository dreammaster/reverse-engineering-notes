// Host for the lifted Xanth game logic: a 1 MB memory image of the original executable's data, the lifted functions (gen/lifted_*.cpp) and the engine
// functions the original code calls but that are not game logic (screen, mouse, sound, files), implemented here.
#pragma once

#include <cstdint>
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

struct Host {
	Cpu cpu;
	uint8_t mem[1 << 20];
	std::vector<uint8_t> image;                       // the pristine image (image.bin written by dump_image.py)
	Legend::MessageFile *msgs = nullptr;              // XANTHSTR.DAT
	std::vector<uint8_t> objectDat;                   // OBJECT.DAT (object name pool), loaded at 0x80740 like game_init does
	std::vector<Event> events;                        // everything the engine functions were asked to do
	std::vector<std::string> problems;                // argument checks that failed
	std::vector<Event> calls;                         // optional trace of every call (name + arguments) when tracing is on
	bool tracing = false;
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
void hostUnimplemented(const char *name, Cpu &c);

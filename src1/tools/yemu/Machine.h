/* The emulated PC for yemu: 1 MB of memory, the CPU, the EGA, and a high-level emulation of the DOS/BIOS services the
 * Chapter 1 programs use (INT 21h, 10h, 16h, 1Ah, 33h), including EXEC with handle inheritance and TSR exit, so the
 * original SW.EXE can be run end to end. Not part of the engine. */

#ifndef YEMU_MACHINE_H
#define YEMU_MACHINE_H

#include "Cpu86.h"
#include "Ega.h"

#include <deque>
#include <functional>
#include <map>
#include <stdio.h>
#include <string>
#include <vector>

namespace Yemu {

struct ScriptStep {
	uint64_t at;                 // absolute instruction count
	enum Kind { Key, MouseMove, MouseDown, MouseUp, Shot, Quit } kind;
	int a, b, c;
	std::string name;
};

class Machine : public Bus {
public:
	explicit Machine(const std::string &rootDir);
	~Machine();

	uint8_t mem[1 << 20];
	Cpu cpu;
	Ega ega;

	static const int kInstrPerSecond = 3000000;

	void setFonts(const std::vector<uint8_t> &f8, const std::vector<uint8_t> &f14);

	/** Load and run `program` (a DOS path under the root) as the top-level process until it exits. Returns its exit code. */
	int run(const std::string &program, uint64_t maxInstructions);

	// scripted input / capture
	void schedule(const ScriptStep &s) { _script.push_back(s); }
	std::function<void(const std::string &name)> onShot;
	/** Called for every INT 62h before it runs (AH in cpu.r[AX] >> 8). Return true if the hook ran the call itself. */
	std::function<bool(Machine &)> int62Hook;
	std::function<void(Machine &, int)> onProgramStart;     // (machine, psp)

	// state
	std::string console;                 // text printed with INT 21h
	std::string log;                     // diagnostics
	uint64_t maxInstr;
	bool quit;
	int currentPsp;
	std::vector<std::string> programTrace;   // programs started, in order
	bool watchOn = false;
	uint16_t watchCs = 0, watchIp = 0;   // dump the recent-instruction ring when this address is reached
	std::deque<uint32_t> recent;         // last executed CS:IP pairs (ring), for diagnostics
	std::string dumpRecent(size_t n) const;
	uint16_t ctxSegment;                 // the video driver's context segment (from INT 62h AH=0)

	/** Run an interrupt service routine to completion (used by compare mode and mouse callbacks). */
	void syncInterrupt(uint8_t n);

	// Bus
	uint8_t in8(uint16_t port);
	void out8(uint16_t port, uint8_t v);
	uint8_t vramRead(uint32_t offset) { return ega.read(offset); }
	void vramWrite(uint32_t offset, uint8_t v) { ega.write(offset, v); }
	bool softInt(Cpu &cpu, uint8_t n);
	void hleStub(Cpu &cpu, uint16_t offset);

private:
	struct Block { uint16_t start, size; uint16_t owner; bool used; };
	struct SftEntry { FILE *f; std::string path; int refs; bool console; };
	struct Process { uint16_t psp; std::vector<int> extraOpen; };

	std::string _root;
	std::vector<Block> _blocks;
	std::vector<SftEntry> _sft;
	std::vector<ScriptStep> _script;
	std::deque<uint8_t> _keys;
	uint16_t _dtaSeg, _dtaOff;
	uint8_t _lastExit, _lastExitType;
	bool _childDone, _skipIret;
	uint8_t _childExit, _childExitType;
	std::vector<Process> _procs;
	std::vector<std::string> _findList;
	size_t _findPos;
	uint8_t _findAttr;
	// mouse
	uint16_t _mouseSeg, _mouseOff, _mouseMask;
	int _mickX, _mickY, _mouseX, _mouseY;
	int _press[2];
	uint8_t _buttons;
	uint64_t _nextTick;
	size_t _scriptPos;
	std::map<std::string, int> _unhandled;
	std::vector<uint8_t> _font8, _font14;

	// memory manager
	uint16_t memAlloc(uint16_t paras, uint16_t owner, uint16_t *largest);
	bool memFree(uint16_t seg);
	bool memResize(uint16_t seg, uint16_t paras, uint16_t *maxAvail);
	uint16_t memLargest() const;
	void memFreeOwner(uint16_t owner);

	// process management
	bool loadProgram(const std::string &dosPath, uint16_t parentPsp, uint16_t tailSeg, uint16_t tailOff, uint16_t envSeg, bool isOverlayless, uint16_t *pspOut);
	int runChild(const std::string &dosPath, uint16_t paramSeg, uint16_t paramOff);
	void terminate(uint8_t code, uint8_t type, bool tsr, uint16_t keepParas);
	void runLoop(const std::function<bool()> &done);
	void pump();
	void deliverTick();
	void deliverStep(const ScriptStep &s);
	void callFar(uint16_t seg, uint16_t off, uint16_t ax, uint16_t bx, uint16_t cx, uint16_t dx);
	void iretFromStub();

	// DOS
	void int21();
	void int10();
	void int16h();
	void int1a();
	void int33();
	void unhandled(const char *what);
	std::string readDosString(uint16_t seg, uint16_t off);
	std::string hostPath(const std::string &dos) const;
	int jftLookup(int handle);
	int sftAlloc(FILE *f, const std::string &path, bool console);
	int jftAssign(int sftIndex, bool inherit);
	void sftRelease(int idx);
	void setCarry(bool c) { if (c) cpu.flags |= CF; else cpu.flags &= (uint16_t)~CF; }
	void dosError(uint16_t code) { cpu.r[AX] = code; setCarry(true); }
	uint16_t pspSeg() const { return (uint16_t)currentPsp; }
	uint32_t ticks() const;
};

} // namespace Yemu

#endif

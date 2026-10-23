/* Yendor1::Shared runtime: the routines every Chapter 1 program carries its own copy of (SW.EXE's file record API, tile cache,
 * input polling, string drawing, tick delays; fingerprints show the copies are instruction-identical, see docs1/program-graph.md).
 * The names follow the SW.EXE disassembly (sw.idb). Platform services (keys, time, speaker) come in through System. */

#ifndef YENDOR1_SHARED_RUNTIME_H
#define YENDOR1_SHARED_RUNTIME_H

#include "data/Tiles.h"
#include "shared/Blob.h"
#include "shared/Types.h"
#include "vid/Video.h"

#include <stdio.h>
#include <string>

namespace Yendor1 {
namespace Shared {

/** What the runtime needs from the platform (SDL host, ScummVM backend, or a scripted test double). */
class System {
public:
	virtual ~System() {}
	/** Let the platform process events (window, mouse, keyboard). Returns false to quit. */
	virtual bool pump() = 0;
	/** DOS INT 21h/06h style key byte, or -1. Extended keys arrive as 0 followed by the scan code. */
	virtual int pollKey() = 0;
	/** Discard any pending key presses (DOS INT 21h/AH=0Ch). */
	virtual void flushKeys() = 0;
	/** BIOS clock ticks (18.2 per second) since start. */
	virtual uint32 ticks() = 0;
	/** The DOS clock (INT 21h/2Ch DX): seconds in the high byte, hundredths in the low byte. The game's random generator seeds itself
	 *  from it. The default derives it from ticks(). */
	virtual uint16 dosClock() {
		const uint32 t = ticks();
		return (uint16)((((t * 10 / 182) % 60) << 8) | ((t * 1000 / 182) % 100));
	}
	/** Give the CPU back for a moment (busy-wait loops call this). */
	virtual void idle() = 0;
	/** PC speaker tone (frequency in Hz) or silence (0). */
	virtual void speaker(uint16 hz) = 0;
	/** SW.EXE's detectCpuClass result: 0x88 (8088), 0x186, 0x286 or 0x386. Selects the busy-wait counts programs use. */
	virtual uint16 cpuClass() = 0;
};

/** One of the game's data files: a flat array of fixed-size records (SW.EXE's file descriptor). */
class RecordFile {
public:
	RecordFile();
	~RecordFile();

	/** fileOpen: returns false (and counts an error) when the file cannot be opened. */
	bool open(const char *path, uint16 recordSize, bool writable);
	void close();
	bool isOpen() const { return _f != 0; }
	uint16 recordSize() const { return _recordSize; }

	// fileSeekRecord / fileReadRecord / fileWriteRecord: `record` is the 16-bit record number
	bool read(uint16 record, byte *buf);
	bool write(uint16 record, const byte *buf);
	/** Number of whole records in the file. */
	uint32 recordCount() const;

private:
	RecordFile(const RecordFile &);
	RecordFile &operator=(const RecordFile &);
	FILE *_f;
	uint16 _recordSize;
};

/** The result of one pollInput: what kind of event and the key/scan byte (SW.EXE: ctx+2Ch and byte ds:0EDFh). */
struct InputEvent {
	enum Source { None = 0, Key = 1, Extended = 2, LeftClick = 3, RightClick = 4 };
	byte source;
	byte code;      ///< ASCII (letters upper-cased), scan code for Extended, 0Dh for clicks
	InputEvent() : source(None), code(0) {}
};

class Runtime {
public:
	Runtime(Vid::Video &video, System &system);

	Vid::Video &video;
	System &system;

	/** The game directory (FILE0n.16 etc.). */
	bool openGameFiles(const char *gameDir);
	/** The directory given to openGameFiles (programs read their texts from the original executables there). */
	const std::string &dataDir() const { return _dataDir; }
	/** Where CURRGAME\ (the working copy of the party, NPC and area files) lives; the game directory unless the shell says otherwise. */
	const std::string &workDir() const { return _workDir.empty() ? _dataDir : _workDir; }
	void setWorkDir(const std::string &dir) { _workDir = dir; }
	Data::TileFile tiles[Data::kTileFileCount];

	// ---- input (pollInput, waitInput) ----
	/** Poll the keyboard and the mouse button flags once; clicks become Enter. Also refreshes the cursor. */
	InputEvent pollInput();
	/** Block until pollInput returns an event (or the platform asks to quit, then returns None). */
	InputEvent waitInput();
	/** The result of the most recent pollInput (the original's byte_12DE6 / ctx+2Ch pair). */
	InputEvent last;
	/** Write a command for the palette helper (VIDP16) into the driver's mailbox: waits until the previous one was taken. */
	void sendPaletteCommand(byte command);

	// ---- text (drawString, drawStringLines) ----
	/** Draw an ASCIZ string at ctx text cell (textCol, textRow) with the mouse hidden; the cell position is restored afterwards. */
	void drawString(const char *s);
	/** Draw `count` consecutive strings, one per text row, starting at the current cell. `strings` points at an array of char*. */
	void drawStringLines(const char *const *strings, int count);

	// ---- time ----
	/** delayTicks: wait `n` BIOS ticks. */
	void delayTicks(uint16 n);

	// ---- tiles (tileOp) ----
	/** tileOp AL=1: draw tile (file, index) at (ctx.x1, ctx.y1) on the current draw page. Returns false if the tile does not exist. */
	bool drawTile(byte file, uint16 index);
	/** Draw `count` consecutive tiles left to right, advancing ctx.x1 by 32 each (drawTileRun). */
	void drawTileRun(byte file, uint16 firstIndex, int count);

	// ---- driver shortcuts ----
	void vid(byte ah, byte al = 0) { video.call(ah, al); }

private:
	std::string _dataDir;
	std::string _workDir;
	Runtime(const Runtime &);
	Runtime &operator=(const Runtime &);
};

} // namespace Shared
} // namespace Yendor1

#endif

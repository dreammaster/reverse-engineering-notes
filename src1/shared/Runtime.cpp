#include "shared/Runtime.h"

#include <string.h>

namespace Yendor1 {
namespace Shared {

// ---------------------------------------------------------------------------------------------
// RecordFile
// ---------------------------------------------------------------------------------------------

RecordFile::RecordFile() : _f(0), _recordSize(0) {
}

RecordFile::~RecordFile() {
	close();
}

bool RecordFile::open(const char *path, uint16 recordSize, bool writable) {
	close();
	_f = fopen(path, writable ? "r+b" : "rb");
	_recordSize = recordSize;
	return _f != 0;
}

void RecordFile::close() {
	if (_f)
		fclose(_f);
	_f = 0;
}

bool RecordFile::read(uint16 record, byte *buf) {
	if (!_f || fseek(_f, (long)record * _recordSize, SEEK_SET) != 0)
		return false;
	return fread(buf, 1, _recordSize, _f) == _recordSize;
}

bool RecordFile::write(uint16 record, const byte *buf) {
	if (!_f || fseek(_f, (long)record * _recordSize, SEEK_SET) != 0)
		return false;
	const bool ok = fwrite(buf, 1, _recordSize, _f) == _recordSize;
	fflush(_f);
	return ok;
}

uint32 RecordFile::recordCount() const {
	if (!_f || !_recordSize)
		return 0;
	fseek(_f, 0, SEEK_END);
	const long n = ftell(_f);
	return n < 0 ? 0 : (uint32)n / _recordSize;
}

// ---------------------------------------------------------------------------------------------
// Runtime
// ---------------------------------------------------------------------------------------------

Runtime::Runtime(Vid::Video &v, System &s) : video(v), system(s) {
}

bool Runtime::openGameFiles(const char *gameDir) {
	_dataDir = gameDir;
	bool ok = true;
	for (int i = 0; i < Data::kTileFileCount; i++)
		ok = tiles[i].load(gameDir, i) && ok;
	return ok;
}

// pollInput (sub_1134E): keyboard first (INT 21h/06h), then the mouse button flags the driver's INT 33h callback sets.
InputEvent Runtime::pollInput() {
	InputEvent ev;
	Vid::Context &c = video.ctx;
	c.textChar = 0;
	const int k = system.pollKey();
	if (k > 0) {
		byte ch = (byte)k;
		if (ch >= 'a' && ch <= 'z')
			ch &= 0x5F;                         // letters are upper-cased
		c.textChar = ch;
		ev.source = InputEvent::Key;
		ev.code = ch;
	} else if (k == 0) {
		const int scan = system.pollKey();      // the extended key's scan code follows
		c.textChar = (byte)(scan < 0 ? 0 : scan);
		ev.source = InputEvent::Extended;
		ev.code = c.textChar;
	} else if (c.mousePresent != 0) {
		if (c.mouseFlags & 0x20) {
			c.textChar = 0x0D;
			ev.source = InputEvent::LeftClick;
			ev.code = 0x0D;
			c.mouseFlags &= 0x9F;
		} else if (c.mouseFlags & 0x02) {
			c.textChar = 0x0D;
			ev.source = InputEvent::RightClick;
			ev.code = 0x0D;
			c.mouseFlags &= 0xBD;
		}
	}
	if (c.mouseFlags & 0x80)
		vid(0x2B);                              // the pointer moved: redraw the cursor
	last = ev;
	return ev;
}

InputEvent Runtime::waitInput() {
	for (;;) {
		if (!system.pump()) {
			InputEvent none;
			return none;
		}
		const InputEvent ev = pollInput();
		if (ev.source != InputEvent::None)
			return ev;
		system.idle();
	}
}

// sendPaletteCommand: the helper clears the mailbox when it has taken the command (here it is taken at once)
void Runtime::sendPaletteCommand(byte command) {
	video.ctx.paletteCommand = command;
	video.ctx.paletteCommand = 0;
}

// drawString (sub_11678)
void Runtime::drawString(const char *s) {
	Vid::Context &c = video.ctx;
	const byte col = c.textCol, row = c.textRow;
	vid(0x29);                                  // hide the cursor while drawing
	for (; *s; s++) {
		c.textChar = (byte)*s;
		vid(0x08);
		c.textCol++;
	}
	vid(0x28);                                  // show it again
	c.textCol = col;
	c.textRow = row;
}

// drawStringLines (sub_114A4): each call draws one string and then moves to the next text row
void Runtime::drawStringLines(const char *const *strings, int count) {
	Vid::Context &c = video.ctx;
	for (int i = 0; i < count; i++) {
		drawString(strings[i]);
		c.textRow++;
	}
}

// delayTicks (sub_111F8)
void Runtime::delayTicks(uint16 n) {
	const uint32 start = system.ticks();
	while ((uint32)(system.ticks() - start) < n) {
		if (!system.pump())
			return;
		system.idle();
	}
}

// tileOp AL=1: draw a 32x32 tile with putImage (rows 32, 4 bytes wide, mask FFh). The caller's image pointer is preserved, and a
// pending mouse movement is flushed with vidMouseRefresh afterwards, as the original's tileOp epilogue does.
bool Runtime::drawTile(byte file, uint16 index) {
	byte batpic[Data::kTileBytes];
	const byte *raw = 0;
	if (file == kBatpicFile) {
		if (!readBatpic(index, batpic))
			return false;
		raw = batpic;
	} else if (file < Data::kTileFileCount) {
		raw = tiles[file].raw(index);
	}
	if (!raw)
		return false;
	Vid::Context &c = video.ctx;
	const Vid::ImagePtr saved = c.image;
	c.imgRows = 0x20;
	c.imgBytes = 4;
	c.imgMask = 0xFF;
	c.image.set(const_cast<byte *>(raw), Data::kTileBytes, 0);
	vid(0x0A);
	c.image = saved;
	if (c.mouseFlags & 0x80)
		vid(0x2B);
	return true;
}

// tile file 7 is CURRGAME/BATPIC.DAT: the character portraits the party strip code renders and stores (one 512-byte tile per record)
static std::string batpicPath(const std::string &dir) {
	std::string path = dir;
	if (!path.empty() && path[path.size() - 1] != '/' && path[path.size() - 1] != '\\')
		path += "/";
	return path + "CURRGAME/BATPIC.DAT";
}

bool Runtime::readBatpic(uint16 record, byte *tile512) {
	RecordFile f;
	return f.open(batpicPath(workDir()).c_str(), Data::kTileBytes, false) && f.read(record, tile512);
}

bool Runtime::writeBatpic(uint16 record, const byte *tile512) {
	RecordFile f;
	return f.open(batpicPath(workDir()).c_str(), Data::kTileBytes, true) && f.write(record, tile512);
}

void Runtime::drawTileRun(byte file, uint16 firstIndex, int count) {
	Vid::Context &c = video.ctx;
	for (int i = 0; i < count; i++) {
		drawTile(file, (uint16)(firstIndex + i));
		c.x1 = (uint16)(c.x1 + 0x20);
	}
}

} // namespace Shared
} // namespace Yendor1

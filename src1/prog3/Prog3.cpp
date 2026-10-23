#include "prog3/Prog3.h"

#include <stdint.h>
#include <string.h>

namespace Yendor1 {
namespace Prog3 {

namespace {

// PROG3.EXE's data segment starts 14F0h bytes into its load image
const uint32_t kDataBase = 0x14F0;
const uint16 kMenuText = 0x0F72;            // the 19 lines of the table of contents
const uint16 kHitBoxes = 0x11EB;            // 9 entries of x1, y1, x2, y2 (words)
const uint16 kLabelTable = 0x12BF;          // near pointers to the info panel labels, indexed by code - 0Ah
const uint16 kPanelBox = 0x146A;
const uint16 kPanelTitle1 = 0x1445, kPanelTitle2 = 0x1457;
const uint16 kBoxTall = 0x0E11, kBoxShort = 0x0E07;    // the message boxes
const uint16 kNoMoreNew = 0x0EDA, kNoPlayersView = 0x0EF6, kNoPlayersRemove = 0x0F53, kNoPlayersAssemble = 0x0F13, kNoPlayersReady = 0x0F34;
const int kEntries = 9;
const int kCloseCode = 0x20;                 // the info panel code of "EXIT?"

} // namespace

Prog3::Prog3(Shared::Runtime &rt, const Shared::Params &params, Shared::Launcher &launcher)
	: _rt(rt), _c(rt.video.ctx), _ui(rt), _params(params), _launcher(launcher), _haveExe(false), _entry(1), _quit(false) {
	memset(_party, 0, sizeof(_party));
	std::string path = rt.dataDir();
	if (!path.empty() && path[path.size() - 1] != '/' && path[path.size() - 1] != '\\')
		path += "/";
	path += "PROG3.EXE";
	_haveExe = _exe.load(path.c_str(), kDataBase);
}

const byte *Prog3::data(uint16 dsOffset, uint32_t length) const {
	return _haveExe ? _exe.bytesAt(dsOffset, length) : 0;
}

const char *Prog3::text(uint16 dsOffset) const {
	const char *s = _haveExe ? _exe.stringAt(dsOffset) : 0;
	return s ? s : "";
}

bool Prog3::readPartyHeader() {
	std::string path = _rt.workDir();
	if (!path.empty() && path[path.size() - 1] != '/' && path[path.size() - 1] != '\\')
		path += "/";
	path += "CURRGAME/PARTY.DAT";
	Shared::RecordFile f;
	if (!f.open(path.c_str(), 100, false))
		return false;
	return f.read(0, _party);
}

uint16 Prog3::headerWord(int offset) const {
	return (uint16)(_party[offset] | (_party[offset + 1] << 8));
}

// ---------------------------------------------------------------------------------------------
// the page
// ---------------------------------------------------------------------------------------------

// sub_1053F: the table of contents page
void Prog3::drawMenuPage() {
	_c.x1 = 0x30; _c.y1 = 0x0E; _c.x2 = 0x24F; _c.y2 = 0x12D;
	_c.colour = _c.colourMap[15];
	_rt.vid(0x07);
	_c.textFg = _c.colourMap[0];
	_c.textBg = _c.colourMap[15];
	_c.textRow = 2;
	_c.textCol = 7;
	const char *lines[19];
	uint32_t off = kMenuText;
	for (int i = 0; i < 19; i++) {
		lines[i] = text((uint16)off);
		off += (uint32_t)strlen(lines[i]) + 1;
	}
	_rt.drawStringLines(lines, 19);
	_c.x1 = 0xE8; _c.y1 = 0x20;
	_rt.drawTile(0, 0xC9);
	_ui.drawTopCorners();
	_ui.drawBottomCorners();
}

// shared_0bb58f: the frame round an entry, in the colour the caller left in the context; the pointer is hidden while the frame is drawn
// over it
void Prog3::highlight(byte entry, byte colour) {
	const byte *r = data((uint16)(kHitBoxes + (entry - 1) * 8), 8);
	if (!r)
		return;
	_ui.drawFrame(r, colour);
}

// sub_105BB: Up/Down move the frame, Enter accepts, a click on an entry selects it
void Prog3::chooseEntry() {
	highlight(_entry, _c.colourMap[7]);
	_rt.system.flushKeys();
	for (;;) {
		if (!_rt.system.pump()) {
			_quit = true;
			return;
		}
		const Shared::InputEvent ev = _rt.pollInput();
		if (ev.source == Shared::InputEvent::None) {
			_rt.system.idle();
			continue;
		}
		if (ev.source == Shared::InputEvent::LeftClick) {
			for (byte i = 1; i <= kEntries; i++) {
				const byte *r = data((uint16)(kHitBoxes + (i - 1) * 8), 8);
				if (!r)
					break;
				auto word = [r](int k) { return (int16_t)(r[k] | (r[k + 1] << 8)); };
				const int16_t x = (int16_t)_c.curX, y = (int16_t)_c.curY;
				if (x < word(0) || x > word(4) || y < word(2) || y > word(6))
					continue;
				highlight(_entry, _c.colourMap[15]);
				_entry = i;
				return;
			}
			continue;
		}
		if (_c.textChar == 0x50) {                    // Down
			highlight(_entry, _c.colourMap[15]);
			_entry = _entry == kEntries ? 1 : (byte)(_entry + 1);
			highlight(_entry, _c.colourMap[7]);
		} else if (_c.textChar == 0x48) {             // Up
			highlight(_entry, _c.colourMap[15]);
			_entry = _entry == 1 ? kEntries : (byte)(_entry - 1);
			highlight(_entry, _c.colourMap[7]);
		} else if (_c.textChar == 0x0D) {             // Enter
			highlight(_entry, _c.colourMap[15]);
			return;
		}
	}
}

// loc_10441: a boxed one-line message at text row `row`; any key or click closes it again
void Prog3::message(uint16 boxOffset, uint16 textOffset, byte row) {
	_c.textRow = row;
	_c.textCol = 0x1C;
	_c.textFg = _c.colourMap[11];
	_c.textBg = _c.colourMap[15];
	_rt.vid(0x29);
	_rt.vid(0x14);
	if (const byte *box = data(boxOffset, 10))
		_ui.drawBox(box);
	_rt.drawString(text(textOffset));
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
	for (;;) {
		if (!_rt.system.pump()) {
			_quit = true;
			break;
		}
		if (_rt.pollInput().source != Shared::InputEvent::None)
			break;
		_rt.system.idle();
	}
	_rt.vid(0x29);
	_rt.vid(0x12, 1);
	_rt.vid(0x1E);
	_rt.vid(0x28);
}

// the "Close the book" confirmation: Enter (or its icon) keeps the choice, Esc (or its icon) takes it back
void Prog3::confirmClose() {
	const byte *table = data((uint16)(kLabelTable + 2 * (kCloseCode - 0x0A)), 2);
	const uint16 labelOffset = table ? (uint16)(table[0] | (table[1] << 8)) : 0;
	if (const byte *box = data(kPanelBox, 10))
		_ui.openConfirmPanel(box, text(kPanelTitle1), text(kPanelTitle2), text(labelOffset), 0x42);
	const byte result = _ui.input(0x42, 0x42, 0);
	if (result == 0xFF)
		_quit = true;
	_rt.vid(0x29);
	_ui.closeInfoPanel();
	_rt.vid(0x14);
	_ui.drawBottomDecorations();
	_rt.vid(0x12, 0);
	_rt.vid(0x28);
}

// execProgram: the child gets its own copy of the parameter block; the video context's image pointer is the parent's again afterwards
int Prog3::runChild(int program) {
	Shared::Params child = _params;
	const Vid::ImagePtr saved = _c.image;
	const int code = _launcher.run(program, child, _rt);
	_c.image = saved;
	return code;
}

// ---------------------------------------------------------------------------------------------
// the program
// ---------------------------------------------------------------------------------------------

int Prog3::run() {
	_rt.vid(0x29);
	_rt.vid(0x2D, 0);                                 // pointer shape mode 0
	_entry = _params.parm(0x0D) == 2 ? 5 : 1;         // the game already started: "Open book" first
	_rt.vid(0x14);
	_ui.drawBottomDecorations();
	drawMenuPage();
	if (_params.parm(3) != 0) {
		_params.setParm(3, 0);
		if (_c.videoKind != 2) {
			// (clearTextScreen: the DOS text screen is not used in this port)
			_rt.sendPaletteCommand(4);
			_rt.vid(0x15);
		} else {
			_rt.sendPaletteCommand(4);
		}
	} else {
		_rt.vid(0x16);
		_rt.sendPaletteCommand(4);
	}
	_rt.vid(0x12, 0);
	_rt.vid(0x28);

	for (;;) {
		chooseEntry();
		if (_quit) {
			_entry = 9;
			break;
		}
		if (_entry >= 7) {
			if (_entry == 9) {
				if (_params.parm(0x0D) != 2)
					_entry = 1;
				confirmClose();
				if (_quit) {
					_entry = 9;
					break;
				}
			}
			if (_c.textChar == 0x0D)
				break;
			continue;
		}
		readPartyHeader();
		const uint16 count = headerWord(0);              // word_114FC: 0Ah means "no characters yet"
		int code = 0;
		switch (_entry) {
		case 1:                                          // Create
			if ((int16_t)count <= 0x1B) {
				_params.setParm(0x0E, 1);
				code = runChild(5);
				_params.setParm(0x0E, 0);
				_entry = 1;
			} else {
				_entry = 2;
				message(kBoxTall, kNoMoreNew, 8);
			}
			break;
		case 2:                                          // View
			if ((int16_t)count > 0x0A) {
				_params.setParm(2, 0);
				_params.setParm(0x0E, 2);
				code = runChild(5);
				_params.setParm(0x0E, 0);
				_params.setParm(0x10, 0);
				_entry = 2;
			} else {
				_entry = 1;
				message(kBoxTall, kNoPlayersView, 8);
			}
			break;
		case 3:                                          // Remove
			if ((int16_t)count > 0x0A) {
				_params.setParm(2, 0);
				_params.setParm(0x0E, 3);
				code = runChild(5);
				_params.setParm(0x0E, 0);
				_params.setParm(2, 0);
				_entry = 3;
			} else {
				_entry = 1;
				message(kBoxTall, kNoPlayersRemove, 8);
			}
			break;
		case 4:                                          // Assemble party members
			if (count != 0x0A) {
				_params.setParm(2, 0);
				_params.setParm(0x0E, 4);
				code = runChild(5);
				_params.setParm(0x0E, 0);
				_params.setParm(2, 0);
				_entry = 4;
			} else {
				_entry = 1;
				message(kBoxShort, kNoPlayersAssemble, 0x0D);
			}
			break;
		case 5:                                          // Open book (Play)
			if (count == 0x0A) {
				_entry = 1;
				message(kBoxShort, kNoPlayersReady, 0x0D);
			} else if (headerWord(0x26) == 0) {
				_entry = 4;
				message(kBoxShort, kNoPlayersReady, 0x0D);
			} else {
				_rt.vid(0x29);
				return _entry;
			}
			break;
		case 6:                                          // Bookmarker (Save/Restore)
			_params.setParm(2, 0);
			code = runChild(6);
			if (code == Shared::Launcher::kNotImplemented)
				return code;
			if (code == 2) {
				_entry = 1;
				return _entry;
			}
			_entry = 6;
			break;
		}
		if (code == Shared::Launcher::kNotImplemented)
			return code;
		if (_quit) {
			_entry = 9;
			break;
		}
	}
	return _entry;
}

} // namespace Prog3
} // namespace Yendor1

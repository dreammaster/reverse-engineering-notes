#include "sw/Shell.h"

#include "prog1/Prog1.h"
#include "prog2/Prog2.h"
#include "prog3/Prog3.h"

#include <stdio.h>
#include <string.h>

namespace Yendor1 {
namespace Sw {

namespace {

// SW.EXE's data segment starts 19A0h bytes into its load image
const uint32_t kSwDataBase = 0x19A0;

// DS offsets of the texts SW.EXE prints (the strings stay in the original executable)
const uint32_t kMessageA = 0x108E;        // 21 lines of the shareware message, shown at start-up
const uint32_t kMessageB = 0x14FC;        // 24 lines shown after the PROG16 cutscene

enum Program {
	kProg0 = 0, kProg1 = 1, kProg2 = 2, kProg3 = 3, kProg4 = 4, kProg5 = 5, kProg10 = 10, kProg16 = 16, kProg17 = 17, kProg18 = 18
};

std::string join(const std::string &dir, const char *name) {
	if (dir.empty())
		return name;
	const char last = dir[dir.size() - 1];
	return dir + ((last == '/' || last == '\\') ? "" : "/") + name;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// DefaultLauncher
// ---------------------------------------------------------------------------------------------

int DefaultLauncher::run(int program, Shared::Params &params, Shared::Runtime &rt) {
	switch (program) {
	case kProg0:
		return 0;               // the party/save-slot program is not reimplemented yet; "nothing to do" is its normal success
	case kProg1: {
		Prog1::Prog1 p(rt, params);
		return p.run();
	}
	case kProg2: {
		Prog2::Prog2 p(rt, params);
		return p.run();
	}
	case kProg3: {
		Prog3::Prog3 p(rt, params, *this);
		return p.run();
	}
	default:
		return kNotImplemented;
	}
}

// ---------------------------------------------------------------------------------------------
// Shell
// ---------------------------------------------------------------------------------------------

Shell::Shell(Shared::Runtime &rt, Launcher &launcher, const Config &config)
	: _rt(rt), _launcher(launcher), _config(config), _stopped(false) {
	memset(_header.raw, 0, sizeof(_header.raw));
}

bool Shell::loadHeader() {
	Shared::RecordFile f;
	if (!f.open(join(_config.workDir, "CURRGAME/PARTY.DAT").c_str(), 100, false))
		return false;
	return f.read(0, _header.raw);
}

bool Shell::saveHeader() {
	Shared::RecordFile f;
	if (!f.open(join(_config.workDir, "CURRGAME/PARTY.DAT").c_str(), 100, true))
		return false;
	return f.write(0, _header.raw);
}

// sub_10873: EXEC VID16.DRV + VIDP16.DRV, then fn00 and the image size defaults
bool Shell::startVideoDriver() {
	_rt.video.init(true);
	Vid::Context &c = _rt.video.ctx;
	c.imgRows = 0x20;
	c.imgBytes = 4;
	c.imgMask = 0xFF;
	return true;
}

// sub_10B08: the CPU class picks the busy-wait counts programs use (8088 is the slowest machine)
void Shell::calibrateDelays() {
	Vid::Context &c = _rt.video.ctx;
	c.drawPage = 0;
	c.cpuClass = _rt.system.cpuClass();
	c.delayStep = 0x1388; c.delayWipe = 0x0BB8; c.delayCpu = 0x05DC;
	if (c.cpuClass == 0x88)
		return;
	c.delayStep = 0x1F40; c.delayWipe = 0x1B58; c.delayCpu = 0x0FA0;
	if (c.cpuClass == 0x286)
		return;
	c.delayStep = 0x55F0; c.delayWipe = 0x2328; c.delayCpu = 0x189C;
}

// sub_105AF: open the data files (the tile files here) and clear both video pages
void Shell::openDataFilesAndClear() {
	_rt.openGameFiles(_config.dataDir.c_str());
	Vid::Context &c = _rt.video.ctx;
	_rt.sendPaletteCommand(5);
	c.y2 = 0x15D;
	c.y1 = 0; c.x1 = 0; c.x2 = 0x27F;
	c.colour = c.colourMap[0];
	c.drawPage = 1;
	_rt.vid(0x07);
	c.drawPage = 0;
	_rt.vid(0x07);
	if (_header.gameState() != 2)
		flushPartyAndRefresh();
}

// sub_1082E: BATPIC.DAT is emptied, the party members' pictures are redrawn, page 1 is shown
void Shell::flushPartyAndRefresh() {
	const std::string batpic = join(_config.workDir, "CURRGAME/BATPIC.DAT");
	if (FILE *f = fopen(batpic.c_str(), "rb")) {
		fclose(f);
		if (FILE *t = fopen(batpic.c_str(), "wb"))      // write 0 bytes: truncate
			fclose(t);
	}
	// up to six party member records are drawn with the party strip; an empty party has none (not yet ported)
	_rt.vid(0x1E);
}

// sub_10984 / sub_10A46: a text screen with the shareware message; Enter (or a click) dismisses it
void Shell::showSharewareMessage(bool afterPlay) {
	Vid::Context &c = _rt.video.ctx;
	c.x1 = 0; c.y1 = 0; c.x2 = 0x27F; c.y2 = 0x15D;
	if (c.videoKind == 2)
		c.y2 = 0x18F;
	c.colour = c.colourMap[0];
	_rt.vid(0x07);
	_rt.sendPaletteCommand(5);
	_rt.vid(0x12, 0);
	_rt.system.speaker((uint16)(1193180 / 0x1F4));
	if (!afterPlay) {
		c.textBg = c.colourMap[2];
		c.textFg = c.videoKind == 1 ? c.colourMap[3] : c.colourMap[11];
	} else {
		c.textFg = c.colourMap[2];
		c.textBg = c.videoKind == 1 ? c.colourMap[3] : c.colourMap[11];
	}
	_rt.delayTicks(3);
	_rt.system.speaker(0);
	c.textCol = 0x0F;
	c.textRow = 1;
	const int lines = afterPlay ? 0x18 : 0x15;
	uint32_t off = afterPlay ? kMessageB : kMessageA;
	for (int i = 0; i < lines; i++) {
		const char *s = _exe.stringAt(off);
		if (!s)
			break;
		_rt.drawString(s);
		c.textRow++;
		off += (uint32_t)strlen(s) + 1;
	}
	if (c.videoKind != 2) {
		_rt.sendPaletteCommand(1);
		_rt.waitInput();
		_rt.sendPaletteCommand(5);
	}
}

// sub_103AB: the closing "thank you" tile screen (not yet verified against a trace of the original)
void Shell::showEndScreen() {
	Vid::Context &c = _rt.video.ctx;
	_rt.vid(0x14);
	c.x1 = 0; c.y1 = 0; c.x2 = 0x27F; c.y2 = 0x15D;
	if (c.videoKind == 2)
		c.y2 = 0x18F;
	c.colour = 0;
	c.drawPage = 0;
	_rt.vid(0x07);
	if (_params.parm(0x0D) == 2) {
		_rt.sendPaletteCommand(0x0A);
		_rt.sendPaletteCommand(c.videoKind == 2 ? 4 : 5);
		return;
	}
	// the title lettering: two rows of FILE00 tiles
	c.x1 = 0x50; c.y1 = 0x87;
	_rt.drawTileRun(0, 0x1EB, 7);
	static const uint16 row1[5] = {0x1F2, 0x1EC, 0x1F0, 0x1F3, 0x1FF};
	c.x1 = 0x170;
	for (int i = 0; i < 5; i++) {
		_rt.drawTile(0, row1[i]);
		c.x1 = (uint16)(c.x1 + 0x20);
	}
	_rt.drawTileRun(0, 0x226, 2);
	c.x1 = 0xD0; c.y1 = 0xB7;
	static const uint16 row2[6] = {0x228, 0x1FF, 0x226};
	for (int i = 0; i < 3; i++) {
		_rt.drawTile(0, row2[i]);
		c.x1 = (uint16)(c.x1 + 0x20);
	}
	c.x1 = (uint16)(c.x1 + 0x20);
	static const uint16 row3[3] = {0x1F0, 0x1FF, 0x229};
	for (int i = 0; i < 3; i++) {
		_rt.drawTile(0, row3[i]);
		c.x1 = (uint16)(c.x1 + 0x20);
	}
	_rt.vid(0x17);
	_rt.sendPaletteCommand(0x0A);
	if (c.videoKind != 2)
		_rt.sendPaletteCommand(5);
	_rt.delayTicks(0x14);
}

int Shell::runProgram(int program) {
	_params.setParm(0, 0x0D);
	return _launcher.run(program, _params, _rt);
}

// sub_10571: the prologue; afterwards the game state becomes "prologue seen"
int Shell::runPrologue() {
	const int code = runProgram(kProg2);
	_params.setParm(3, 0);
	_params.setParm(4, 0);
	_header.setGameState(1);
	saveHeader();
	return code;
}

int Shell::endPath() {
	_rt.vid(0x29);
	showEndScreen();
	showSharewareMessage(false);
	return 0;
}

int Shell::run() {
	if (!_exe.load(join(_config.dataDir, "SW.EXE").c_str(), kSwDataBase)) {
		_message = "Unable to open one of the data files.";
		return 0xFF;
	}
	_rt.setWorkDir(_config.workDir);
	_params.setParm(0, 0x0D);
	_params.raw[0x58] = 0x34;
	_params.raw[0x59] = 0x12;
	if (!loadHeader()) {
		_message = "Please run the install program first.";
		return 0xFF;
	}
	if (_header.gameState() != 2) {
		// choose the party: PROG0 in mode 1
		_params.setParm(0x0E, 1);
		const int code = runProgram(kProg0);
		_params.setParm(0x0E, 0);
		if (code == 0x21) {
			_message = "Please run the install program first.";
			return 0xFF;
		}
		if (code == 0x20) {
			_message = "No party was selected.";
			return 0;
		}
		if (code != 0) {
			_message = "Problems with loading a party.";
			return code < 0 ? 0xFF : code;
		}
	}
	if (!loadHeader()) {
		_message = "Please run the install program first.";
		return 0xFF;
	}
	if (_header.videoKind() == 1) {
		startVideoDriver();
	} else {
		_message = _header.videoKind() == 2 ? "The required VESA driver is not loaded." : "Please run the install program first.";
		return 0xFF;
	}
	calibrateDelays();
	const byte state = _header.gameState();
	_params.setParm(0x0D, state);
	_params.setParm(3, state == 2 ? 3 : (state == 1 ? 1 : 0));
	_rt.vid(0x02);
	openDataFilesAndClear();
	showSharewareMessage(false);

	Vid::Context &c = _rt.video.ctx;
	c.x1 = 0; c.y1 = 0; c.x2 = 0x270; c.y2 = c.videoKind == 2 ? 0x180 : 0x14E;
	_rt.vid(0x2A);
	c.x1 = 0x1F4; c.y1 = 0x118;
	_rt.vid(0x2C);
	byte intro = _header.introFlag();
	if (intro != 0 && c.mousePresent == 0)
		intro = 0;
	_params.setParm(2, intro);

	int code = runProgram(kProg1);
	if (code == Launcher::kNotImplemented)
		return 0xFF;
	if (code == 1)
		return endPath();

	if (_header.gameState() == 2)
		return bookMenuLoop();
	if (_header.gameState() == 0) {
		code = runPrologue();
		if (code == Launcher::kNotImplemented)
			return 0xFF;
		return bookMenuLoop();
	}
	_params.setParm(3, 1);
	if (_header.resumeFlag() == 0)
		return bookMenuLoop();
	return gameLoop();
}

// loc_101B9: the book menu (PROG3) and what each of its exit codes starts
int Shell::bookMenuLoop() {
	for (;;) {
		const int code = runProgram(kProg3);
		if (code == Launcher::kNotImplemented)
			return 0xFF;
		switch (code) {
		case 9:
			_rt.vid(0x14);
			return endPath();
		case 8:
			_params.setParm(3, 1);
			_params.setParm(4, 1);
			if (runPrologue() == Launcher::kNotImplemented)
				return 0xFF;
			break;
		case 7:
			_params.setParm(3, 2);
			if (runProgram(kProg1) == Launcher::kNotImplemented)
				return 0xFF;
			_params.setParm(3, 0);
			break;
		case 5:
			_params.setParm(3, 0);
			return gameLoop();
		case 1:
			return endPath();
		default:
			_message = "Errors have been encountered.";
			return code;
		}
	}
}

// loc_1021E: the game (PROG4) and the programs it hands over to
int Shell::gameLoop() {
	for (;;) {
		const int code = runProgram(kProg4);
		if (code == Launcher::kNotImplemented)
			return 0xFF;
		_params.setParm(3, 0);
		switch (code) {
		case 0:
			return bookMenuLoop();
		case 1:
			return endPath();
		case 2:
			if (runProgram(kProg16) == Launcher::kNotImplemented)
				return 0xFF;
			showSharewareMessage(true);
			return 0;
		case 3: {
			const int sub = runProgram(kProg10);
			if (sub == Launcher::kNotImplemented)
				return 0xFF;
			if (sub != 1) {
				_rt.vid(0x14);
				_params.setParm(3, 0);
				break;
			}
		}
		// fall through: a PROG10 exit code of 1 saves and restores through PROG0, like exit code 4
		case 4: {
			loadHeader();
			_params.setParm(0x0E, 3);
			_params.setParm(0x10, _header.raw[0x21]);
			const int r = runProgram(kProg0);
			_params.setParm(0x0E, 0);
			_params.setParm(0x10, 0);
			if (r != 0)
				return 0;
			_rt.vid(0x14);
			flushPartyAndRefresh();
			_params.setParm(3, 0);
			break;
		}
		case 5:
			if (runProgram(kProg17) == Launcher::kNotImplemented)
				return 0xFF;
			showEndScreen();
			showSharewareMessage(false);
			return 0;
		case 6:
			if (runProgram(kProg18) == Launcher::kNotImplemented)
				return 0xFF;
			_rt.vid(0x14);
			_params.setParm(3, 0);
			break;
		default:
			return 0;
		}
	}
}

} // namespace Sw
} // namespace Yendor1

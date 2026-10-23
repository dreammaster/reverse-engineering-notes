/* Yendor1::Sw -- SW.EXE, the shell. It opens the game's files, starts the video driver, shows the shareware message, and then
 * runs the other programs in a loop, choosing the next one from each program's exit code (docs1/program-graph.md). Where the
 * original EXECs a program and reads its exit code, the shell calls a Launcher. */

#ifndef YENDOR1_SW_SHELL_H
#define YENDOR1_SW_SHELL_H

#include "data/ExeImage.h"
#include "shared/Launcher.h"
#include "shared/Params.h"
#include "shared/Runtime.h"

#include <string>

namespace Yendor1 {
namespace Sw {

using Shared::byte;
using Shared::uint16;

typedef Shared::Launcher Launcher;

/** The default launcher: the programs that have been reimplemented; every other one reports kNotImplemented. */
class DefaultLauncher : public Launcher {
public:
	int run(int program, Shared::Params &params, Shared::Runtime &rt);
};

struct Config {
	std::string dataDir;   ///< FILEnn.16 etc. and SW.EXE (the original game directory)
	std::string workDir;   ///< where CURRGAME\ and SAVEGAME.x\ live (may be the same directory)
};

/** The party file's header record (record 0 of CURRGAME\PARTY.DAT, 100 bytes): the fields SW.EXE uses. */
struct PartyHeader {
	byte raw[100];
	byte gameState() const { return raw[0x25]; }          ///< byte_11A4B: 0 new, 1 prologue seen, 2 party chosen
	void setGameState(byte v) { raw[0x25] = v; }
	uint16 resumeFlag() const { return (uint16)(raw[0x26] | (raw[0x27] << 8)); }   ///< word_11A4C
	byte videoKind() const { return raw[0x3F]; }          ///< byte_11A65: 1 = EGA, 2 = SVGA
	byte introFlag() const { return raw[0x1C]; }          ///< byte_11A42: passed to PROG1 as parm_02
	uint16 member(int i) const { return (uint16)(raw[0x26 + 2 * i] | (raw[0x27 + 2 * i] << 8)); }
};

class Shell {
public:
	Shell(Shared::Runtime &rt, Launcher &launcher, const Config &config);

	/** Run the shell until the player quits. Returns the DOS exit code. */
	int run();
	/** The text the original prints to the DOS prompt on the way out ("" when nothing). */
	const std::string &exitMessage() const { return _message; }

private:
	bool loadHeader();
	bool saveHeader();
	bool startVideoDriver();                  // sub_10873
	void calibrateDelays();                   // sub_10B08
	void openDataFilesAndClear();             // sub_105AF
	void flushPartyAndRefresh();              // sub_1082E
	void showSharewareMessage(bool afterPlay);// sub_10984 / sub_10A46
	void showEndScreen();                     // sub_103AB
	int runProgram(int program);              // execProgram + the parameter block
	int runPrologue();                        // sub_10571
	int bookMenuLoop();                       // loc_101B9
	int gameLoop();                           // loc_1021E
	int endPath();                            // loc_10265

	Shared::Runtime &_rt;
	Launcher &_launcher;
	Config _config;
	Data::ExeImage _exe;
	PartyHeader _header;
	Shared::Params _params;
	std::string _message;
	bool _stopped;      // a program that is not implemented yet was requested
};

} // namespace Sw
} // namespace Yendor1

#endif

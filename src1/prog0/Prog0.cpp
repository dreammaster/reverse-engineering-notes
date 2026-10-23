#include "prog0/Prog0.h"

#include <stdio.h>
#include <string.h>

namespace Yendor1 {
namespace Prog0 {

namespace {

const int kPartyRecord = 100, kNpcRecord = 32, kAreaRecord = 128;
const int kNpcRecords = 446;                 // 1BDh records are copied after the first
const int kExitNoSave = 0x21, kExitNoParty = 0x20, kExitBadMode = 0xFE;

} // namespace

Prog0::Prog0(Shared::Runtime &rt, const Shared::Params &params) : _rt(rt), _params(params) {
}

std::string Prog0::path(const char *dir, const char *file) const {
	std::string p = _rt.workDir();
	if (!p.empty() && p[p.size() - 1] != '/' && p[p.size() - 1] != '\\')
		p += "/";
	return p + dir + "/" + file;
}

void Prog0::text(int col, int row, const char *s) {
	_rt.system.textOut(col, row, s);
}

// FINDFIRST/FINDNEXT of "SAVEGAME.*": the extension letters of the saved games. (The DOS directory order is not reproduced; the
// letters are probed in alphabetical order, digits last.)
std::vector<char> Prog0::findSlots() {
	std::vector<char> slots;
	for (int i = 0; i < 36; i++) {
		const char c = i < 26 ? (char)('A' + i) : (char)('0' + i - 26);
		const std::string dir = std::string("SAVEGAME.") + c;
		if (FILE *f = fopen(path(dir.c_str(), "PARTY.DAT").c_str(), "rb")) {
			fclose(f);
			slots.push_back(c);
		}
	}
	return slots;
}

// mode 1 with several saved games: the list ("Following Parties Found:", each with its party name) and the number prompt
int Prog0::chooseSlot(const std::vector<char> &slots, char &chosen) {
	text(0x1D, 4, "Following Parties Found:");
	int row = 5;
	for (size_t i = 0; i < slots.size(); i++) {
		row++;
		const std::string dir = std::string("SAVEGAME.") + slots[i];
		byte record[kPartyRecord];
		memset(record, 0, sizeof(record));
		Shared::RecordFile f;
		if (f.open(path(dir.c_str(), "PARTY.DAT").c_str(), kPartyRecord, false))
			f.read(0, record);
		record[kPartyRecord - 1] = 0;
		char number[8];
		snprintf(number, sizeof(number), "%2u.", (unsigned)(i + 1));
		text(0x19, row, number);
		text(0x1D, row, (const char *)record + 2);
	}
	row += 2;
	text(0x1D, row, "Select Party to be active   ");
	const std::string line = _rt.system.readLine();
	unsigned value = 0;
	for (size_t i = 0; i < line.size() && i < 3; i++) {
		if (line[i] < '0' || line[i] > '9') {
			value = 0;
			break;
		}
		value = value * 10 + (unsigned)(line[i] - '0');
	}
	if (value == 0 || value > slots.size())
		return kExitNoParty;
	chosen = slots[value - 1];
	return 0;
}

// sub_103C0 (mode 1: the CURRGAME files too) and sub_103AC/loc_10407 (the saved game's files): everything has to open. Returns the
// exit code (0 = fine).
int Prog0::openAll(char slot) {
	const std::string dir = std::string("SAVEGAME.") + slot;
	struct File { const char *dir; const char *name; int failCode; };
	std::vector<File> files;
	if (_params.parm(0x0E) == 1) {
		files.push_back({"CURRGAME", "NPC.DAT", 1});
		files.push_back({"CURRGAME", "AREA.DAT", 1});
		files.push_back({"CURRGAME", "PARTY.DAT", 3});
	}
	files.push_back({dir.c_str(), "PARTY.DAT", 9});
	files.push_back({dir.c_str(), "NPC.DAT", 0x0A});
	files.push_back({dir.c_str(), "AREA.DAT", 0x0B});
	for (size_t i = 0; i < files.size(); i++) {
		FILE *f = fopen(path(files[i].dir, files[i].name).c_str(), "r+b");
		if (!f)
			return files[i].failCode;
		fclose(f);
	}
	return 0;
}

// Copy the party file (the record count is the first word of its record 0), the 446 NPC records and as many 128-byte area records as
// bytes 4-5 of the first NPC record say. A record the source cannot deliver leaves the buffer as it was, as with DOS.
int Prog0::copyGame(bool restore, char slot) {
	const std::string saved = std::string("SAVEGAME.") + slot;
	const char *from = restore ? saved.c_str() : "CURRGAME";
	const char *to = restore ? "CURRGAME" : saved.c_str();

	Shared::RecordFile src, dst;
	byte buf[kAreaRecord];
	memset(buf, 0, sizeof(buf));

	if (!src.open(path(from, "PARTY.DAT").c_str(), kPartyRecord, false) || !dst.open(path(to, "PARTY.DAT").c_str(), kPartyRecord, true))
		return 0;
	src.read(0, buf);
	const uint16 partyRecords = (uint16)(buf[0] | (buf[1] << 8));
	dst.write(0, buf);
	for (uint16 n = 1; n < partyRecords; n++) {
		src.read(n, buf);
		dst.write(n, buf);
	}

	if (!src.open(path(from, "NPC.DAT").c_str(), kNpcRecord, false) || !dst.open(path(to, "NPC.DAT").c_str(), kNpcRecord, true))
		return 0;
	src.read(0, buf);
	const uint16 areaRecords = (uint16)(buf[4] | (buf[5] << 8));
	dst.write(0, buf);
	for (int n = 1; n < kNpcRecords; n++) {
		src.read((uint16)n, buf);
		dst.write((uint16)n, buf);
	}

	if (!src.open(path(from, "AREA.DAT").c_str(), kAreaRecord, false) || !dst.open(path(to, "AREA.DAT").c_str(), kAreaRecord, true))
		return 0;
	for (uint16 n = 0; n < areaRecords; n++) {
		src.read(n, buf);
		dst.write(n, buf);
	}
	return 0;
}

int Prog0::run() {
	char slot = 0;
	int code = 0;
	switch (_params.parm(0x0E)) {
	case 1: {
		const std::vector<char> slots = findSlots();
		if (slots.empty())
			return kExitNoSave;
		if (slots.size() > 1) {
			code = chooseSlot(slots, slot);
			if (code != 0)
				return code;
		} else {
			slot = slots[0];
		}
		code = openAll(slot);
		if (code != 0)
			return code;
		text(1, 0x18, "Copying game files");
		return copyGame(true, slot);
	}
	case 2:
	case 3:
		slot = (char)_params.parm(0x10);
		code = openAll(slot);
		if (code != 0)
			return code;
		return copyGame(_params.parm(0x0E) == 3, slot);
	default:
		return kExitBadMode;
	}
}

} // namespace Prog0
} // namespace Yendor1

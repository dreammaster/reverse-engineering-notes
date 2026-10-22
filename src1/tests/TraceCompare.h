/* Comparing a reimplemented program's INT 62h call trace (Vid::formatCall lines) with the original's (yemu --log62).
 * Registers and fields a call does not read are masked, and runs of pixel operations (the random-order dissolve) only have to
 * contain the same set of calls. */

#ifndef YENDOR1_TESTS_TRACECOMPARE_H
#define YENDOR1_TESTS_TRACECOMPARE_H

#include <algorithm>
#include <fstream>
#include <stdlib.h>
#include <string>
#include <vector>

namespace TraceCompare {

inline int ahOf(const std::string &l) {
	return (int)strtol(l.substr(0, 2).c_str(), 0, 16);
}

inline std::string maskField(std::string r, const char *key) {
	const size_t c = r.find(key);
	if (c == std::string::npos)
		return r;
	const size_t e = r.find(' ', c + 1);
	return r.substr(0, c) + key + "-" + (e == std::string::npos ? std::string() : r.substr(e));
}

/** Mask what the call does not read: AL (only 0Dh, 12h, 2Dh take it), the drawing colour (only 03, 05, 06, 07 use it; 04 returns it). */
inline std::string normalise(const std::string &l) {
	const int ah = ahOf(l);
	std::string r = (ah == 0x0D || ah == 0x12 || ah == 0x2D) ? l : l.substr(0, 3) + "--" + l.substr(5);
	if (!(ah == 0x03 || ah == 0x05 || ah == 0x06 || ah == 0x07))
		r = maskField(r, " col=");
	return r;
}

inline bool isPixelOp(const std::string &l) {
	return l.compare(0, 2, "03") == 0 || l.compare(0, 2, "04") == 0;
}

/** "" when equal, else a description of the first difference. */
inline std::string compare(const std::vector<std::string> &want, const std::vector<std::string> &got) {
	size_t i = 0, j = 0;
	while (i < want.size() && j < got.size()) {
		if (isPixelOp(want[i]) || isPixelOp(got[j])) {
			size_t ie = i, je = j;
			while (ie < want.size() && isPixelOp(want[ie])) ie++;
			while (je < got.size() && isPixelOp(got[je])) je++;
			std::vector<std::string> a(want.begin() + i, want.begin() + ie), b(got.begin() + j, got.begin() + je);
			std::sort(a.begin(), a.end());
			std::sort(b.begin(), b.end());
			if (a != b) {
				size_t k = 0;
				while (k < a.size() && k < b.size() && a[k] == b[k]) k++;
				return "pixel run at call " + std::to_string(i) + " differs (" + std::to_string(a.size()) + " vs " + std::to_string(b.size()) +
					" ops); first difference: want [" + (k < a.size() ? a[k] : "-") + "] got [" + (k < b.size() ? b[k] : "-") + "]";
			}
			i = ie;
			j = je;
			continue;
		}
		if (want[i] != got[j])
			return "call " + std::to_string(i) + " (got index " + std::to_string(j) + "):\n   want " + want[i] + "\n   got  " + got[j];
		i++;
		j++;
	}
	if (i != want.size() || j != got.size())
		return "length differs: want " + std::to_string(want.size()) + " calls, got " + std::to_string(got.size());
	return "";
}

/** Lines of the trace file between the `occurrence`-th (1-based) "# start <program>" marker and the next marker or the end. */
inline std::vector<std::string> slice(const std::string &file, const std::string &program, int occurrence = 1) {
	std::ifstream f(file.c_str());
	std::vector<std::string> out;
	std::string line;
	bool on = false;
	int seen = 0;
	while (std::getline(f, line)) {
		if (!line.empty() && line[0] == '#') {
			if (on)
				break;
			if (line == "# start " + program && ++seen == occurrence)
				on = true;
			continue;
		}
		if (on)
			out.push_back(line);
	}
	return out;
}

inline std::vector<std::string> until(const std::string &file, const std::string &stopAt) {
	std::ifstream f(file.c_str());
	std::vector<std::string> out;
	std::string line;
	while (std::getline(f, line)) {
		if (!line.empty() && line[0] == '#') {
			if (line == "# start " + stopAt)
				break;
			continue;
		}
		out.push_back(line);
	}
	return out;
}

} // namespace TraceCompare

#endif

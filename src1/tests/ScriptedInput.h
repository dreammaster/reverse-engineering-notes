/* Test doubles for running a ported program against a yemu reference run (see test_prog3.cpp): a call recorder, a System that plays
 * a script of keys and mouse actions, and a video memory comparison. The script is played as the program polls for input: one step per
 * poll, so a step is seen exactly when the program is waiting, as in the original run where the script waited between steps. */

#ifndef YENDOR1_TESTS_SCRIPTEDINPUT_H
#define YENDOR1_TESTS_SCRIPTEDINPUT_H

#include "TraceCompare.h"
#include "shared/Runtime.h"
#include "tools/Snapshot.h"
#include "vid/Trace.h"

#include <stdio.h>
#include <stdlib.h>
#include <string>
#include <vector>

namespace ScriptedInput {

using namespace Yendor1;

class Recorder : public Vid::CallTrace {
public:
	std::vector<std::string> lines;
	void onCall(const Vid::Video &v, int ah, int al) {
		if (getenv("CALL_LOG") && ah != 0x2B)
			fprintf(stderr, "CALL %02X %02X\n", ah, al);
		lines.push_back(Vid::formatCall(v, ah, al));
	}
};

struct Step {
	enum Kind { Key, Ext, MoveTo, LeftClick, RightClick };
	Kind kind;
	int a, b;
	Step(Kind k, int x = 0, int y = 0) : kind(k), a(x), b(y) {}
};

inline Step key(int ch) { return Step(Step::Key, ch); }
inline Step ext(int scan) { return Step(Step::Ext, scan); }
inline Step moveTo(int x, int y) { return Step(Step::MoveTo, x, y); }
inline Step click() { return Step(Step::LeftClick); }
inline Step rightClick() { return Step(Step::RightClick); }

class InputScript : public Shared::System {
public:
	InputScript(Vid::Video &v, const std::vector<Step> &s) : _video(v), _steps(s), _next(0), _scan(-1), _now(0), _idleAfterEnd(0) {}

	bool pump() { return _next < _steps.size() || _scan >= 0 || ++_idleAfterEnd < 3000; }   // a program still waiting after the script ends is stopped
	int pollKey() {
		if (_scan >= 0) {
			const int s = _scan;
			_scan = -1;
			return s;
		}
		if (_next >= _steps.size())
			return -1;
		const Step &s = _steps[_next++];
		Vid::Context &c = _video.ctx;
		switch (s.kind) {
		case Step::Key:
			return s.a;
		case Step::Ext:
			_scan = s.a;
			return 0;
		case Step::MoveTo:
			_video.mouseEvent(true, s.a - c.mouseX, s.b - c.mouseY, 0, 0);
			return -1;
		case Step::LeftClick:
			_video.mouseEvent(false, 0, 0, 1, 0);
			return -1;
		case Step::RightClick:
			_video.mouseEvent(false, 0, 0, 0, 1);
			return -1;
		}
		return -1;
	}
	void flushKeys() {}
	Shared::uint32 ticks() { return _now++; }
	void idle() {}
	void speaker(Shared::uint16) {}
	Shared::uint16 cpuClass() { return 0x386; }
	bool finished() const { return _next >= _steps.size(); }

private:
	Vid::Video &_video;
	std::vector<Step> _steps;
	size_t _next;
	int _scan;
	Shared::uint32 _now;
	int _idleAfterEnd;
};

/** Drop the pointer refreshes (2Bh): they depend on when the original's mouse moved, not on the program. */
inline std::vector<std::string> withoutRefresh(const std::vector<std::string> &lines, bool normalise) {
	std::vector<std::string> out;
	for (size_t i = 0; i < lines.size(); i++)
		if (TraceCompare::ahOf(lines[i]) != 0x2B)
			out.push_back(normalise ? TraceCompare::normalise(lines[i]) : lines[i]);
	return out;
}

/** Number of differing bytes between the port's video memory and a snapshot (prints the first). */
inline int videoDifferences(const Vid::Video &v, const Tools::Snapshot &snap) {
	int bad = 0, plane = -1, off = -1;
	for (int p = 0; p < 4; p++)
		for (int o = 0; o < 65536; o++)
			if (v.planeData(p)[o] != snap.planes[p][o]) {
				if (!bad) { plane = p; off = o; }
				bad++;
			}
	if (bad)
		printf("  video memory differs in %d bytes (first: plane %d offset %04X = page %d row %d byte %d)\n", bad, plane, off, off >> 15,
		       (off & 0x7FFF) / 80, (off & 0x7FFF) % 80);
	return bad;
}

/** Print the calls around the first difference reported by TraceCompare::compare. */
inline void showContext(const std::string &diff, const std::vector<std::string> &want, const std::vector<std::string> &got) {
	unsigned long atv = 0;
	if (sscanf(diff.c_str(), "call %lu", &atv) != 1)
		return;
	const size_t at = (size_t)atv;
	for (size_t k = at > 6 ? at - 6 : 0; k < at + 3 && k < want.size() && k < got.size(); k++)
		printf("   [%lu] want %.90s\n        got  %.90s\n", (unsigned long)k, want[k].c_str(), got[k].c_str());
}

} // namespace ScriptedInput

#endif

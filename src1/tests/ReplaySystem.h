/* Replaying a reference run: a System that gives the program the keys the original read (trace.txt.events, the "K" lines yemu writes:
 * program, driver calls so far, number of the INT 21h/06h poll of that program, byte) and stops the program at the first driver call
 * that differs from the original's trace.
 *   - a translated program (Shared::Machine16) is given each key at the very poll the original read it: the translated programs
 *     count work in loop iterations (the NPC sweeps of PROG4), and the original read its keys after an arbitrary number of them,
 *   - a hand-ported program is given the next key of its program once the port has made as many driver calls as the original had. */

#ifndef YENDOR1_TESTS_REPLAYSYSTEM_H
#define YENDOR1_TESTS_REPLAYSYSTEM_H

#include "ScriptedInput.h"

#include <deque>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

namespace ScriptedInput {

struct KeyRead {
	size_t calls;        // driver calls (pointer refreshes excluded) the original had made, counted from `base`
	unsigned long long poll;     // polls of the program since its last driver call
	int key;
};

/** The keys of every program in the reference run's trace.txt.events; `base` is subtracted from the call counts. */
struct KeyReads {
	std::map<std::string, std::deque<KeyRead> > byProgram;
	// BIOS tick reads: per program the instances (one per start), each a sparse tape of (read number, value)
	std::map<std::string, std::vector<std::vector<std::pair<unsigned long long, unsigned> > > > ticks;
	void load(const std::string &file, size_t base) {
		byProgram.clear();
		FILE *f = fopen(file.c_str(), "r");
		if (!f)
			return;
		char line[256];
		while (fgets(line, sizeof(line), f)) {
			char tag[8], prog[64];
			long calls;
			unsigned long long poll;
			int key;
			unsigned t;
			if (sscanf(line, "T %63s %llu %u", prog, &poll, &t) == 3) {
				std::vector<std::vector<std::pair<unsigned long long, unsigned> > > &inst = ticks[prog];
				if (poll == 0 || inst.empty())
					inst.push_back(std::vector<std::pair<unsigned long long, unsigned> >());
				inst.back().push_back(std::make_pair(poll, t));
				continue;
			}
			if (sscanf(line, "%7s %63s %ld %llu %d", tag, prog, &calls, &poll, &key) != 5 || strcmp(tag, "K") != 0)
				continue;
			KeyRead k;
			k.calls = (size_t)calls >= base ? (size_t)calls - base : 0;
			k.poll = poll;
			k.key = key;
			byProgram[prog].push_back(k);
		}
		fclose(f);
	}
};

// stops the program when the port has recorded `limit` calls
class Replay : public InputScript {
public:
	Replay(Vid::Video &v, const std::vector<Step> &s, const Recorder &r, size_t limit, unsigned clock, const std::vector<std::string> *want,
	       const KeyReads &keys)
		: InputScript(v, s), _rec(r), _limit(limit), _clock(clock), _want(want), _checked(0), _matched(0), _failed(false), _keys(keys), _counted(0),
		  _calls(0), _idle(0) {}

	// the reference run executed `refName` where the port runs `program` (a game directory with PROG4.EXE replaced by another program)
	void alias(int program, const char *refName) { _alias[program] = refName; }
	void programStarted(int program, bool generated) {
		char name[32];
		snprintf(name, sizeof(name), "PROG%d.EXE", program);
		if (_alias.count(program))
			snprintf(name, sizeof(name), "%s", _alias[program].c_str());
		Active a;
		a.name = name;
		a.generated = generated;
		a.instance = _started[a.name]++;
		_stack.push_back(a);
	}
	void programEnded() {
		if (!_stack.empty())
			_stack.pop_back();
	}
	// a translated program: the key the original read at this poll after the call (the key is read once the port has made as many
	// calls as the original had and polled as often since the last one)
	int pollKeyFor(unsigned long long sinceCall) {
		if (_stack.empty() || !_stack.back().generated)
			return pollKey();
		countCalls();
		std::deque<KeyRead> &q = _keys.byProgram[_stack.back().name];
		if (!q.empty() && (_calls > q.front().calls || (_calls == q.front().calls && sinceCall >= q.front().poll))) {
			const int k = q.front().key;
			q.pop_front();
			return k;
		}
		return -1;
	}
	// a translated program: the BIOS tick count the original read at this read
	Shared::uint32 ticksFor(unsigned long long n) {
		if (!_stack.empty() && _stack.back().generated) {
			const Active &a = _stack.back();
			std::map<std::string, std::vector<std::vector<std::pair<unsigned long long, unsigned> > > >::const_iterator it = _keys.ticks.find(a.name);
			if (it != _keys.ticks.end() && a.instance < it->second.size() && !it->second[a.instance].empty()) {
				const std::vector<std::pair<unsigned long long, unsigned> > &tape = it->second[a.instance];
				size_t lo = 0, hi = tape.size();
				while (hi - lo > 1) {
					const size_t mid = (lo + hi) / 2;
					if (tape[mid].first <= n) lo = mid; else hi = mid;
				}
				return tape[lo].second;
			}
		}
		return ticks();
	}
	// a hand-ported program (or the shell): the next key of its program when the port has made as many calls
	int pollKey() {
		countCalls();
		const std::string name = _stack.empty() ? std::string("SW.EXE") : _stack.back().name;
		std::deque<KeyRead> &q = _keys.byProgram[name];
		if (!q.empty() && _calls >= q.front().calls) {
			const int k = q.front().key;
			q.pop_front();
			return k;
		}
		return -1;
	}
	// stops at the first call that differs from the reference, when all of its calls were made, or when the program has been waiting
	// for a long time without making a call (the original's trace ends with its last call)
	bool pump() {
		const size_t before = _matched;
		while (_want && !_failed && _checked < _rec.lines.size()) {
			const std::string &l = _rec.lines[_checked++];
			if (TraceCompare::ahOf(l) == 0x2B)
				continue;
			if (_matched < _want->size() && TraceCompare::normalise(l) != (*_want)[_matched])
				_failed = true;
			else
				_matched++;
		}
		_idle = _matched != before ? 0 : _idle + 1;
		const bool done = _want && _matched >= _want->size();   // all of the original's calls were made: the original was waiting too
		const bool go = !_failed && _rec.lines.size() < _limit && _idle < (done ? 2000 : 100000000);
		if (!go && getenv("YENDOR1_EXITS"))
			fprintf(stderr, "replay stops: failed=%d lines=%zu limit=%zu idle=%zu matched=%zu\n", (int)_failed, _rec.lines.size(), _limit, _idle, _matched);
		return go;
	}
	bool failed() const { return _failed; }
	Shared::uint16 dosClock() { return (Shared::uint16)_clock; }

private:
	struct Active {
		std::string name;
		bool generated;
		size_t instance;
	};
	void countCalls() {
		for (; _counted < _rec.lines.size(); _counted++)
			if (TraceCompare::ahOf(_rec.lines[_counted]) != 0x2B)
				_calls++;
	}

	const Recorder &_rec;
	size_t _limit;
	unsigned _clock;
	const std::vector<std::string> *_want;
	size_t _checked, _matched;
	bool _failed;
	KeyReads _keys;
	std::vector<Active> _stack;
	std::map<int, std::string> _alias;
	std::map<std::string, size_t> _started;
	size_t _counted, _calls, _idle;
};

// driver calls (without the pointer refreshes) before the program's start marker
inline size_t callsBefore(const std::string &file, const std::string &program) {
	std::vector<std::string> lines = TraceCompare::until(file, program);
	size_t n = 0;
	for (size_t i = 0; i < lines.size(); i++)
		if (TraceCompare::ahOf(lines[i]) != 0x2B)
			n++;
	return n;
}

} // namespace ScriptedInput

#endif

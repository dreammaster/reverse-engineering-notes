// fuzz_tool EXE GAMEDIR ANALYSISDIR OUTOPS SECONDS [SEED]
// Coverage-guided state explorer on the native engine. A scenario is a list of operations (reset / flag / goto / move / cmd / attr) from the start state; scenarios
// that execute game-layer instructions no earlier scenario did are kept in a corpus and extended with more random operations, so rare states (flags, party, items, rooms)
// are reached much faster than with plain random testing. Every kept scenario is written to OUTOPS (the format of engine_tool and tests/replay_diff.py), which then lets the
// unicorn oracle verify it: python port/tests/replay_diff.py OUTOPS
#include "engine.h"

#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <random>
#include <sstream>
#include <vector>

using namespace shannara;

struct Op {
	char kind; // R reset, F flag, G goto, M move, C cmd, A attr
	int a, b, c;
};

static const char *kindName(char k) {
	switch (k) {
	case 'R': return "reset";
	case 'F': return "flag";
	case 'G': return "goto";
	case 'M': return "move";
	case 'C': return "cmd";
	default: return "attr";
	}
}

struct Verb {
	int id, nobj;
};

static std::vector<std::string> readLines(const std::string &path) {
	std::ifstream f(path);
	std::vector<std::string> v;
	std::string l;
	while (std::getline(f, l)) {
		if (!l.empty() && l.back() == '\r')
			l.pop_back();
		v.push_back(l);
	}
	return v;
}

static std::vector<std::string> split(const std::string &s, char c) {
	std::vector<std::string> v;
	std::stringstream ss(s);
	std::string t;
	while (std::getline(ss, t, c))
		v.push_back(t);
	return v;
}

class Fuzzer {
public:
	Fuzzer(Engine &e, const std::string &analysis, uint32_t seed) : _e(e), _rng(seed) {
		for (auto &l : readLines(analysis + "/retail_verb_handlers.tsv")) {
			auto f = split(l, '\t');
			if (f.size() >= 5 && isdigit((unsigned char)f[0][0]) && f[2] != "Wait") // Wait spins on the (frozen) clock
				_verbs.push_back({atoi(f[0].c_str()), atoi(f[4].c_str())});
		}
		bool first = true;
		for (auto &l : readLines(analysis + "/retail_rooms.tsv")) {
			if (first) {
				first = false;
				continue;
			}
			auto f = split(l, '\t');
			if (!f.empty() && !f[0].empty())
				_rooms.push_back(atoi(f[0].c_str()));
		}
		_objects = _e.machine().r16(0x217A0);
		_seen.assign(0x75000, 0);
		_e.machine().enableCoverage();
		_e.setBudget(1500000);
	}

	// executes one operation; false if it faulted (budget...) - the caller then rebuilds the state
	bool exec(const Op &o) {
		Machine &m = _e.machine();
		try {
			switch (o.kind) {
			case 'R': _e.restore(); break;
			case 'F': {
				uint32_t addr = 0xCD8AC + (uint32_t)((o.a - 1) >> 3);
				uint8_t bit = (uint8_t)(1 << ((o.a - 1) & 7));
				m.w8(addr, o.b ? (m.r8(addr) | bit) : (m.r8(addr) & ~bit));
				break;
			}
			case 'G': _e.callFunction(0x1EE10, (uint32_t)o.a); break;
			case 'M': _e.callFunction(0x1B470, (uint32_t)o.a, (uint32_t)o.b, (uint32_t)o.c); break;
			case 'C': _e.command(o.a, o.b, o.c); break;
			case 'A': _e.callFunction(o.c ? 0x1A1B0 : 0x1A1D8, (uint32_t)o.a, (uint32_t)o.b); break;
			}
		} catch (const std::exception &) {
			return false;
		}
		_e.takeText();
		return true;
	}

	bool inScope(int parent, int k) { return _e.callFunction(0x1B5C8, (uint32_t)parent, (uint32_t)k) & 0xFF; }

	void refreshScope() {
		_scope.clear();
		try {
			int here = _e.machine().r16(0xADE94 + 2);
			for (int k = 1; k <= _objects; k++)
				if (k != here && (inScope(here, k) || inScope(3, k)))
					_scope.push_back(k);
		} catch (const std::exception &) { // a broken object list (random moves can create cycles): no scope this time
			_scope.clear();
		}
	}

	Op randomOp(bool &changesScope) {
		changesScope = false;
		double r = uni();
		if (r < 0.72) {
			Verb v = _verbs[_rng() % _verbs.size()];
			auto pick = [&]() { return !_scope.empty() && uni() < 0.8 ? _scope[_rng() % _scope.size()] : 1 + (int)(_rng() % _objects); };
			int d = v.nobj >= 1 ? pick() : 0;
			int i = v.nobj >= 2 ? pick() : 0;
			return {'C', v.id, d, i};
		}
		if (r < 0.84)
			return {'F', 1 + (int)(_rng() % 800), (int)(_rng() & 1), 0};
		if (r < 0.90) {
			changesScope = true;
			return {'G', _rooms[_rng() % _rooms.size()], 0, 0};
		}
		if (r < 0.97) {
			changesScope = true;
			int here = _e.machine().r16(0xADE94 + 2);
			int o = 1 + (int)(_rng() % _objects);
			int par = uni() < 0.5 ? here : (uni() < 0.5 ? 3 : 1 + (int)(_rng() % _objects));
			return {'M', o, par, (int)(_rng() % 3)};
		}
		return {'A', 1 + (int)(_rng() % _objects), 1 + (int)(_rng() % 24), (int)(_rng() & 1)};
	}

	// rebuilds the machine state by replaying ops (dropping those that fault)
	void rebuild(std::vector<Op> &ops) {
		std::vector<Op> ok;
		for (auto &o : ops)
			if (exec(o))
				ok.push_back(o);
		ops = ok;
	}

	// instructions executed by the scenario just run that no earlier scenario executed; `all` receives every executed address
	size_t newCoverage(std::vector<uint32_t> &all) {
		size_t n = 0;
		const auto &cov = _e.machine().coverage;
		all.clear();
		for (size_t a = 0x10000; a < _seen.size() && a < cov.size(); a++)
			if (cov[a]) {
				all.push_back((uint32_t)a);
				if (!_seen[a]) {
					_seen[a] = 1;
					n++;
				}
			}
		return n;
	}

	void run(const std::string &outPath, double seconds) {
		FILE *out = fopen(outPath.c_str(), "w");
		auto t0 = std::chrono::steady_clock::now();
		auto elapsed = [&]() { return std::chrono::duration<double>(std::chrono::steady_clock::now() - t0).count(); };
		std::vector<std::vector<Op>> corpus = {{{'R', 0, 0, 0}}};
		size_t total = 0, execs = 0, kept = 0;
		std::vector<std::vector<uint32_t>> covSets; // full coverage of corpus[i + 1]
		std::vector<uint32_t> covSet;
		while (elapsed() < seconds) {
			execs++;
			std::vector<Op> ops = corpus[_rng() % corpus.size()];
			std::fill(_e.machine().coverage.begin(), _e.machine().coverage.end(), 0);
			rebuild(ops);
			bool scope = false;
			refreshScope();
			if (corpus.size() == 1 || uni() < 0.3) { // a fresh random flag density at the start of a lineage
				double d = uni();
				for (int fl = 1; fl <= 800; fl++)
					if (uni() < d * d) {
						Op o = {'F', fl, 1, 0};
						if (exec(o))
							ops.push_back(o);
					}
			}
			int extra = 8 + (int)(_rng() % 40);
			for (int i = 0; i < extra; i++) {
				Op o = randomOp(scope);
				if (!exec(o)) { // faulted: the state is polluted - rebuild from the recorded ops
					_e.restore();
					rebuild(ops);
					continue;
				}
				ops.push_back(o);
				if (scope)
					refreshScope();
			}
			size_t gain = newCoverage(covSet);
			if (gain) {
				total += gain;
				kept++;
				corpus.push_back(ops);
				covSets.push_back(covSet);
				fprintf(stderr, "[%6.0fs] exec %zu corpus %zu: +%zu -> %zu instructions\n", elapsed(), execs, corpus.size(), gain, total);
			}
		}
		// greedy set cover: the fewest scenarios that together reach everything found (keeps the file small enough for the oracle to replay)
		std::vector<uint8_t> have(_seen.size(), 0);
		std::vector<bool> used(covSets.size(), false);
		size_t written = 0, covered = 0;
		for (;;) {
			size_t best = covSets.size(), bestGain = 0;
			for (size_t i = 0; i < covSets.size(); i++) {
				if (used[i])
					continue;
				size_t g = 0;
				for (uint32_t a : covSets[i])
					g += !have[a];
				if (g > bestGain) {
					bestGain = g;
					best = i;
				}
			}
			if (!bestGain)
				break;
			used[best] = true;
			for (uint32_t a : covSets[best])
				if (!have[a]) {
					have[a] = 1;
					covered++;
				}
			written++;
			fprintf(out, "# selected scenario %zu (corpus %zu): +%zu instructions (total %zu)\n", written, best + 1, bestGain, covered);
			for (auto &o : corpus[best + 1])
				fprintf(out, "%s %d %d %d\n", kindName(o.kind), o.a, o.b, o.c);
		}
		fprintf(stderr, "selected %zu of %zu scenarios covering %zu instructions\n", written, covSets.size(), covered);
		fclose(out);
		fprintf(stderr, "done: %zu executions, %zu scenarios kept, %zu instructions covered\n", execs, kept, total);
	}

private:
	double uni() { return (_rng() & 0xFFFFFF) / 16777216.0; }

	Engine &_e;
	std::mt19937 _rng;
	std::vector<Verb> _verbs;
	std::vector<int> _rooms, _scope;
	std::vector<uint8_t> _seen;
	int _objects = 0;
};

int main(int argc, char **argv) {
	setvbuf(stderr, nullptr, _IOLBF, 0);
	if (argc < 6) {
		fprintf(stderr, "usage: fuzz_tool EXE GAMEDIR ANALYSISDIR OUTOPS SECONDS [SEED]\n");
		return 2;
	}
	try {
		Engine e(argv[1], argv[2]);
		e.snapshot();
		Fuzzer f(e, argv[3], argc > 6 ? (uint32_t)atoi(argv[6]) : 1);
		f.run(argv[4], atof(argv[5]));
	} catch (const std::exception &x) {
		fprintf(stderr, "error: %s\n", x.what());
		return 1;
	}
	return 0;
}

"""
Cross-checks the two independent translations of every Xanth handler:

  * engine/gen/handlers_gen.cpp  - from the symbolic digest (gen_handlers.py)
  * engine/gen_all/lifted_*.cpp  - instruction-level lifts of the same functions from xanth.asm (lift_handlers.py --all)

Both are run under the same all-zero game state for a grid of (phase, verb) combinations with mock hosts that only record the names of the engine functions
called; the sequences must agree.  Writes engine/gen_all/{mock_gen.cpp,test_main.cpp}, builds and runs them.

    python make_trace_test.py        # needs lift_handlers.py --all to have been run
"""
import os
import re
import subprocess
import sys

HERE = os.path.dirname(os.path.abspath(__file__))
E = os.path.join(HERE, "..", "..", "engine")
GEN, ALL = os.path.join(E, "gen"), os.path.join(E, "gen_all")

api = open(os.path.join(GEN, "handlers_api.h")).read()
variables = re.findall(r"^extern int (\w+);", api, re.M)
funcs = re.findall(r"^intptr_t (\w+)\(\.\.\.\);", api, re.M)
gen_handlers = re.findall(r"^intptr_t (\w+)\(int phase, int arg_2 = 0, int arg_4 = 0\);", api, re.M)
lapi = open(os.path.join(ALL, "lifted_api.h")).read()
hosts = re.findall(r"^void H_(\w+)\(Cpu &c\);", lapi, re.M)
all_names = re.findall(r"^intptr_t LW_(\w+)\(int phase", lapi, re.M)
both = [n for n in gen_handlers if n in all_names]

with open(os.path.join(ALL, "mock_gen.cpp"), "w") as f:
    f.write('#include "handlers_api.h"\n#include <string>\n#include <vector>\nextern std::vector<std::string> g_trace;\n')
    f.write("int cmd, dobj, iobj;\n")
    for v in variables:
        if v not in ("cmd", "dobj", "iobj"):
            f.write("int %s;\n" % v)
    for fn in funcs:
        f.write('intptr_t %s(...) { g_trace.push_back("%s"); return 0; }\n' % (fn, fn))
    f.write("void reset_gen() {\n\tcmd = dobj = iobj = 0;\n")
    for v in variables:
        if v not in ("cmd", "dobj", "iobj"):
            f.write("\t%s = 0;\n" % v)
    f.write("}\n")

with open(os.path.join(ALL, "test_main.cpp"), "w") as f:
    f.write('#define LIFT_STEP_LIMIT 200000\n#include "lifted_api.h"\n#include "lifted_data.h"\n#include <cstdio>\n#include <cstdlib>\n#include <string>\n#include <vector>\n')
    f.write("std::vector<std::string> g_trace;\nstatic Cpu g_cpu;\nCpu &liftedCpu() { return g_cpu; }\n")
    f.write("static std::string norm(std::string s) { if (s.rfind(\"j_\", 0) == 0) s = s.substr(2); if (s.size() > 5 && s.compare(s.size() - 5, 5, \"_stub\") == 0) s.resize(s.size() - 5); if (s == \"show_compendium\") s = \"Conversation_Show\"; return s; }\n")
    for h in hosts:
        f.write('void H_%s(Cpu &c) { g_trace.push_back(norm("%s")); c.ax.x = 0; c.dx.x = 0; }\n' % (h, h))
    f.write("extern int cmd, dobj, iobj, selectedCommand, current_room;\nvoid reset_gen();\n")
    for n in both:
        f.write("intptr_t %s(int, int, int);\n" % n)
    f.write("struct Pair { const char *name; intptr_t (*gen)(int, int, int); intptr_t (*lift)(int, int, int); };\nstatic Pair pairs[] = {\n")
    for n in both:
        f.write('\t{"%s", %s, LW_%s},\n' % (n, n, n))
    f.write("};\n")
    f.write(r'''
int main() {
	static uint8_t mem[1 << 20];
	g_cpu.mem = mem;
	g_cpu.ss = 0x2000;
	g_cpu.ds = g_cpu.es = g_cpu.cs = 0x84FC;
	int phases[] = {0, 1, 2, 3, 4, 5, 6, 7, 0xA, 0xD, 0xE, 0xF, 0x1A, 0x1B};
	int verbs[] = {0, 1, 19, 25, 68, 69, 70, 71, 5, 33, 40, 55};
	int agree = 0, differ = 0, scen_total = 0, scen_bad = 0;
	std::vector<std::string> worst;
	for (auto &p : pairs) {
		int bad = 0, total = 0;
		for (int ph : phases)
			for (int v : verbs) {
				memset(mem, 0, sizeof(mem));
				reset_gen();
				g_cpu.sp = 0xF000;
				g_cpu.steps = 0;
				cmd = selectedCommand = v; dobj = 7; iobj = 8; current_room = 34;
				g_cpu.wr16(g_cpu.ds, D_selectedCommand, v);
				g_cpu.wr16(g_cpu.ds, D_dobj, 7);
				g_cpu.wr16(g_cpu.ds, D_iobj, 8);
				g_cpu.wr16(g_cpu.ds, D_current_room, 34);
				g_trace.clear();
				p.gen(ph, 0, 0);
				std::vector<std::string> a = g_trace;
				g_trace.clear();
				p.lift(ph, 0, 0);
				std::vector<std::string> b = g_trace;
				for (auto &s : a) s = norm(s);
				total++;
				if (a != b) {
					bad++;
					if (worst.size() < 40 && bad == 1) {
						std::string line = std::string(p.name) + " ph=" + std::to_string(ph) + " cmd=" + std::to_string(v) + "  gen:";
						for (size_t i = 0; i < a.size() && i < 6; i++) line += " " + a[i];
						line += "  lift:";
						for (size_t i = 0; i < b.size() && i < 6; i++) line += " " + b[i];
						worst.push_back(line);
					}
				}
			}
		scen_total += total; scen_bad += bad;
		if (bad) printf("DIFF %s\n", p.name);
		if (bad) differ++; else agree++;
	}
	printf("handlers compared: %d, agreeing in every scenario: %d, differing: %d; scenarios %d, differing %d\n", agree + differ, agree, differ, scen_total, scen_bad);
	for (auto &w : worst) printf("%s\n", w.c_str());
	return 0;
}
''')
print("pairs", len(both))
cmd = ["g++", "-std=c++17", "-O1", "-w", "-I", GEN, "-I", ALL, "-o", os.path.join(ALL, "trace_test.exe"), os.path.join(ALL, "test_main.cpp"), os.path.join(ALL, "lifted_handlers.cpp"),
       os.path.join(ALL, "mock_gen.cpp"), os.path.join(GEN, "handlers_gen.cpp")]
print("building...")
r = subprocess.run(cmd, capture_output=True, text=True)
if r.returncode:
    print(r.stderr[:3000])
    sys.exit(1)
out = subprocess.run([os.path.join(ALL, "trace_test.exe")], capture_output=True, text=True).stdout
diff = sorted(l.split()[1] for l in out.splitlines() if l.startswith("DIFF "))
mm = os.path.join(GEN, "handlers_mismatch.txt")
if os.path.exists(mm):
    diff = sorted(set(diff) | set(open(mm).read().split()))     # accumulates: delete the file to start over
open(mm, "w").write("\n".join(diff) + ("\n" if diff else ""))
print("\n".join(l for l in out.splitlines() if not l.startswith("DIFF ")))
print(len(diff), "handlers differ -> engine/gen/handlers_mismatch.txt (gen_handlers.py leaves them to the lifter)")

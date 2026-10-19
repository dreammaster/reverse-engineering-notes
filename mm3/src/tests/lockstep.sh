#!/bin/bash
# Lockstep comparison against the original code (tools/mm3_lockstep.py) for a few scripted scenarios and random key/mouse fuzz.
# usage (from mm3/src, after `make mm3game_tape`): tests/lockstep.sh [SCENARIO ...]
#   scenarios: explore combat intro title fuzzN mapN (walk around map N from 8,8)   (default: explore combat intro title fuzz1 fuzz2 fuzz3 fuzz4)
# Each scenario writes build/ls_NAME.{log,report,json} (and .tape with KEEP_TAPES=1); a divergence prints the report and the script exits 1.
keys=(4800 4800 4B00 4D00 5000 1051 2E43 1352 1F53 2F56 3920 1E41 3249 1769 1C0D 011B 3B00 3C00 3D00 3E00 3F00 4000 0231 0332 0433 0534 0635 2064 1E41 2146)
mkdir -p build
args_for() {
	case "$1" in
	explore) echo "--keys 4800,4800,4B00,4D00,5000,4800" ;;
	combat) local ks="4800,3920,3920"; for i in $(seq 1 40); do ks="$ks,1E41"; done; echo "--at 2,1,4,0 --keys $ks" ;;
	intro) echo "--intro --keys 3920" ;;
	title) echo "--title --keys 1C0D,1C0D,1C0D" ;;
	map*) local m=${1#map}; echo "--at $m,8,8,0 --keys 4800,4800,4B00,4800,4D00,4D00,4800,5000,4800,4B00,4800" ;;
	fuzz*) local seed=${1#fuzz}; RANDOM=$seed; local s=""
		for i in $(seq 1 40); do
			if [ $((RANDOM % 3)) -eq 0 ]; then s="$s,m$((RANDOM % 320)):$((RANDOM % 200))"; else s="$s,${keys[$((RANDOM % ${#keys[@]}))]}"; fi
		done; echo "--keys ${s:1}" ;;
	esac
}
run_one() {
	local name=$1 args; args=$(args_for "$name")
	MM3_TAPE=build/ls_$name.tape SDL_VIDEODRIVER=dummy timeout -s KILL 600 ./mm3game_tape ../data --headless $args >build/ls_$name.log 2>&1
	python3 ../tools/mm3_lockstep.py --stats build/ls_$name.json ${LOCKSTEP_OPTS} build/ls_$name.tape >build/ls_$name.report 2>&1
	local rc=$?
	if [ $rc -eq 0 ]; then echo "$name: no divergence ($(sed -n 2p build/ls_$name.report))"; else echo "$name: DIVERGENCE"; sed -n 1,14p build/ls_$name.report; fi
	[ -z "$KEEP_TAPES" ] && rm -f build/ls_$name.tape
	return $rc
}
scen=("$@"); [ ${#scen[@]} -eq 0 ] && scen=(explore combat intro title fuzz1 fuzz2 fuzz3 fuzz4)
fail=0; running=0
for s in "${scen[@]}"; do
	run_one "$s" & running=$((running + 1))
	if [ $running -ge 4 ]; then wait -n || fail=1; running=$((running - 1)); fi
done
while [ $running -gt 0 ]; do wait -n || fail=1; running=$((running - 1)); done
exit $fail

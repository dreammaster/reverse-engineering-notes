#!/bin/bash
# Call every translated routine (or the named ones) with mutated game state and random arguments, in the recompiled game and in the
# original code (tools/mm3_lockstep.py), and report where they differ.
# usage (from mm3/src, after `make mm3game_tape`): tests/calltest.sh [-n CALLS] [-s SEED] [NAME ...]
n=15; seed=1
while getopts "n:s:" o; do case $o in n) n=$OPTARG;; s) seed=$OPTARG;; esac; done
shift $((OPTIND - 1))
mkdir -p build/ct
names=("$@")
[ ${#names[@]} -eq 0 ] && names=($(grep -o "^static void fn_[A-Za-z0-9_]*" gen/game_gen.c | sed 's/static void fn_//' | sort -u))
one() {
	local f=$1
	MM3_TAPE=build/ct/$f.tape SDL_VIDEODRIVER=dummy timeout -s KILL 300 ./mm3game_tape ../data --headless --calltest $f $n $seed >build/ct/$f.log 2>&1
	python3 ../tools/mm3_lockstep.py --stats build/ct/$f.json build/ct/$f.tape >build/ct/$f.report 2>&1
	local rc=$?
	rm -f build/ct/$f.tape
	if [ $rc -eq 0 ]; then echo "ok        $f"; else echo "DIVERGES  $f: $(sed -n 3p build/ct/$f.report | cut -c1-230)"; fi
}
export -f one; export n seed
printf '%s\n' "${names[@]}" | xargs -P4 -I{} bash -c 'one {}' | sort

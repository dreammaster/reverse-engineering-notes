#!/bin/bash
# incremental build of host_test.exe (bash engine/host/build_obj.sh): objects in engine/host/obj (not committed), only changed sources are compiled
cd "$(dirname "$0")"
mkdir -p obj
F="-std=c++17 -O1 -w -DLIFT_STEP_LIMIT=3000000 -I gen -I . -I .."
for s in host.cpp host_impl.cpp host_machine.cpp host_test.cpp gen/lifted_handlers.cpp gen/lifted_table.cpp gen/lifted_names.cpp gen/host_defaults.cpp ../legend_formats.cpp; do
  o=obj/$(basename $s .cpp).o
  if [ ! -f $o ] || [ $s -nt $o ]; then echo "compile $s"; g++ $F -c $s -o $o || exit 1; fi
done
g++ -static -o host_test.exe obj/*.o && echo built

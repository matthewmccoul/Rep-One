#!/bin/bash
# Compile cpp/gen/*.cpp and the runtime into build/libmc.a (rebuilds what is older than its source or any header).
set -e
X=$(cd "$(dirname "$0")/.." && pwd); B=$X/build; mkdir -p $B
NEWEST=$(ls -t $X/cpp/*.h $X/cpp/gen/*.h | head -1)
ls $X/cpp/gen/*.cpp $X/cpp/cs_runtime.cpp | while read f; do o=$B/$(basename ${f%.cpp}).o; if [ ! -f $o ] || [ $f -nt $o ] || [ $NEWEST -nt $o ]; then echo $f; fi; done |
  xargs -r -P ${JOBS:-8} -I{} sh -c 'f={}; o='$B'/$(basename ${f%.cpp}).o; g++ -std=c++17 -O1 -w -I'$X'/cpp -I'$X'/cpp/gen -c $f -o $o 2> $o.err || { echo FAIL $f; head -5 $o.err; }'
rm -f $B/libmc.a; ar rcs $B/libmc.a $(ls $B/*.o | grep -v link_all)
echo built $(ls $B/*.o | wc -l) objects

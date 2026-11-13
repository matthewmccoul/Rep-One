#!/bin/bash
# usage: run_group.sh <group dir>  needs cases.json Case.cs case.cpp; optional renames.txt ("<src> <name> <newname>") and shim_<stem>.cpp (appended to src/<stem>.cpp)
set -u; G=$1; H=/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/harness; C=/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/prune/code
export PATH=/root/.dotnet:$PATH DOTNET_NOLOGO=1 DOTNET_CLI_TELEMETRY_OPTOUT=1 MORDOR_GAMEDIR=/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/work/game/shareware
rm -rf $G/cs && cp -r $H/cs_template $G/cs && cp $G/Case.cs $G/cs/Case.cs
( cd $G/cs && dotnet build -c Release -o out > $G/cs_build.log 2>&1 ) || { echo CS_BUILD_FAILED; exit 11; }
dotnet $G/cs/out/Harness.dll $G/cases.json > $G/core.txt 2> $G/core_err.log || { echo CS_RUN_FAILED; exit 12; }
: > $G/cpp_build.log; EXTRA=""; declare -A DEF; declare -A SRCS
if [ -f $G/renames.txt ]; then while read f n nn; do [ -z "$f" ] && continue; s=$(basename $f .cpp); DEF[$s]="${DEF[$s]:-} -D$n=$nn"; SRCS[$s]=$f; done < $G/renames.txt; fi
for sh in $G/shim_*.cpp; do [ -e "$sh" ] || continue; s=$(basename $sh .cpp); s=${s#shim_}; SRCS[$s]=src/$s.cpp; done
for s in "${!SRCS[@]}"; do f=${SRCS[$s]}; tu=$G/tu_$s.cpp; cp $C/$f $tu; [ -f $G/shim_$s.cpp ] && { printf '\n' >> $tu; cat $G/shim_$s.cpp >> $tu; }
  g++ -std=c++17 -O1 -w -I$C/include -I$C/src ${DEF[$s]:-} -c $tu -o $G/tu_$s.o >> $G/cpp_build.log 2>&1 || { echo CPP_BUILD_FAILED; exit 13; }; EXTRA="$EXTRA $G/tu_$s.o"; done
g++ -std=c++17 -O1 -w -I$C/include -I$H/cpp $H/cpp/main.cpp $G/case.cpp $EXTRA $C/b/libmordor_core.a -Wl,--allow-multiple-definition -o $G/cpp_h >> $G/cpp_build.log 2>&1 || { echo CPP_BUILD_FAILED; exit 13; }
$G/cpp_h $C/data/combat-data.json $G/cases.json > $G/cpp.txt 2> $G/cpp_err.log || { echo CPP_RUN_FAILED; exit 14; }
echo OK

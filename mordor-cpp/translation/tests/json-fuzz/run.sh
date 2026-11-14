#!/bin/bash
# usage: run.sh SEED N [NOSTRNULL]  -> generates cases, runs C# and C++ round trips, compares
X=/tmp/claude-0/-home-user-Rep-One/5abe2313-f548-5c8c-8816-6ae645168e4f/scratchpad/xl; M=/home/user/Rep-One/mordor-cpp/real-game/MORDOR; cd $X/json/fuzz
rm -rf cases out_cs out_cpp; NOSTRNULL=${3:-0} N=$2 python3 gen.py cases $1
(cd cs && ~/.dotnet/dotnet run -c Release --no-build -- $M ../cases ../out_cs > /dev/null 2>&1)
./rt $M cases out_cpp; python3 cmp.py out_cs out_cpp cases

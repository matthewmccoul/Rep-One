#!/bin/bash
# Run all header waves (conc kept modest: ants run on the user's account), then dedupe and fix.
cd /tmp/claude-0/-home-user-Rep-One/17a752bb-9e88-5884-a5bf-3b7fc4ab8c87/scratchpad/mordor/code
for w in 1 2 3 4; do echo "=== wave $w"; python3 -I tools/phase_headers.py $w 2>&1 | tail -4; python3 -I tools/dedupe_headers.py; done
python3 -I tools/fixloop.py headers 4 | tail -4

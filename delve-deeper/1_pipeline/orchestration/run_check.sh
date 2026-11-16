#!/usr/bin/env bash
# Run ONE command and print its output, then a last line EXIT=<code>. Exists because the agents' allowlists deny compound
# commands such as `python3 x.py; echo $?`. Usage: orchestration/run_check.sh <program> [args...]
# Only these programs may be run: python3 g++ gcc clang clang++ cmake make ninja ctest ls wc diff sha256sum,
# or a program (given with a path) that lives inside work/. The command is run directly, never through a shell.
set -uo pipefail
cd "$(dirname "$0")/.."
ROOT="$(pwd -P)"
if [[ $# -lt 1 ]]; then echo "usage: orchestration/run_check.sh <program> [args...]" >&2; echo "EXIT=2"; exit 0; fi
CMD="$1"; shift
case "$CMD" in
  python3|g++|gcc|clang|clang++|cmake|make|ninja|ctest|ls|wc|diff|sha256sum) ;;
  */*)
    real="$(realpath -- "$CMD" 2>/dev/null || true)"
    if [[ -z "$real" || ! -x "$real" || "$real" != "$ROOT/work/"* ]]; then
      echo "run_check.sh: refused: $CMD (a program given with a path must be an executable inside work/)" >&2; echo "EXIT=126"; exit 0
    fi ;;
  *) echo "run_check.sh: refused: $CMD is not on the list (python3 g++ gcc clang clang++ cmake make ninja ctest ls wc diff sha256sum, or a program inside work/)" >&2
     echo "EXIT=126"; exit 0 ;;
esac
timeout --kill-after=10 "${DD_CHECK_TIMEOUT_S:-900}" "$CMD" "$@"
rc=$?
echo "EXIT=$rc"
exit 0

#!/usr/bin/env python3
"""Run one command and keep a tool-written log of it (standard library only).

Usage: python3 -I work/T001/qa/run_check.py <check-id> -- <command> [args ...]
Runs the command (no shell), waits up to 300 seconds, and writes work/T001/qa/out/<check-id>.log with the command
line, the exit code and everything the command printed. Prints "RUN <id> exit=<n> log=<path>" and the first 40
lines of output. Its own exit code is the command's exit code (124 on timeout, 127 if the command was not found),
so the caller compares it with the expected value. It judges nothing.
This is a logging helper written by the Producer; it is not one of the checks.
"""
import os, subprocess, sys

def main(argv):
    if len(argv) < 4 or argv[2] != "--":
        print(__doc__, file=sys.stderr)
        return 2
    cid, cmd = argv[1], argv[3:]
    if not cid.replace("-", "").replace("_", "").isalnum():
        print("bad check id", file=sys.stderr)
        return 2
    out_dir = os.path.join("work", "T001", "qa", "out")
    os.makedirs(out_dir, exist_ok=True)
    log = os.path.join(out_dir, cid + ".log")
    try:
        r = subprocess.run(cmd, capture_output=True, text=True, timeout=300)
        code, text = r.returncode, (r.stdout or "") + (("[stderr]\n" + r.stderr) if r.stderr else "")
    except subprocess.TimeoutExpired as e:
        code, text = 124, "timeout after 300 seconds\n" + ((e.stdout or b"").decode(errors="replace") if isinstance(e.stdout, bytes) else (e.stdout or ""))
    except FileNotFoundError:
        code, text = 127, "command not found: " + cmd[0] + "\n"
    with open(log, "w", encoding="utf-8") as f:
        f.write("$ " + " ".join(cmd) + "\nexit: " + str(code) + "\n--- output ---\n" + text)
    print(f"RUN {cid} exit={code} log={log}")
    lines = text.splitlines()
    print("\n".join(lines[:40]))
    if len(lines) > 40:
        print(f"... ({len(lines) - 40} more lines in the log)")
    return code

if __name__ == "__main__":
    sys.exit(main(sys.argv))

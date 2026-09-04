"""Adapt the official Rapfi pbrain executable to the assignment's static input.

This is an offline analysis/oracle tool, not a submission: the official engine
and its NNUE weights remain external. Set RAPFI_PBRAIN to select another build.
"""
from __future__ import annotations

import os
from pathlib import Path
import re
import subprocess
import sys


ROOT = Path(__file__).resolve().parent
DEFAULT_ENGINE = (ROOT / "upstream-rapfi-release" / "extracted" /
                  "pbrain-rapfi-windows-avx2.exe")


def main() -> int:
    words = sys.stdin.read().split()
    if len(words) != 226:
        return 2
    side = int(words[0])
    cells = [int(value) for value in words[1:]]
    engine = Path(os.environ.get("RAPFI_PBRAIN", DEFAULT_ENGINE)).resolve()
    timeout_ms = int(os.environ.get("RAPFI_ORACLE_MS", "5000"))
    if not engine.is_file():
        print(f"missing Rapfi pbrain: {engine}", file=sys.stderr)
        return 2

    commands = ["START 15", f"INFO timeout_turn {timeout_ms}", "INFO rule 1", "BOARD"]
    stones = {0: [], 1: []}
    for row in range(15):
        for col in range(15):
            stone = cells[row * 15 + col]
            if stone != -1:
                stones[stone].append((row, col))
    # BOARD is a move sequence, not an unordered position dump. Reconstruct any
    # alternating order consistent with the static board; Rapfi's evaluation is
    # position based, while this also lets it infer the engine's actual colour.
    for index in range(max(map(len, stones.values()))):
        for color in (0, 1):
            if index < len(stones[color]):
                row, col = stones[color][index]
                # Piskvork BOARD uses 1 for the engine and 2 for its opponent.
                owner = 1 if color == side else 2
                commands.append(f"{col},{row},{owner}")
    commands.extend(("DONE", "END"))
    proc = subprocess.run(
        [str(engine)], input="\n".join(commands) + "\n", text=True,
        capture_output=True, cwd=engine.parent, timeout=timeout_ms / 1000 + 8,
    )
    matches = re.findall(r"(?m)^(\d+),(\d+)\s*$", proc.stdout)
    if not matches:
        print(proc.stdout[-2000:], file=sys.stderr)
        print(proc.stderr[-2000:], file=sys.stderr)
        return 1
    col, row = map(int, matches[-1])
    print(row, col)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

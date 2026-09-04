#!/usr/bin/env python3
"""Replay a finished game and report where the engine's own evaluation collapsed.

The site's console shows the board handed to the program and the move it
answered with on every turn.  Feed that sequence back here and the engine
re-searches each of our positions, so a loss stops being "we lost somehow" and
becomes a specific ply where the score fell off and a specific move it would
rather have played given more time.
"""
from __future__ import annotations

import argparse
import io
import json
import re
import subprocess
import sys
from pathlib import Path

from arena_server import black_forbidden, winner
from gomoku_match import compile_source

ROOT = Path(__file__).resolve().parent
SIZE = 15
# The root value lives in a local of fullSearch, so the analysis build copies it
# into a global on the way out.  Only this scratch build prints it; src.cpp is
# untouched and still writes nothing but the move.
PATCH_FROM = "\treturn bestMove.pos;"
PATCH_TO = ("\tstd::fprintf(stderr, \"value=%d depth=%d\\n\", bestMove.value, searchDepth);\n"
            "\treturn bestMove.pos;")


def analysis_binary() -> Path:
    source = io.open(ROOT / "src.cpp", encoding="latin-1").read()
    if source.count(PATCH_FROM) != 1:
        raise RuntimeError("fullSearch return point moved; update PATCH_FROM")
    patched = ROOT / ".build-book" / "analysis.cpp"
    patched.parent.mkdir(exist_ok=True)
    io.open(patched, "w", encoding="latin-1").write(source.replace(PATCH_FROM, PATCH_TO, 1))
    return compile_source(patched)


def ask(binary: Path, board, side, budget_ms):
    import os
    text = f"{side}\n" + "\n".join(" ".join(str(v) for v in row) for row in board) + "\n"
    proc = subprocess.run([str(binary)], input=text, capture_output=True, text=True,
                          env=dict(os.environ, GOMOKU_DEADLINE_MS=str(budget_ms)), timeout=120)
    move = tuple(int(v) for v in proc.stdout.split()[:2])
    found = re.findall(r"value=(-?\d+) depth=(\d+)", proc.stderr)
    if not found:
        return move, None, None          # answered from the opening book, no search
    value, depth = int(found[-1][0]), int(found[-1][1])
    # fullSearch leaves bestMove.value untouched when it bails out of a hopeless
    # position, so anything past the mate range is a sentinel, not a score.
    if abs(value) > 30000:
        value = -30000 if value < 0 else 30000
    return move, value, depth


def parse_moves(text: str) -> list[tuple[int, int]]:
    """Accept a JSON list, or any text with one 'row col' pair per line."""
    text = text.strip()
    if text.startswith("["):
        return [tuple(m) for m in json.loads(text)]
    moves = []
    for line in text.splitlines():
        nums = re.findall(r"-?\d+", line)
        if len(nums) >= 2:
            moves.append((int(nums[-2]), int(nums[-1])))
    return moves


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("moves", help="file with the move sequence, or - for stdin")
    ap.add_argument("--side", type=int, required=True, choices=(0, 1), help="0 if we were black")
    ap.add_argument("--budget-ms", type=int, default=820, help="contest budget to reproduce")
    ap.add_argument("--long-ms", type=int, default=10000, help="budget for the second opinion")
    args = ap.parse_args()

    text = sys.stdin.read() if args.moves == "-" else Path(args.moves).read_text()
    moves = parse_moves(text)
    binary = analysis_binary()
    print(f"{len(moves)} plies, we played {'black' if args.side == 0 else 'white'}\n")

    board = [[-1] * SIZE for _ in range(SIZE)]
    header = f"{'ply':>4} {'side':>5} {'played':>8} {'value':>7} {'depth':>5}  {'长考':>8}  note"
    print(header)
    print("-" * len(header))
    for ply, (r, c) in enumerate(moves):
        side = ply & 1
        if side != args.side:
            board[r][c] = side
            continue
        move, value, depth = ask(binary, board, side, args.budget_ms)
        long_move, long_value, _ = ask(binary, board, side, args.long_ms)
        note = []
        if move != (r, c):
            note.append(f"实战走了 {r},{c}")
        if long_move != move:
            note.append("长考改变着法")
        if value is None:
            shown, note_extra = "书", "开局库直出"
            note.insert(0, note_extra)
        elif value <= -29000:
            shown = "必败"
            note.append("引擎已判负")
        elif value >= 29000:
            shown = "必胜"
        else:
            shown = str(value)
        print(f"{ply:>4} {'黑' if side == 0 else '白':>5} {str(move):>8} {shown:>7} "
              f"{str(depth) if depth else '-':>5}  {str(long_move):>8}  {'; '.join(note)}")
        board[r][c] = side
        if side == 0 and black_forbidden(board, r, c):
            print(f"     ^ 实战这一手是禁手: {black_forbidden(board, r, c)}")
        if winner(board) != -1:
            print(f"\n{'黑' if winner(board) == 0 else '白'}方在第 {ply} 手成五")
            break
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

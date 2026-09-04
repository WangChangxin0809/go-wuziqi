#!/usr/bin/env python3
"""Check every forced win the engine announces against the rules themselves.

Two of these claims have already turned out to be false, both for the same
reason: Rapfi's tables come from freestyle, where five or more wins, so a gap
whose fill would make six still counts as a four.  A false mate is expensive
beyond the wrong move — the iteration stops the moment the root reports one, so
the engine also throws away the rest of its second.

Finding those one game at a time is luck.  This replays saved games, asks the
engine about every position in them, and for each announced mate runs an
exhaustive check that follows only the written rules: the attacker may try
anything, the defender may try everything.
"""
from __future__ import annotations

import argparse
import glob
import json
import os
import re
import subprocess
from pathlib import Path

from arena_server import black_forbidden, winner
from gomoku_match import compile_source

ROOT = Path(__file__).resolve().parent
SIZE = 15
STATS = re.compile(r"depth=(-?\d+) value=(-?\d+) node=(\d+) expanded=(\d+)")
WIN_MAX, WIN_MIN = 30000, 29000


def legal(board, r, c, side) -> bool:
    if board[r][c] != -1:
        return False
    if side == 1:
        return True
    board[r][c] = 0
    banned = black_forbidden(board, r, c)
    board[r][c] = -1
    return banned is None


def wins_now(board, r, c, side) -> bool:
    """A legal move that ends the game on the spot."""
    if not legal(board, r, c, side):
        return False
    board[r][c] = side
    won = winner(board) == side
    board[r][c] = -1
    return won


def five_points(board, side) -> list[tuple[int, int]]:
    return [(r, c) for r in range(SIZE) for c in range(SIZE) if wins_now(board, r, c, side)]


def candidates(board) -> list[tuple[int, int]]:
    """Empty points within two of a stone.  A move further out cannot join
    anything, so leaving them out does not let a defence escape the net."""
    out = []
    for r in range(SIZE):
        for c in range(SIZE):
            if board[r][c] != -1:
                continue
            near = any(board[r + dr][c + dc] != -1
                       for dr in range(-2, 3) for dc in range(-2, 3)
                       if 0 <= r + dr < SIZE and 0 <= c + dc < SIZE)
            if near:
                out.append((r, c))
    return out


def forced_win(board, side, plies: int, cands=None) -> bool:
    """Can `side` force a win within `plies`?  Exhaustive over both sides.

    The defender is given every candidate reply, so a False here is a real
    refutation of the claim and not an artefact of a narrow search.
    """
    if plies <= 0:
        return False
    if five_points(board, side):
        return True
    if plies < 3:
        return False
    cands = cands or candidates(board)
    other = side ^ 1
    for r, c in cands:
        if not legal(board, r, c, side):
            continue
        board[r][c] = side
        # the defender moves next, so an immediate five of its own ends it there
        beaten = bool(five_points(board, other))
        if not beaten:
            replies = [p for p in candidates(board) if legal(board, *p, other)]
            if not replies:
                board[r][c] = -1
                return True                       # nothing legal left to answer with
            if all(forced_win(board_after(board, p, other), side, plies - 2)
                   for p in replies):
                board[r][c] = -1
                return True
        board[r][c] = -1
    return False


def board_after(board, move, side):
    r, c = move
    board[r][c] = side
    copy = [row[:] for row in board]
    board[r][c] = -1
    return copy


def ask(binary: Path, board, side: int, budget_ms: int):
    text = f"{side}\n" + "\n".join(" ".join(str(v) for v in row) for row in board) + "\n"
    proc = subprocess.run([str(binary)], input=text, capture_output=True, text=True,
                          env=dict(os.environ, GOMOKU_DEADLINE_MS=str(budget_ms),
                                   GOMOKU_STATS="1"), timeout=budget_ms / 1000 + 30)
    found = STATS.findall(proc.stderr)
    if not found:
        return None
    depth, value, node, expanded = (int(v) for v in found[-1])
    move = tuple(int(v) for v in proc.stdout.split()[:2])
    return {"move": move, "value": value, "depth": depth, "node": node}


def positions_from(paths, stride: int):
    """Every position in the saved games, thinned by `stride`."""
    for path in paths:
        try:
            data = json.loads(Path(path).read_text())
        except Exception:
            continue
        games = data.get("games") or [data]
        for game in games:
            raw = game.get("moves") or []
            moves = [m["move"] if isinstance(m, dict) else m for m in raw]
            board = [[-1] * SIZE for _ in range(SIZE)]
            for ply, mv in enumerate(moves):
                if not (isinstance(mv, (list, tuple)) and len(mv) == 2):
                    break
                if ply % stride == 0 and ply >= 6:
                    yield [row[:] for row in board], ply & 1, f"{Path(path).name}#{ply}"
                board[mv[0]][mv[1]] = ply & 1


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("games", nargs="+", help="game JSON files or globs")
    ap.add_argument("--engine", default="src.cpp")
    ap.add_argument("--budget-ms", type=int, default=400)
    ap.add_argument("--stride", type=int, default=4)
    ap.add_argument("--limit", type=int, default=400)
    ap.add_argument("--json", dest="json_path")
    args = ap.parse_args()

    paths = [p for pattern in args.games for p in sorted(glob.glob(pattern))]
    binary = (compile_source(ROOT / args.engine) if args.engine.endswith(".cpp")
              else Path(args.engine))

    checked = claims = false_claims = 0
    findings = []
    for board, side, label in positions_from(paths, args.stride):
        if checked >= args.limit:
            break
        checked += 1
        report = ask(binary, board, side, args.budget_ms)
        if not report or abs(report["value"]) < WIN_MIN:
            continue
        claims += 1
        if report["value"] < 0:
            continue                              # a claimed loss needs the other proof
        # WIN_MAX - k is a mate k plies away; give the check one ply of slack
        plies = max(1, WIN_MAX - report["value"] + 1)
        if plies > 5:
            continue                              # too deep to settle exhaustively
        if not forced_win([row[:] for row in board], side, plies):
            false_claims += 1
            findings.append({"label": label, "side": side, "value": report["value"],
                             "depth": report["depth"], "node": report["node"],
                             "plies_checked": plies, "move": list(report["move"]),
                             "board": board})
            print(f"  假必胜 {label}: 宣称 {report['value']} (depth {report['depth']}, "
                  f"{report['node']} 节点), {plies} 手内并无必胜")
    print(f"\n查了 {checked} 个局面, 其中 {claims} 个宣称杀棋, {false_claims} 个是假的")
    if args.json_path and findings:
        Path(args.json_path).write_text(json.dumps(findings, indent=1), encoding="utf-8")
        print(f"wrote {args.json_path}")
    return 1 if false_claims else 0


if __name__ == "__main__":
    raise SystemExit(main())

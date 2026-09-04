#!/usr/bin/env python3
"""Play the real online opponent while forcing our own move at chosen plies.

Losing the same way every time is only useful if the loss can be pushed
somewhere else.  The engine already accepts GOMOKU_EXCLUDE, so a ply can be
replayed with its best move struck out and the second choice played instead;
running that against the live opponent says whether the line was actually
refuted or just badly chosen.
"""
from __future__ import annotations

import argparse
import json
import os
import subprocess
import time
from pathlib import Path

from arena_server import black_forbidden, winner
from gomoku_match import compile_source
from gomoku_site_api import SiteClient, DEFAULT_COOKIE_FILE, DEFAULT_STATE, resolve_cookie

ROOT = Path(__file__).resolve().parent
SIZE = 15


def board_text(side: int, board) -> str:
    return f"{side}\n" + "\n".join(" ".join(str(v) for v in row) for row in board) + "\n"


def local_move(binary: Path, side: int, board, budget_ms: int, exclude: list[tuple[int, int]]):
    env = dict(os.environ, GOMOKU_DEADLINE_MS=str(budget_ms))
    if exclude:
        env["GOMOKU_EXCLUDE"] = ";".join(f"{r},{c}" for r, c in exclude)
    out = subprocess.run([str(binary)], input=board_text(side, board), env=env,
                         capture_output=True, text=True, timeout=budget_ms / 1000 + 30).stdout.split()
    return int(out[0]), int(out[1])


def remote_move(client: SiteClient, uid: str, side: int, board):
    reply = client.execute(uid, board_text(side, board))
    if reply.get("result", {}).get("status") != 1:
        raise RuntimeError(f"remote status {reply.get('result')}")
    values = reply["output"].split()
    return int(values[0]), int(values[1])


def play(binary, client, uid, our_side, budget_ms, forks, max_plies):
    board = [[-1] * SIZE for _ in range(SIZE)]
    moves, notes = [], []
    for ply in range(max_plies):
        side = ply & 1
        if side == our_side:
            exclude = []
            for _ in range(forks.get(ply, 0)):
                exclude.append(local_move(binary, side, board, budget_ms, exclude))
            move = local_move(binary, side, board, budget_ms, exclude)
            if exclude:
                notes.append(f"ply {ply}: skipped {exclude} and played {move}")
        else:
            move = remote_move(client, uid, side, board)
        r, c = move
        if not (0 <= r < SIZE and 0 <= c < SIZE) or board[r][c] != -1:
            return {"winner": side ^ 1, "reason": "Invalid", "moves": moves, "notes": notes}
        board[r][c] = side
        moves.append([r, c])
        if side == 0 and black_forbidden(board, r, c):
            return {"winner": 1, "reason": black_forbidden(board, r, c), "moves": moves, "notes": notes}
        if winner(board) == side:
            return {"winner": side, "reason": "Five", "moves": moves, "notes": notes}
    return {"winner": -1, "reason": "Draw", "moves": moves, "notes": notes}


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--uid", required=True)
    ap.add_argument("--side", choices=("black", "white"), required=True)
    ap.add_argument("--engine", default="src.cpp")
    ap.add_argument("--budget-ms", type=int, default=15000)
    ap.add_argument("--max-plies", type=int, default=225)
    ap.add_argument("--fork", default="", help="ply:rank pairs, e.g. 7:1,9:2 to take the "
                                               "second choice at ply 7 and the third at ply 9")
    ap.add_argument("--json", dest="json_path")
    args = ap.parse_args()

    forks = {}
    for chunk in args.fork.split(","):
        if chunk.strip():
            ply, rank = chunk.split(":")
            forks[int(ply)] = int(rank)

    binary = compile_source(ROOT / args.engine)
    client = SiteClient(resolve_cookie(None, DEFAULT_COOKIE_FILE, DEFAULT_STATE))
    our_side = 0 if args.side == "black" else 1
    started = time.time()
    result = play(binary, client, args.uid, our_side, args.budget_ms, forks, args.max_plies)
    result.update({"uid": args.uid, "side": args.side, "budget_ms": args.budget_ms,
                   "fork": args.fork, "seconds": round(time.time() - started, 1)})
    verdict = "赢" if result["winner"] == our_side else ("平" if result["winner"] == -1 else "输")
    print(f"fork={args.fork or 'none'}  {verdict}  {result['reason']}  {len(result['moves'])} plies  "
          f"{result['seconds']}s")
    for note in result["notes"]:
        print("   " + note)
    if args.json_path:
        Path(args.json_path).write_text(json.dumps(result, indent=1), encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

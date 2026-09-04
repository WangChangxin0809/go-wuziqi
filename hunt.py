#!/usr/bin/env python3
"""Search for a line that beats one deterministic online opponent.

The strongest program on the board answers every position with the same move in
the same time, three probes in a row.  That turns "we always lose as white" from
a playing-strength problem into a search problem: the opponent is an oracle, and
somewhere in the tree of our own choices there may be a path it does not answer.

Both sides are cached by position, so a prefix that has already been walked
costs nothing to walk again and the search can be stopped and resumed.  Our own
moves are enumerated through GOMOKU_EXCLUDE: excluding the moves already tried
makes the engine hand back its next preference.
"""
from __future__ import annotations

import argparse
import json
import time
from pathlib import Path

from arena_server import black_forbidden, winner
from gomoku_match import compile_source
from gomoku_site_api import SiteClient, DEFAULT_COOKIE_FILE, DEFAULT_STATE, resolve_cookie
from refute import local_move, remote_move

ROOT = Path(__file__).resolve().parent
SIZE = 15


def key(side: int, board) -> str:
    return str(side) + "".join("." if v < 0 else str(v) for row in board for v in row)


class Cache:
    """Both oracles keyed by position, kept on disk so a run can be resumed."""

    def __init__(self, path: Path):
        self.path = path
        self.data = json.loads(path.read_text()) if path.exists() else {"remote": {}, "local": {}}
        self.dirty = 0

    def save(self) -> None:
        self.path.write_text(json.dumps(self.data), encoding="utf-8")
        self.dirty = 0

    def touch(self) -> None:
        self.dirty += 1
        if self.dirty >= 8:
            self.save()


class Hunt:
    def __init__(self, args, binary: Path, client: SiteClient, cache: Cache):
        self.args, self.binary, self.client, self.cache = args, binary, client, cache
        self.our_side = 0 if args.side == "black" else 1
        self.remote_calls = self.local_calls = 0
        self.leaves: list[dict] = []
        self.deadline = time.time() + args.minutes * 60

    def remote(self, side: int, board):
        k = key(side, board)
        hit = self.cache.data["remote"].get(k)
        if hit is None:
            hit = list(remote_move(self.client, self.args.uid, side, board))
            self.cache.data["remote"][k] = hit
            self.cache.touch()
            self.remote_calls += 1
        return tuple(hit)

    def ours(self, side: int, board, want: int):
        """Our engine's top `want` moves for this position, in its own order."""
        k = key(side, board)
        ranked = [tuple(m) for m in self.cache.data["local"].get(k, [])]
        while len(ranked) < want:
            move = local_move(self.binary, side, board, self.args.budget_ms, list(ranked))
            self.local_calls += 1
            if move in ranked:            # the engine ran out of distinct choices
                break
            ranked.append(move)
            self.cache.data["local"][k] = [list(m) for m in ranked]
            self.cache.touch()
        return ranked[:want]

    def step(self, board, side: int, move):
        """Apply a move and report a terminal verdict, or None to keep playing."""
        r, c = move
        if not (0 <= r < SIZE and 0 <= c < SIZE) or board[r][c] != -1:
            return side ^ 1, "Invalid"
        board[r][c] = side
        if side == 0 and black_forbidden(board, r, c):
            return 1, black_forbidden(board, r, c)
        if winner(board) == side:
            return side, "Five"
        return None

    def explore(self, board, ply: int, budget: int, path: list):
        """Depth-first over our own choices; `budget` counts remaining deviations."""
        if ply >= self.args.max_plies or time.time() > self.deadline:
            return None
        side = ply & 1
        if side != self.our_side:
            move = self.remote(side, board)
            verdict = self.step(board, side, move)
            path.append(list(move))
            result = verdict if verdict else self.explore(board, ply + 1, budget, path)
            board[move[0]][move[1]] = -1
            path.pop()
            if verdict:
                self.leaves.append({"winner": verdict[0], "reason": verdict[1],
                                    "moves": [list(m) for m in path]})
            return result

        width = self.args.branch if budget > 0 else 1
        for rank, move in enumerate(self.ours(side, board, width)):
            verdict = self.step(board, side, move)
            path.append(list(move))
            if verdict:
                self.leaves.append({"winner": verdict[0], "reason": verdict[1],
                                    "moves": [list(m) for m in path]})
                result = verdict
            else:
                result = self.explore(board, ply + 1, budget - (1 if rank else 0), path)
            board[move[0]][move[1]] = -1
            path.pop()
            if result and result[0] == self.our_side:
                return result                      # a won line: stop and report it
            if time.time() > self.deadline:
                return None
        return None


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--uid", required=True)
    ap.add_argument("--side", choices=("black", "white"), required=True)
    ap.add_argument("--engine", default="src.cpp")
    ap.add_argument("--budget-ms", type=int, default=3000)
    ap.add_argument("--branch", type=int, default=3, help="candidates to try at a deviation")
    ap.add_argument("--deviations", type=int, default=2, help="deepest search, tried in order")
    ap.add_argument("--max-plies", type=int, default=90)
    ap.add_argument("--minutes", type=float, default=60.0)
    ap.add_argument("--cache", default=None)
    ap.add_argument("--json", dest="json_path", default=None)
    args = ap.parse_args()

    binary = compile_source(ROOT / args.engine) if args.engine.endswith(".cpp") else Path(args.engine)
    client = SiteClient(resolve_cookie(None, DEFAULT_COOKIE_FILE, DEFAULT_STATE))
    cache = Cache(Path(args.cache or f"cache-{args.uid}-{args.side}.json"))
    hunt = Hunt(args, binary, client, cache)

    found = None
    for budget in range(args.deviations + 1):
        started = time.time()
        found = hunt.explore([[-1] * SIZE for _ in range(SIZE)], 0, budget, [])
        won = sum(1 for leaf in hunt.leaves if leaf["winner"] == hunt.our_side)
        print(f"deviations<={budget}: {len(hunt.leaves)} lines, {won} won, "
              f"{hunt.remote_calls} remote / {hunt.local_calls} local calls, "
              f"{time.time() - started:.0f}s")
        if found and found[0] == hunt.our_side:
            break
        if time.time() > hunt.deadline:
            print("时间用完")
            break
    cache.save()

    winning = [leaf for leaf in hunt.leaves if leaf["winner"] == hunt.our_side]
    if winning:
        best = min(winning, key=lambda leaf: len(leaf["moves"]))
        print(f"\n找到取胜线路（{len(best['moves'])} 手）: {best['moves']}")
    else:
        print("\n没有找到取胜线路")
    if args.json_path:
        Path(args.json_path).write_text(json.dumps(
            {"uid": args.uid, "side": args.side, "budget_ms": args.budget_ms,
             "branch": args.branch, "leaves": hunt.leaves, "winning": winning}, indent=1),
            encoding="utf-8")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

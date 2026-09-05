"""Offline opening-book tooling for the BetaGomoku assignment.

The judge restarts the engine for every move and gives each process its own one
second, so search depth in the opening cannot be bought at run time.  It can be
bought offline: this script drives the same engine with GOMOKU_DEADLINE_MS set
to seconds instead of milliseconds, and records what it plays.

Two modes:

  probe  walk self-play lines and report how often the long search disagrees
         with the contest-time search.  A book is only worth building if that
         disagreement rate is meaningfully above zero.
  build  expand the opening tree and write a book of positions where the long
         search knows better, keyed by the board's canonical form under the
         eight board symmetries.
"""

from __future__ import annotations

import argparse
import hashlib
import json
import os
import subprocess
import sys
from concurrent.futures import ProcessPoolExecutor
from pathlib import Path

ROOT = Path(__file__).resolve().parent
BUILD_DIR = ROOT / ".build-book"
SIZE = 15
EMPTY, BLACK, WHITE = -1, 0, 1

# The eight symmetries of the square board, as coordinate maps, paired with the
# index of the inverse map.  A book entry is stored in canonical orientation and
# rotated back when it is read.
TRANSFORMS = [
    lambda r, c: (r, c),
    lambda r, c: (c, SIZE - 1 - r),
    lambda r, c: (SIZE - 1 - r, SIZE - 1 - c),
    lambda r, c: (SIZE - 1 - c, r),
    lambda r, c: (r, SIZE - 1 - c),
    lambda r, c: (SIZE - 1 - r, c),
    lambda r, c: (c, r),
    lambda r, c: (SIZE - 1 - c, SIZE - 1 - r),
]
INVERSE = [0, 3, 2, 1, 4, 5, 6, 7]


def empty_board() -> list[list[int]]:
    return [[EMPTY] * SIZE for _ in range(SIZE)]


def apply_transform(board: list[list[int]], index: int) -> list[list[int]]:
    mapper = TRANSFORMS[index]
    out = empty_board()
    for r in range(SIZE):
        for c in range(SIZE):
            nr, nc = mapper(r, c)
            out[nr][nc] = board[r][c]
    return out


def board_key(board: list[list[int]]) -> str:
    return "".join(".bw"[v + 1] for row in board for v in row)


def canonical(board: list[list[int]]) -> tuple[str, int]:
    """Return the lexicographically smallest orientation and the transform used."""
    best_key, best_index = None, 0
    for index in range(len(TRANSFORMS)):
        key = board_key(apply_transform(board, index))
        if best_key is None or key < best_key:
            best_key, best_index = key, index
    return best_key, best_index


def side_to_move(board: list[list[int]]) -> int:
    stones = [0, 0]
    for row in board:
        for v in row:
            if v in (BLACK, WHITE):
                stones[v] += 1
    return BLACK if stones[BLACK] == stones[WHITE] else WHITE


def board_input(board: list[list[int]], side: int) -> str:
    rows = [" ".join(str(v) for v in row) for row in board]
    return f"{side}\n" + "\n".join(rows) + "\n"


def resolve_engine(path: Path) -> Path:
    """Accept either a source file or an already-built binary."""
    if path.suffix.lower() in (".out", ".exe", ".bin") or path.suffix == "":
        return path
    return compile_engine(path)


def compile_engine(source: Path) -> Path:
    BUILD_DIR.mkdir(exist_ok=True)
    digest = hashlib.sha256(source.read_bytes()).hexdigest()[:12]
    binary = BUILD_DIR / f"{source.parent.name}-{digest}"
    if binary.exists() and binary.stat().st_mtime_ns >= source.stat().st_mtime_ns:
        return binary
    # Two builds started at once would otherwise write the same file while the
    # other one is trying to exec it, which fails as "Text file busy".  Compile
    # to a private name and move it into place, which is atomic.
    staging = binary.with_name(binary.name + f".{os.getpid()}.tmp")
    cmd = ["g++", str(source), "-O2", "-o", str(staging), "-std=c++17", "-Wall"]
    proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=180)
    if proc.returncode:
        staging.unlink(missing_ok=True)
        raise RuntimeError(f"compile failed: {source}\n{proc.stdout}{proc.stderr}")
    os.replace(staging, binary)
    return binary


def ask(binary: Path, board: list[list[int]], deadline_ms: int,
        exclude: list[tuple[int, int]] | None = None) -> tuple[int, int]:
    env = dict(os.environ)
    env["GOMOKU_DEADLINE_MS"] = str(deadline_ms)
    if exclude:
        env["GOMOKU_EXCLUDE"] = ";".join(f"{r},{c}" for r, c in exclude)
    else:
        env.pop("GOMOKU_EXCLUDE", None)
    side = side_to_move(board)
    proc = subprocess.run([str(binary)], input=board_input(board, side), env=env,
                          capture_output=True, text=True,
                          timeout=deadline_ms / 1000.0 + 30.0)
    parts = proc.stdout.split()
    if len(parts) < 2:
        raise RuntimeError(f"engine produced no move: {proc.stdout!r} {proc.stderr!r}")
    row, col = int(parts[0]), int(parts[1])
    if not (0 <= row < SIZE and 0 <= col < SIZE and board[row][col] == EMPTY):
        raise RuntimeError(f"engine produced an occupied or off-board point: {row} {col}")
    return row, col


def top_moves(binary: Path, board: list[list[int]], deadline_ms: int, count: int
              ) -> list[tuple[int, int]]:
    moves: list[tuple[int, int]] = []
    while len(moves) < count:
        try:
            move = ask(binary, board, deadline_ms, exclude=moves)
        except RuntimeError:
            break
        if move in moves:
            break
        moves.append(move)
    return moves


def cmd_probe(args: argparse.Namespace) -> int:
    binary = resolve_engine(Path(args.engine))
    board = empty_board()
    disagreements = []
    for ply in range(args.plies):
        side = side_to_move(board)
        short = ask(binary, board, args.short_ms)
        long_move = ask(binary, board, args.long_ms)
        same = short == long_move
        marker = "same" if same else "DIFFERS"
        print(f"ply {ply:2d} {'black' if side == BLACK else 'white'}  "
              f"{args.short_ms}ms -> {short}   {args.long_ms}ms -> {long_move}   {marker}",
              flush=True)
        if not same:
            disagreements.append({"ply": ply, "short": short, "long": long_move,
                                  "board": board_key(board)})
        board[long_move[0]][long_move[1]] = side
    rate = len(disagreements) / max(1, args.plies)
    print(f"\ndisagreement rate along this line: {len(disagreements)}/{args.plies} = {rate:.0%}")
    if args.json_path:
        Path(args.json_path).write_text(json.dumps(
            {"engine": args.engine, "short_ms": args.short_ms, "long_ms": args.long_ms,
             "plies": args.plies, "disagreements": disagreements}, indent=1), encoding="utf-8")
    return 0


def _walk_shard(payload: dict) -> dict:
    """Explore one subtree in a worker process and return what it found.

    Each shard keeps its own transposition set, so subtrees that reach a common
    position explore it more than once.  That costs some duplicated search but
    keeps the workers independent, which matters more: every query is a wall
    clock search, so a worker that blocks on a lock is a worker whose engine is
    handing its time slice to somebody else.
    """
    binary = Path(payload["binary"])
    board = payload["board"]
    ply = payload["ply"]
    opt = payload["options"]
    our_side = opt["our_side"]
    book: dict[str, list[int]] = {}
    seen: set[str] = set()
    stats = {"nodes": 0, "queries": 0, "stored": 0, "agreed": 0}

    def walk(board: list[list[int]], ply: int) -> None:
        if ply >= opt["plies"]:
            return
        key, index = canonical(board)
        if key in seen:
            return
        seen.add(key)
        stats["nodes"] += 1
        side = side_to_move(board)

        if side == our_side:
            long_move = ask(binary, board, opt["long_ms"])
            stats["queries"] += 1
            store = True
            if opt["only_corrections"]:
                short = ask(binary, board, opt["short_ms"])
                stats["queries"] += 1
                store = short != long_move
                if not store:
                    stats["agreed"] += 1
            if store:
                cr, cc = TRANSFORMS[index](*long_move)
                book[key] = [cr, cc]
                stats["stored"] += 1
            board[long_move[0]][long_move[1]] = side
            walk(board, ply + 1)
            board[long_move[0]][long_move[1]] = EMPTY
        else:
            for move in top_moves(binary, board, opt["reply_ms"], opt["branch"]):
                stats["queries"] += 1
                board[move[0]][move[1]] = side
                walk(board, ply + 1)
                board[move[0]][move[1]] = EMPTY

    walk(board, ply)
    return {"book": book, "stats": stats}


def cmd_build(args: argparse.Namespace) -> int:
    binary = resolve_engine(Path(args.engine))
    our_side = BLACK if args.side == "black" else WHITE
    book: dict[str, list[int]] = {}
    seen: set[str] = set()
    stats = {"nodes": 0, "queries": 0, "stored": 0, "agreed": 0}
    options = {"our_side": our_side, "plies": args.plies, "branch": args.branch,
               "long_ms": args.long_ms, "reply_ms": args.reply_ms,
               "short_ms": args.short_ms, "only_corrections": args.only_corrections}

    if args.jobs > 1:
        return _build_sharded(binary, options, args)

    def walk(board: list[list[int]], ply: int) -> None:
        if ply >= args.plies:
            return
        key, index = canonical(board)
        if key in seen:
            return
        seen.add(key)
        stats["nodes"] += 1
        side = side_to_move(board)

        if side == our_side:
            long_move = ask(binary, board, args.long_ms)
            stats["queries"] += 1
            store = True
            if args.only_corrections:
                short = ask(binary, board, args.short_ms)
                stats["queries"] += 1
                store = short != long_move
                if not store:
                    stats["agreed"] += 1
            if store:
                cr, cc = TRANSFORMS[index](*long_move)
                book[key] = [cr, cc]
                stats["stored"] += 1
                print(f"  ply {ply:2d} store {long_move} (canonical {cr},{cc})", flush=True)
            board[long_move[0]][long_move[1]] = side
            walk(board, ply + 1)
            board[long_move[0]][long_move[1]] = EMPTY
        else:
            replies = top_moves(binary, board, args.reply_ms, args.branch)
            stats["queries"] += len(replies)
            for move in replies:
                board[move[0]][move[1]] = side
                walk(board, ply + 1)
                board[move[0]][move[1]] = EMPTY

    walk(empty_board(), 0)
    payload = {"engine": args.engine, "side": args.side, "plies": args.plies,
               "branch": args.branch, "long_ms": args.long_ms,
               "only_corrections": args.only_corrections, "stats": stats, "book": book}
    Path(args.out).write_text(json.dumps(payload, indent=1), encoding="utf-8")
    print(f"\n{stats}\nwrote {args.out}")
    return 0


FNV_OFFSET = 14695981039346656037
FNV_PRIME = 1099511628211
MASK64 = (1 << 64) - 1


def fnv1a64(text: str) -> int:
    """Must stay identical to contestBookHash() in the engine source."""
    digest = FNV_OFFSET
    for byte in text.encode("ascii"):
        digest = ((digest ^ byte) * FNV_PRIME) & MASK64
    return digest


def cmd_emit(args: argparse.Namespace) -> int:
    entries: dict[int, tuple[str, list[int]]] = {}
    for path in args.books:
        payload = json.loads(Path(path).read_text(encoding="utf-8"))
        for key, move in payload["book"].items():
            digest = fnv1a64(key)
            if digest in entries and entries[digest][0] != key:
                raise RuntimeError(f"hash collision between two positions: {digest}")
            entries[digest] = (key, move)
    if not entries:
        print("no book entries, nothing to emit")
        return 1

    lines = []
    for digest in sorted(entries):
        _, (row, col) = entries[digest]
        lines.append(f"\t{{{digest}ull, {row * SIZE + col}}},")
    body = "\n".join(lines)

    source = Path(args.source)
    text = source.read_text(encoding="latin-1")
    start = text.index("/* BOOK-BEGIN */")
    end = text.index("/* BOOK-END */")
    patched = (text[:start] + "/* BOOK-BEGIN */\n" + body + "\n" + text[end:])
    source.write_text(patched, encoding="latin-1")
    print(f"embedded {len(entries)} book entries into {args.source}")
    return 0


def _build_sharded(binary: Path, options: dict, args: argparse.Namespace) -> int:
    """Expand the top of the tree here, then hand each subtree to a worker.

    The frontier is grown breadth-first until there are at least as many
    subtrees as workers.  Positions visited during that expansion are searched
    and stored by this process, so nothing above the split is lost.
    """
    book: dict[str, list[int]] = {}
    stats = {"nodes": 0, "queries": 0, "stored": 0, "agreed": 0}
    frontier: list[tuple[list[list[int]], int]] = [(empty_board(), 0)]

    while len(frontier) < args.jobs:
        expandable = [(b, p) for b, p in frontier if p < options["plies"]]
        if not expandable:
            break
        nxt: list[tuple[list[list[int]], int]] = []
        for board, ply in frontier:
            if ply >= options["plies"]:
                continue
            side = side_to_move(board)
            if side == options["our_side"]:
                long_move = ask(binary, board, options["long_ms"])
                stats["queries"] += 1
                stats["nodes"] += 1
                store = True
                if options["only_corrections"]:
                    store = ask(binary, board, options["short_ms"]) != long_move
                    stats["queries"] += 1
                    if not store:
                        stats["agreed"] += 1
                if store:
                    key, index = canonical(board)
                    cr, cc = TRANSFORMS[index](*long_move)
                    book[key] = [cr, cc]
                    stats["stored"] += 1
                    print(f"  root ply {ply} store {long_move}", flush=True)
                child = [row[:] for row in board]
                child[long_move[0]][long_move[1]] = side
                nxt.append((child, ply + 1))
            else:
                for move in top_moves(binary, board, options["reply_ms"], options["branch"]):
                    stats["queries"] += 1
                    child = [row[:] for row in board]
                    child[move[0]][move[1]] = side
                    nxt.append((child, ply + 1))
        frontier = nxt

    print(f"split into {len(frontier)} subtrees across {args.jobs} workers", flush=True)
    payloads = [{"binary": str(binary), "board": board, "ply": ply, "options": options}
                for board, ply in frontier]
    with ProcessPoolExecutor(max_workers=args.jobs) as pool:
        for done, result in enumerate(pool.map(_walk_shard, payloads), 1):
            book.update(result["book"])
            for field, value in result["stats"].items():
                stats[field] += value
            print(f"  subtree {done}/{len(payloads)} done, {result['stats']['stored']} stored, "
                  f"{len(book)} entries so far", flush=True)

    payload = {"engine": args.engine, "side": args.side, "plies": args.plies,
               "branch": args.branch, "long_ms": args.long_ms, "jobs": args.jobs,
               "only_corrections": args.only_corrections, "stats": stats, "book": book}
    Path(args.out).write_text(json.dumps(payload, indent=1), encoding="utf-8")
    print(f"\n{stats}\nwrote {args.out}")
    return 0


def cmd_line(args: argparse.Namespace) -> int:
    """Turn one played game into book entries for our side of it.

    A line generated against the real opponent is worth more than one guessed
    from our own replies: it is the tree we will actually meet.  The moves come
    from a search given seconds per move, so replaying them costs a table lookup
    and buys back that depth.
    """
    text = Path(args.moves).read_text(encoding="utf-8")
    moves = []
    for chunk in text.split("\n"):
        nums = [int(v) for v in chunk.replace(",", " ").split() if v.lstrip("-").isdigit()]
        if len(nums) >= 2:
            moves.append((nums[-2], nums[-1]))

    our_side = BLACK if args.side == "black" else WHITE
    board = empty_board()
    book: dict[str, list[int]] = {}
    for ply, (r, c) in enumerate(moves):
        side = ply & 1
        if side == our_side:
            key, index = canonical(board)
            cr, cc = TRANSFORMS[index](r, c)
            book[key] = [cr, cc]
        board[r][c] = side

    payload = {"engine": "played line", "side": args.side, "source": args.moves,
               "stats": {"plies": len(moves), "stored": len(book)}, "book": book}
    Path(args.out).write_text(json.dumps(payload, indent=1), encoding="utf-8")
    print(f"{len(moves)} plies -> {len(book)} entries for {args.side}, wrote {args.out}")
    return 0


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__,
                                     formatter_class=argparse.RawDescriptionHelpFormatter)
    sub = parser.add_subparsers(dest="command", required=True)

    probe = sub.add_parser("probe", help="measure short-vs-long search disagreement")
    probe.add_argument("engine")
    probe.add_argument("--plies", type=int, default=12)
    probe.add_argument("--short-ms", type=int, default=820)
    probe.add_argument("--long-ms", type=int, default=15000)
    probe.add_argument("--json", dest="json_path")
    probe.set_defaults(func=cmd_probe)

    build = sub.add_parser("build", help="expand the opening tree into a book")
    build.add_argument("engine")
    build.add_argument("--side", choices=["black", "white"], required=True)
    build.add_argument("--plies", type=int, default=10)
    build.add_argument("--branch", type=int, default=2)
    build.add_argument("--long-ms", type=int, default=15000)
    build.add_argument("--reply-ms", type=int, default=5000)
    build.add_argument("--short-ms", type=int, default=820)
    build.add_argument("--only-corrections", action="store_true",
                       help="store only positions where the long search changes the move")
    build.add_argument("--jobs", type=int, default=1,
                       help="explore this many subtrees in parallel; each worker "
                            "runs its own engine process, so keep it below the core count")
    build.add_argument("--out", default="book.json")
    build.set_defaults(func=cmd_build)

    line = sub.add_parser("line", help="convert one played game into book entries")
    line.add_argument("moves", help="file with the move sequence, one 'row col' per line")
    line.add_argument("--side", choices=("black", "white"), required=True)
    line.add_argument("--out", default="book-line.json")
    line.set_defaults(func=cmd_line)

    emit = sub.add_parser("emit", help="embed generated books into the engine source")
    emit.add_argument("source", help="engine .cpp carrying the BOOK-BEGIN/BOOK-END markers")
    emit.add_argument("books", nargs="+", help="book JSON files produced by build")
    emit.set_defaults(func=cmd_emit)

    args = parser.parse_args()
    return args.func(args)


if __name__ == "__main__":
    sys.exit(main())

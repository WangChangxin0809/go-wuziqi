"""Reproducible command-line match runner for the BetaGomoku assignment.

Every move launches a fresh process and sends only the current side and the
15x15 board, matching the contest protocol. Results can be written as JSON so
different development directions can be compared in the repository.
"""
from __future__ import annotations

import argparse
from dataclasses import dataclass
import hashlib
import itertools
import json
import os
import subprocess
import sys
import time
from pathlib import Path
from typing import Any

from arena_server import black_forbidden, encode_input, in_board, winner
from gomoku_site_api import DEFAULT_STATE, STATUS_NAMES, SiteClient


ROOT = Path(__file__).resolve().parent
BUILD_DIR = ROOT / ".arena-build"
DEFAULT_OPENINGS = [
    {"name": "empty", "moves": []},
    {"name": "center-horizontal", "moves": [[7, 7], [7, 8]]},
    {"name": "center-diagonal", "moves": [[7, 7], [8, 8], [6, 8], [8, 7]]},
    {"name": "cross", "moves": [[7, 7], [6, 7], [7, 8], [8, 7], [8, 8], [6, 8]]},
]


@dataclass
class Engine:
    label: str
    binary: Path | None = None
    uid: str | None = None
    site: SiteClient | None = None

    def run(self, side: int, board: list[list[int]], timeout: float) -> dict[str, Any]:
        if self.binary is not None:
            return run_engine(self.binary, side, board, timeout)
        assert self.uid is not None and self.site is not None
        started = time.perf_counter()
        response = self.site.execute(self.uid, encode_input(side, board))
        request_ms = round((time.perf_counter() - started) * 1000, 3)
        raw_result = response.get("result", {})
        status = int(raw_result.get("status", 0))
        server_ns = raw_result.get("time")
        result: dict[str, Any] = {
            "ok": status == 1,
            "remote": True,
            "request_ms": request_ms,
            "time_ms": round(server_ns / 1_000_000, 3) if isinstance(server_ns, int) else None,
            "memory": raw_result.get("memory"),
            "code": raw_result.get("code"),
        }
        if status != 1:
            result["reason"] = STATUS_NAMES[status] if status < len(STATUS_NAMES) else "Unknown"
            return result
        words = str(response.get("output", "")).strip().split()
        if len(words) != 2:
            result.update({"ok": False, "reason": "Invalid Output", "output": response.get("output", "")})
            return result
        try:
            result["move"] = [int(words[0]), int(words[1])]
        except ValueError:
            result.update({"ok": False, "reason": "Invalid Output", "output": response.get("output", "")})
        return result


def source_path(value: str) -> Path:
    path = Path(value).resolve()
    if path.is_dir():
        path = path / "src.cpp"
    if not path.is_file():
        raise FileNotFoundError(path)
    return path


def source_id(path: Path) -> str:
    digest = hashlib.sha256(path.read_bytes()).hexdigest()[:12]
    safe = "".join(ch if ch.isalnum() or ch in "-_" else "-" for ch in path.parent.name)
    return f"{safe or path.stem}-{digest}"


def compile_source(path: Path) -> Path:
    BUILD_DIR.mkdir(exist_ok=True)
    suffix = ".exe" if os.name == "nt" else ""
    binary = BUILD_DIR / (source_id(path) + suffix)
    if binary.exists() and binary.stat().st_mtime_ns >= path.stat().st_mtime_ns:
        return binary
    cmd = ["g++", str(path), "-O2", "-o", str(binary), "-std=c++17", "-Wall"]
    proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True, timeout=90)
    if proc.returncode:
        raise RuntimeError(f"compile failed: {path}\n{proc.stdout}{proc.stderr}")
    return binary


def engine_from_spec(value: str, state_path: Path) -> Engine:
    if value.startswith("uid:"):
        uid = value.removeprefix("uid:")
        if not uid:
            raise ValueError("remote engine must be written as uid:STUDENT_ID")
        return Engine(label=f"uid:{uid}", uid=uid, site=SiteClient(state_path))
    path = source_path(value)
    try:
        label = str(path.relative_to(ROOT))
    except ValueError:
        label = str(path)
    if path.suffix.lower() in (".exe", ".out", ".py"):
        return Engine(label=label, binary=path)
    return Engine(label=label, binary=compile_source(path))


def run_engine(binary: Path, side: int, board: list[list[int]], timeout: float) -> dict[str, Any]:
    started = time.perf_counter()
    try:
        command = [sys.executable, str(binary)] if binary.suffix.lower() == ".py" else [str(binary)]
        proc = subprocess.run(
            command,
            input=encode_input(side, board),
            text=True,
            capture_output=True,
            cwd=binary.parent,
            timeout=timeout,
        )
        elapsed = round((time.perf_counter() - started) * 1000, 3)
        # Windows' subprocess timeout can return a process just after the
        # deadline instead of raising TimeoutExpired. The website's runner is
        # a hard wall-clock limit, so classify measured overruns consistently.
        if elapsed > timeout * 1000:
            return {"ok": False, "reason": "Time Limit Exceeded", "time_ms": elapsed,
                    "code": proc.returncode, "stderr": proc.stderr[-2000:]}
        if proc.returncode:
            return {"ok": False, "reason": "Runtime Error", "code": proc.returncode,
                    "stderr": proc.stderr[-2000:], "time_ms": elapsed}
        words = proc.stdout.strip().split()
        if len(words) != 2:
            return {"ok": False, "reason": "Invalid Output", "output": proc.stdout[-2000:],
                    "time_ms": elapsed}
        try:
            move = [int(words[0]), int(words[1])]
        except ValueError:
            return {"ok": False, "reason": "Invalid Output", "output": proc.stdout[-2000:],
                    "time_ms": elapsed}
        return {"ok": True, "move": move, "time_ms": elapsed,
                "stderr": proc.stderr[-2000:]}
    except subprocess.TimeoutExpired:
        return {"ok": False, "reason": "Time Limit Exceeded",
                "time_ms": round((time.perf_counter() - started) * 1000, 3)}
    except OSError as exc:
        return {"ok": False, "reason": "Runtime Error", "error": str(exc),
                "time_ms": round((time.perf_counter() - started) * 1000, 3)}


def initial_board(opening: dict[str, Any]) -> tuple[list[list[int]], list[dict[str, Any]]]:
    board = [[-1] * 15 for _ in range(15)]
    moves: list[dict[str, Any]] = []
    for ply, raw in enumerate(opening.get("moves", [])):
        if len(raw) != 2:
            raise ValueError(f"bad opening move: {raw}")
        r, c = int(raw[0]), int(raw[1])
        side = ply & 1
        if not in_board(r, c) or board[r][c] != -1:
            raise ValueError(f"illegal opening move at ply {ply}: {raw}")
        board[r][c] = side
        if side == 0:
            ban = black_forbidden(board, r, c)
            if ban:
                raise ValueError(f"opening contains {ban} at ply {ply}: {raw}")
        if winner(board) != -1:
            raise ValueError(f"opening already wins at ply {ply}: {raw}")
        moves.append({"ply": ply, "side": side, "move": [r, c], "opening": True})
    return board, moves


def play_game(
    black: Engine,
    white: Engine,
    opening: dict[str, Any],
    timeout: float,
    max_plies: int,
) -> dict[str, Any]:
    board, moves = initial_board(opening)
    players = [black, white]
    start_ply = len(moves)
    for ply in range(start_ply, min(225, max_plies)):
        side = ply & 1
        engine = players[side]
        result = engine.run(side, board, timeout)
        item = {"ply": ply, "side": side, "engine": engine.label, **result}
        moves.append(item)
        if not result["ok"]:
            return {"winner": side ^ 1, "reason": result["reason"], "opening": opening["name"],
                    "black": black.label, "white": white.label, "moves": moves}
        r, c = result["move"]
        if not in_board(r, c) or board[r][c] != -1:
            item["ok"] = False
            item["reason"] = "Illegal Move"
            return {"winner": side ^ 1, "reason": "Illegal Move", "opening": opening["name"],
                    "black": black.label, "white": white.label, "moves": moves}
        board[r][c] = side
        if side == 0:
            ban = black_forbidden(board, r, c)
            if ban:
                item["ok"] = False
                item["reason"] = ban
                return {"winner": 1, "reason": ban, "opening": opening["name"],
                        "black": black.label, "white": white.label, "moves": moves}
        if winner(board) == side:
            return {"winner": side, "reason": "Five", "opening": opening["name"],
                    "black": black.label, "white": white.label, "moves": moves}
    return {"winner": -1, "reason": "Draw", "opening": opening["name"],
            "black": black.label, "white": white.label, "moves": moves}


def load_openings(path: str | None) -> list[dict[str, Any]]:
    if not path:
        return DEFAULT_OPENINGS
    data = json.loads(Path(path).read_text(encoding="utf-8"))
    if not isinstance(data, list) or not data:
        raise ValueError("openings file must be a non-empty JSON array")
    return data


def load_position(path: str) -> tuple[int, list[list[int]]]:
    text = sys.stdin.read() if path == "-" else Path(path).read_text(encoding="utf-8")
    words = text.split()
    if len(words) != 226:
        raise ValueError("position must contain side plus 225 board cells")
    side = int(words[0])
    cells = [int(value) for value in words[1:]]
    if side not in (0, 1) or any(value not in (-1, 0, 1) for value in cells):
        raise ValueError("position contains an invalid side or cell value")
    return side, [cells[row * 15:(row + 1) * 15] for row in range(15)]


def probe(engine: Engine, position: str, timeout: float,
          expected: list[str]) -> tuple[dict[str, Any], bool]:
    side, board = load_position(position)
    result = engine.run(side, board, timeout)
    report: dict[str, Any] = {"engine": engine.label, "side": side, **result}
    passed = bool(result.get("ok"))
    if passed:
        r, c = result["move"]
        if not in_board(r, c) or board[r][c] != -1:
            report.update({"ok": False, "reason": "Illegal Move"})
            passed = False
        else:
            board[r][c] = side
            if side == 0:
                ban = black_forbidden(board, r, c)
                if ban:
                    report.update({"ok": False, "reason": ban})
                    passed = False
    accepted = []
    for value in expected:
        parts = value.replace(",", " ").split()
        if len(parts) != 2:
            raise ValueError(f"bad --expect coordinate: {value}")
        accepted.append([int(parts[0]), int(parts[1])])
    if accepted:
        report["expected"] = accepted
        report["matched"] = result.get("move") in accepted
        passed = passed and report["matched"]
    return report, passed


def match_pair(
    engine_a: Engine,
    engine_b: Engine,
    games: int,
    timeout: float,
    max_plies: int,
    openings: list[dict[str, Any]],
) -> dict[str, Any]:
    if games < 2 or games % 2:
        raise ValueError("games must be a positive even number so colors are swapped equally")
    label_a, label_b = engine_a.label, engine_b.label
    results = []
    score = {label_a: 0, label_b: 0, "draw": 0}
    for game_index in range(games):
        opening = openings[(game_index // 2) % len(openings)]
        if game_index % 2 == 0:
            black, white = engine_a, engine_b
        else:
            black, white = engine_b, engine_a
        result = play_game(black, white, opening, timeout, max_plies)
        results.append(result)
        if result["winner"] == -1:
            score["draw"] += 1
        else:
            score[[result["black"], result["white"]][result["winner"]]] += 1
        print(f"game {game_index + 1}/{games}: {result['black']} vs {result['white']} "
              f"-> winner={result['winner']} {result['reason']} ({len(result['moves'])} plies)")
    return {"schema": 1, "created_at": time.strftime("%Y-%m-%dT%H:%M:%S%z"),
            "timeout_seconds": timeout, "max_plies": max_plies,
            "sources": [label_a, label_b], "score": score, "games": results}


def write_report(report: dict[str, Any], destination: str | None) -> None:
    text = json.dumps(report, ensure_ascii=False, indent=2)
    if destination:
        path = Path(destination)
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(text + "\n", encoding="utf-8")
        print(f"report: {path.resolve()}")
    else:
        print(text)


def build_standings(pair_reports: list[dict[str, Any]]) -> list[dict[str, Any]]:
    table: dict[str, dict[str, Any]] = {}

    def row(name: str) -> dict[str, Any]:
        return table.setdefault(name, {"engine": name, "wins": 0, "losses": 0,
                                       "draws": 0, "tle": 0, "illegal": 0,
                                       "times_ms": []})

    for pair in pair_reports:
        for game in pair["games"]:
            black, white = game["black"], game["white"]
            for move in game["moves"]:
                if not move.get("opening") and isinstance(move.get("time_ms"), (int, float)):
                    row(move["engine"])["times_ms"].append(move["time_ms"])
            if game["winner"] == -1:
                row(black)["draws"] += 1
                row(white)["draws"] += 1
                continue
            victor = [black, white][game["winner"]]
            loser = [white, black][game["winner"]]
            row(victor)["wins"] += 1
            row(loser)["losses"] += 1
            if game["reason"] == "Time Limit Exceeded":
                row(loser)["tle"] += 1
            if "Ban" in game["reason"] or game["reason"] in ("Illegal Move", "Invalid Output"):
                row(loser)["illegal"] += 1

    result = []
    for item in table.values():
        times = sorted(item.pop("times_ms"))
        item["moves"] = len(times)
        item["avg_ms"] = round(sum(times) / len(times), 3) if times else None
        item["p95_ms"] = times[min(len(times) - 1, int(len(times) * 0.95))] if times else None
        item["max_ms"] = times[-1] if times else None
        result.append(item)
    return sorted(result, key=lambda item: (-item["wins"], item["tle"], item["illegal"],
                                            item["avg_ms"] or 0))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--state", type=Path, default=DEFAULT_STATE,
                        help="Playwright auth state used by uid: remote engines")
    sub = parser.add_subparsers(dest="command", required=True)
    pair = sub.add_parser("pair", help="run an even number of games with colors swapped")
    pair.add_argument("source_a")
    pair.add_argument("source_b")
    pair.add_argument("--games", type=int, default=2)
    pair.add_argument("--timeout", type=float, default=1.0)
    pair.add_argument("--max-plies", type=int, default=225)
    pair.add_argument("--openings")
    pair.add_argument("--json", dest="json_path")

    matrix = sub.add_parser("matrix", help="round-robin all supplied sources")
    matrix.add_argument("sources", nargs="+")
    matrix.add_argument("--games", type=int, default=2, help="games per pair")
    matrix.add_argument("--timeout", type=float, default=1.0)
    matrix.add_argument("--max-plies", type=int, default=225)
    matrix.add_argument("--openings")
    matrix.add_argument("--json", dest="json_path")

    probe_parser = sub.add_parser("probe", help="run one engine on one saved position")
    probe_parser.add_argument("engine", help="source path/directory or uid:STUDENT_ID")
    probe_parser.add_argument("position", help="position text file, or - for stdin")
    probe_parser.add_argument("--timeout", type=float, default=1.0)
    probe_parser.add_argument("--expect", action="append", default=[], metavar="ROW,COL")
    probe_parser.add_argument("--json", dest="json_path")

    args = parser.parse_args()
    if args.command == "probe":
        report, passed = probe(engine_from_spec(args.engine, args.state), args.position,
                               args.timeout, args.expect)
        write_report(report, args.json_path)
        return 0 if passed else 1

    openings = load_openings(args.openings)
    if args.command == "pair":
        report = match_pair(engine_from_spec(args.source_a, args.state),
                            engine_from_spec(args.source_b, args.state), args.games,
                            args.timeout, args.max_plies, openings)
    else:
        engines = [engine_from_spec(value, args.state) for value in args.sources]
        pair_reports = []
        for a, b in itertools.combinations(engines, 2):
            pair_reports.append(match_pair(a, b, args.games, args.timeout, args.max_plies, openings))
        standings = build_standings(pair_reports)
        report = {"schema": 1, "type": "matrix", "standings": standings,
                  "pairs": pair_reports}
        print("standings:")
        for index, item in enumerate(standings, 1):
            print(f"  {index}. {item['engine']}: {item['wins']}W {item['losses']}L "
                  f"{item['draws']}D, TLE={item['tle']}, illegal={item['illegal']}, "
                  f"p95={item['p95_ms']}ms")
    write_report(report, args.json_path)
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (FileNotFoundError, RuntimeError, ValueError, subprocess.TimeoutExpired) as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(2)

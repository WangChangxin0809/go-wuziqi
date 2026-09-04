"""Local BetaGomoku-compatible arena.

Stdlib-only server for compiling engines, executing one move with the contest
protocol, and running complete local games.  It intentionally keeps the same
rules as the assignment while making engine variants local and reproducible.
"""
from __future__ import annotations

import cgi
import json
import os
import re
import shutil
import subprocess
import sys
import time
from http.server import BaseHTTPRequestHandler, ThreadingHTTPServer
from pathlib import Path
from typing import Any

ROOT = Path(__file__).resolve().parent
PLAYERS = ROOT / "players"
WEB = ROOT / "arena.html"
PORT = int(os.environ.get("GOMOKU_ARENA_PORT", "8765"))
LIMIT_SECONDS = 1.0
BUILD_TIMEOUT = 90.0
UID_RE = re.compile(r"^[A-Za-z0-9_.-]{1,64}$")
DIRS = ((0, 1), (1, 0), (1, 1), (1, -1))


def safe_uid(uid: str) -> str:
    if not UID_RE.fullmatch(uid or ""):
        raise ValueError("invalid engine id")
    return uid


def engine_dir(uid: str) -> Path:
    return PLAYERS / safe_uid(uid)


def engine_bin(uid: str) -> Path:
    return engine_dir(uid) / ("engine.exe" if os.name == "nt" else "engine")


def compile_engine(uid: str, source: Path) -> dict[str, Any]:
    safe_uid(uid)
    out_dir = engine_dir(uid)
    out_dir.mkdir(parents=True, exist_ok=True)
    out = engine_bin(uid)
    cmd = ["g++", str(source), "-O2", "-o", str(out), "-std=c++17", "-Wall"]
    started = time.perf_counter()
    try:
        proc = subprocess.run(cmd, cwd=ROOT, capture_output=True, text=True,
                              timeout=BUILD_TIMEOUT)
        ok = proc.returncode == 0
        return {"success": ok, "diagnose": (proc.stdout + proc.stderr)[-12000:],
                "returncode": proc.returncode,
                "compile_ms": round((time.perf_counter() - started) * 1000, 1)}
    except FileNotFoundError:
        return {"success": False, "diagnose": "g++ not found in PATH", "returncode": -1}
    except subprocess.TimeoutExpired:
        return {"success": False, "diagnose": "compiler timeout", "returncode": -2}


def list_players() -> list[dict[str, Any]]:
    PLAYERS.mkdir(exist_ok=True)
    result = []
    for d in sorted(PLAYERS.iterdir()):
        if not d.is_dir() or not UID_RE.fullmatch(d.name):
            continue
        src = d / "src.cpp"
        binary = engine_bin(d.name)
        if not binary.exists() and src.exists():
            build = compile_engine(d.name, src)
        else:
            build = None
        if binary.exists():
            result.append({"uid": d.name, "source": src.exists(),
                           "built": True, "build": build})
    return result


def encode_input(side: int, board: list[list[int]]) -> str:
    return str(side) + "\n" + "\n".join(" ".join(map(str, row)) for row in board) + "\n"


def run_engine(uid: str, side: int, board: list[list[int]]) -> dict[str, Any]:
    binary = engine_bin(uid)
    if not binary.exists():
        return {"status": 4, "output": "", "error": "engine is not built", "time_ms": 0}
    payload = encode_input(side, board)
    started = time.perf_counter()
    try:
        proc = subprocess.run([str(binary)], input=payload, text=True,
                              capture_output=True, cwd=engine_dir(uid),
                              timeout=LIMIT_SECONDS)
        elapsed = (time.perf_counter() - started) * 1000
        status = 1 if proc.returncode == 0 else 4
        return {"status": status, "output": proc.stdout, "stderr": proc.stderr[-4000:],
                "code": proc.returncode, "time_ms": round(elapsed, 2)}
    except subprocess.TimeoutExpired as exc:
        elapsed = (time.perf_counter() - started) * 1000
        return {"status": 2, "output": (exc.stdout or ""), "stderr": "timeout",
                "code": None, "time_ms": round(elapsed, 2)}
    except OSError as exc:
        return {"status": 4, "output": "", "stderr": str(exc), "code": None,
                "time_ms": round((time.perf_counter() - started) * 1000, 2)}


def in_board(r: int, c: int) -> bool:
    return 0 <= r < 15 and 0 <= c < 15


def line_len(board: list[list[int]], r: int, c: int, side: int, dr: int, dc: int) -> int:
    n = 1
    for sign in (-1, 1):
        k = 1
        while in_board(r + sign * k * dr, c + sign * k * dc) and \
                board[r + sign * k * dr][c + sign * k * dc] == side:
            n += 1
            k += 1
    return n


def winner(board: list[list[int]]) -> int:
    for r in range(15):
        for c in range(15):
            side = board[r][c]
            if side not in (0, 1):
                continue
            for dr, dc in DIRS:
                if line_len(board, r, c, side, dr, dc) == 5:
                    return side
    return -1


def four_count_on_line(board: list[list[int]], r: int, c: int,
                       dr: int, dc: int) -> int:
    """Exact port of the website's getFourOnOneLine JavaScript function.

    The board already contains the newly placed black stone at (r, c), while
    `middle = 1` accounts for it and both scans start at the adjacent cells.
    """
    before, middle, after = 0, 1, 0
    gap_seen = False
    rr, cc = r - dr, c - dc
    while True:
        if not in_board(rr, cc):
            if not gap_seen:
                before = -1
            break
        value = board[rr][cc]
        if value == 1:
            if not gap_seen:
                before = -1
            break
        if value == 0:
            if gap_seen:
                before += 1
            else:
                middle += 1
        else:
            if gap_seen:
                break
            gap_seen = True
        rr, cc = rr - dr, cc - dc

    gap_seen = False
    rr, cc = r + dr, c + dc
    while True:
        if not in_board(rr, cc):
            if not gap_seen:
                after = -1
            break
        value = board[rr][cc]
        if value == 1:
            if not gap_seen:
                after = -1
            break
        if value == 0:
            if gap_seen:
                after += 1
            else:
                middle += 1
        else:
            if gap_seen:
                break
            gap_seen = True
        rr, cc = rr + dr, cc + dc

    if middle == 4:
        return 1 if before == 0 or after == 0 else 0
    return int(before > 0 and before + middle == 4) + \
        int(after > 0 and middle + after == 4)


def black_forbidden(board: list[list[int]], r: int, c: int) -> str | None:
    for dr, dc in DIRS:
        if line_len(board, r, c, 0, dr, dc) > 5:
            return "LongBan"
    four_count = sum(four_count_on_line(board, r, c, dr, dc) for dr, dc in DIRS)
    return "FourFourBan" if four_count > 1 else None


def play_game(p0: str, p1: str) -> dict[str, Any]:
    board = [[-1] * 15 for _ in range(15)]
    moves = []
    participants = [p0, p1]
    for ply in range(225):
        side = ply & 1
        uid = participants[side]
        result = run_engine(uid, side, board)
        item = {"ply": ply, "side": side, "uid": uid, **result}
        raw = str(result.get("output", "")).strip().split()
        if result["status"] != 1:
            item["error"] = ["Unknown", "OK", "Time Limit Exceeded", "Memory Limit Exceeded",
                              "Runtime Error"][result["status"]] if result["status"] < 5 else "Error"
            moves.append(item)
            return {"winner": side ^ 1, "reason": item["error"], "moves": moves}
        try:
            if len(raw) != 2:
                raise ValueError("Invalid output")
            r, c = int(raw[0]), int(raw[1])
            if not in_board(r, c) or board[r][c] != -1:
                raise ValueError("Illegal move")
            board[r][c] = side
            if side == 0:
                ban = black_forbidden(board, r, c)
                if ban:
                    board[r][c] = -1
                    raise ValueError(ban)
            item["move"] = [r, c]
            moves.append(item)
            if winner(board) != -1:
                return {"winner": side, "reason": "Five", "moves": moves}
        except (ValueError, IndexError) as exc:
            item["error"] = str(exc)
            moves.append(item)
            return {"winner": side ^ 1, "reason": str(exc), "moves": moves}
    return {"winner": -1, "reason": "Draw", "moves": moves}


class Handler(BaseHTTPRequestHandler):
    def log_message(self, fmt: str, *args: Any) -> None:
        sys.stderr.write("[arena] " + (fmt % args) + "\n")

    def send_json(self, value: Any, code: int = 200) -> None:
        data = json.dumps(value, ensure_ascii=False).encode()
        self.send_response(code)
        self.send_header("Content-Type", "application/json; charset=utf-8")
        self.send_header("Content-Length", str(len(data)))
        self.end_headers()
        self.wfile.write(data)

    def do_GET(self) -> None:
        if self.path in ("/", "/index.html"):
            data = WEB.read_bytes()
            self.send_response(200)
            self.send_header("Content-Type", "text/html; charset=utf-8")
            self.send_header("Content-Length", str(len(data)))
            self.end_headers()
            self.wfile.write(data)
        elif self.path == "/api/players":
            self.send_json({"players": list_players()})
        else:
            self.send_json({"error": "not found"}, 404)

    def read_json(self) -> dict[str, Any]:
        length = int(self.headers.get("Content-Length", "0"))
        return json.loads(self.rfile.read(length) or b"{}")

    def do_POST(self) -> None:
        try:
            if self.path == "/api/submit":
                form = cgi.FieldStorage(fp=self.rfile, headers=self.headers,
                                        environ={"REQUEST_METHOD": "POST",
                                                 "CONTENT_TYPE": self.headers.get("Content-Type", "")})
                upload = form["src"]
                if not getattr(upload, "filename", None):
                    raise ValueError("missing src.cpp")
                uid = "local-current"
                source = engine_dir(uid) / "src.cpp"
                source.parent.mkdir(parents=True, exist_ok=True)
                with source.open("wb") as f:
                    shutil.copyfileobj(upload.file, f)
                result = compile_engine(uid, source)
                self.send_json({"error": None, "compile": result})
            elif self.path == "/api/build":
                body = self.read_json()
                uid = safe_uid(str(body["uid"]))
                source = engine_dir(uid) / "src.cpp"
                if not source.exists():
                    raise ValueError("source not found")
                self.send_json({"uid": uid, "compile": compile_engine(uid, source)})
            elif self.path == "/api/exec":
                body = self.read_json()
                uid = safe_uid(str(body["uid"]))
                text = str(body["input"])
                rows = text.split()
                if len(rows) != 226:
                    raise ValueError("input must contain side plus 225 cells")
                side = int(rows[0])
                board = [list(map(int, rows[1 + r * 15:1 + (r + 1) * 15])) for r in range(15)]
                self.send_json(run_engine(uid, side, board))
            elif self.path == "/api/game":
                body = self.read_json()
                self.send_json(play_game(safe_uid(str(body["p0"])), safe_uid(str(body["p1"]))))
            else:
                self.send_json({"error": "not found"}, 404)
        except Exception as exc:
            self.send_json({"error": str(exc)}, 400)


if __name__ == "__main__":
    PLAYERS.mkdir(exist_ok=True)
    server = ThreadingHTTPServer(("127.0.0.1", PORT), Handler)
    print(f"Local BetaGomoku arena: http://127.0.0.1:{PORT}/")
    try:
        server.serve_forever()
    except KeyboardInterrupt:
        pass
    finally:
        server.server_close()

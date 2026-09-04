"""Small authenticated client for the BetaGomoku web endpoints.

Authentication is read from a Playwright storage-state file. The cookie file
is local-only under .playwright/ and must never be committed.
"""
from __future__ import annotations

import argparse
import json
import os
import re
import sys
import urllib.error
import urllib.request
import uuid
from pathlib import Path
from typing import Any


BASE_URL = "http://gomoku.ruc.rvalue.moe"
DEFAULT_STATE = Path(__file__).resolve().parent / ".playwright" / "gomoku-auth.json"
STATUS_NAMES = ["Unknown", "OK", "Time Limit Exceeded", "Memory Limit Exceeded",
                "Runtime Error", "Cancelled", "Output Limit Exceeded"]


def cookie_header(state_path: Path) -> str:
    data = json.loads(state_path.read_text(encoding="utf-8"))
    cookies = []
    for cookie in data.get("cookies", []):
        domain = str(cookie.get("domain", "")).lstrip(".")
        if domain == "gomoku.ruc.rvalue.moe":
            cookies.append(f"{cookie['name']}={cookie['value']}")
    if not cookies:
        raise RuntimeError(f"no BetaGomoku cookie in {state_path}")
    return "; ".join(cookies)


class SiteClient:
    def __init__(self, state_path: Path, base_url: str = BASE_URL):
        self.base_url = base_url.rstrip("/")
        self.cookie = cookie_header(state_path)

    def request(self, path: str, *, data: bytes | None = None,
                content_type: str | None = None) -> bytes:
        headers = {"Cookie": self.cookie, "Referer": self.base_url + "/",
                   "User-Agent": "go-wuziqi-test-client/1"}
        if content_type:
            headers["Content-Type"] = content_type
        request = urllib.request.Request(self.base_url + path, data=data, headers=headers)
        with urllib.request.urlopen(request, timeout=30) as response:
            return response.read()

    def players(self) -> list[str]:
        html = self.request("/").decode("utf-8")
        values = re.findall(r'<option[^>]+value=["\']([^"\']+)["\']', html)
        return sorted(set(value for value in values if value))

    def execute(self, uid: str, input_text: str) -> dict[str, Any]:
        body = json.dumps({"uid": str(uid), "input": input_text}).encode("utf-8")
        return json.loads(self.request("/api/exec", data=body,
                                       content_type="application/json").decode("utf-8"))

    def submit(self, source: Path) -> dict[str, Any]:
        boundary = "----gomoku-" + uuid.uuid4().hex
        content = source.read_bytes()
        body = (
            f"--{boundary}\r\n"
            f"Content-Disposition: form-data; name=\"src\"; filename=\"{source.name}\"\r\n"
            "Content-Type: text/x-c++src\r\n\r\n"
        ).encode() + content + f"\r\n--{boundary}--\r\n".encode()
        return json.loads(self.request("/api/submit", data=body,
                                       content_type=f"multipart/form-data; boundary={boundary}")
                          .decode("utf-8"))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--state", type=Path, default=DEFAULT_STATE)
    parser.add_argument("--base-url", default=BASE_URL)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("players")
    execute = sub.add_parser("exec")
    execute.add_argument("uid")
    execute.add_argument("input", help="input text file, or - for stdin")
    submit = sub.add_parser("submit")
    submit.add_argument("source", type=Path)
    args = parser.parse_args()

    client = SiteClient(args.state, args.base_url)
    if args.command == "players":
        print("\n".join(client.players()))
    elif args.command == "exec":
        input_text = sys.stdin.read() if args.input == "-" else Path(args.input).read_text(encoding="utf-8")
        result = client.execute(args.uid, input_text)
        status = int(result.get("result", {}).get("status", 0))
        result["status_name"] = STATUS_NAMES[status] if status < len(STATUS_NAMES) else "Unknown"
        print(json.dumps(result, ensure_ascii=False, indent=2))
    else:
        # This explicitly replaces the account's active online submission.
        print(json.dumps(client.submit(args.source), ensure_ascii=False, indent=2))
    return 0


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except (OSError, RuntimeError, ValueError, urllib.error.URLError) as exc:
        print(f"error: {exc}", file=sys.stderr)
        raise SystemExit(2)

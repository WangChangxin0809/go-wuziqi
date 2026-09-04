"""Small authenticated client for the BetaGomoku web endpoints.

Authentication comes from a browser session cookie, taken from --cookie, the
GOMOKU_COOKIE environment variable, a plain-text cookie file, or a Playwright
storage-state file. All of those live under .playwright/ and are local-only;
they must never be committed.
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
SITE_DOMAIN = "gomoku.ruc.rvalue.moe"
AUTH_DIR = Path(__file__).resolve().parent / ".playwright"
DEFAULT_STATE = AUTH_DIR / "gomoku-auth.json"
DEFAULT_COOKIE_FILE = AUTH_DIR / "gomoku-cookie.txt"
STATUS_NAMES = ["Unknown", "OK", "Time Limit Exceeded", "Memory Limit Exceeded",
                "Runtime Error", "Cancelled", "Output Limit Exceeded"]
LOGIN_HINT = (
    "not logged in: the site redirected to the 微人大 login page.\n"
    "Log in with a browser, copy the gomoku.ruc.rvalue.moe cookie, and pass it via\n"
    "--cookie, GOMOKU_COOKIE, or .playwright/gomoku-cookie.txt (see SITE_API.md)."
)


def normalize_cookie(text: str) -> str:
    """Accept a raw Cookie header, a `Cookie: ...` line, or one name=value pair."""
    pairs = []
    for line in text.splitlines():
        line = line.strip()
        if not line or line.startswith("#"):
            continue
        if line.lower().startswith("cookie:"):
            line = line.split(":", 1)[1].strip()
        pairs.extend(part.strip() for part in line.split(";") if part.strip())
    if not pairs or not all("=" in pair for pair in pairs):
        raise ValueError("cookie must be one or more name=value pairs")
    return "; ".join(pairs)


def cookie_from_state(state_path: Path) -> str:
    data = json.loads(state_path.read_text(encoding="utf-8"))
    cookies = []
    for cookie in data.get("cookies", []):
        domain = str(cookie.get("domain", "")).lstrip(".")
        if domain == SITE_DOMAIN:
            cookies.append(f"{cookie['name']}={cookie['value']}")
    if not cookies:
        raise RuntimeError(f"no BetaGomoku cookie in {state_path}")
    return "; ".join(cookies)


def resolve_cookie(cookie: str | None, cookie_file: Path | None,
                   state_path: Path | None) -> str:
    """Pick the first credential that is actually present, most explicit first."""
    if cookie:
        return normalize_cookie(cookie)
    from_env = os.environ.get("GOMOKU_COOKIE")
    if from_env:
        return normalize_cookie(from_env)
    if cookie_file and cookie_file.exists():
        return normalize_cookie(cookie_file.read_text(encoding="utf-8"))
    if state_path and state_path.exists():
        return cookie_from_state(state_path)
    raise RuntimeError(
        "no credential found. Provide one of:\n"
        "  --cookie 'name=value'\n"
        "  GOMOKU_COOKIE=name=value\n"
        f"  {DEFAULT_COOKIE_FILE} (paste the browser Cookie header)\n"
        f"  {DEFAULT_STATE} (Playwright storage state)"
    )


class NotLoggedIn(RuntimeError):
    pass


class SiteClient:
    def __init__(self, cookie: str, base_url: str = BASE_URL):
        self.base_url = base_url.rstrip("/")
        self.cookie = cookie

    def request(self, path: str, *, data: bytes | None = None,
                content_type: str | None = None) -> bytes:
        headers = {"Cookie": self.cookie, "Referer": self.base_url + "/",
                   "User-Agent": "go-wuziqi-test-client/1"}
        if content_type:
            headers["Content-Type"] = content_type
        request = urllib.request.Request(self.base_url + path, data=data, headers=headers)
        with urllib.request.urlopen(request, timeout=30) as response:
            if "/users/login" in response.geturl() and "/users/login" not in path:
                raise NotLoggedIn(LOGIN_HINT)
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
    parser.add_argument("--state", type=Path, default=DEFAULT_STATE,
                        help="Playwright storage-state file")
    parser.add_argument("--cookie", help="raw Cookie header for the site")
    parser.add_argument("--cookie-file", type=Path, default=DEFAULT_COOKIE_FILE,
                        help="file holding a pasted Cookie header")
    parser.add_argument("--base-url", default=BASE_URL)
    sub = parser.add_subparsers(dest="command", required=True)
    sub.add_parser("players")
    sub.add_parser("check", help="verify the credential is accepted")
    execute = sub.add_parser("exec")
    execute.add_argument("uid")
    execute.add_argument("input", help="input text file, or - for stdin")
    submit = sub.add_parser("submit")
    submit.add_argument("source", type=Path)
    args = parser.parse_args()

    client = SiteClient(resolve_cookie(args.cookie, args.cookie_file, args.state),
                        args.base_url)
    if args.command == "check":
        players = client.players()
        print(f"logged in; {len(players)} player uid(s) visible")
    elif args.command == "players":
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

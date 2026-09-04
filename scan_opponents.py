#!/usr/bin/env python3
"""Ask every submitted program for one move and record what comes back.

Two hundred entries is too many to play, but one call each is cheap and sorts
them: broken output, an instant answer from a book, or a program that spends
most of its second thinking.  Only the last group is worth a match.
"""
from __future__ import annotations

import argparse
import json
import re
from concurrent.futures import ThreadPoolExecutor
from pathlib import Path

from gomoku_site_api import (SiteClient, DEFAULT_COOKIE_FILE, DEFAULT_STATE,
                             STATUS_NAMES, resolve_cookie)

SIZE = 15


def probe(client: SiteClient, uid: str, board_text: str) -> dict:
    try:
        reply = client.execute(uid, board_text)
    except Exception as exc:                       # network or auth, not the player's fault
        return {"uid": uid, "verdict": "error", "detail": str(exc)[:80]}
    result = reply.get("result", {}) or {}
    status = result.get("status", 0)
    output = str(reply.get("output", "")).strip()
    entry = {"uid": uid, "status": STATUS_NAMES[status] if status < len(STATUS_NAMES) else status,
             "ms": round(result.get("time", 0) / 1e6, 1),
             "mib": round(result.get("memory", 0) / 1048576, 1), "output": output[:40]}
    numbers = re.findall(r"-?\d+", output)
    if status != 1:
        entry["verdict"] = "failed"
    elif len(numbers) < 2 or not (0 <= int(numbers[0]) < SIZE and 0 <= int(numbers[1]) < SIZE):
        entry["verdict"] = "invalid"
    else:
        entry["verdict"] = "ok"
    return entry


def main() -> int:
    ap = argparse.ArgumentParser(description=__doc__)
    ap.add_argument("--json", dest="json_path", default="reports/opponent-scan.json")
    ap.add_argument("--workers", type=int, default=4)
    ap.add_argument("--limit", type=int)
    args = ap.parse_args()

    client = SiteClient(resolve_cookie(None, DEFAULT_COOKIE_FILE, DEFAULT_STATE))
    uids = client.players()
    if args.limit:
        uids = uids[:args.limit]
    # an empty board asks the opening question, where a book shows up as a fast answer
    board_text = "0\n" + "\n".join(" ".join("-1" for _ in range(SIZE)) for _ in range(SIZE)) + "\n"

    with ThreadPoolExecutor(max_workers=args.workers) as pool:
        entries = list(pool.map(lambda uid: probe(client, uid, board_text), uids))

    buckets: dict[str, list[dict]] = {}
    for entry in entries:
        buckets.setdefault(entry["verdict"], []).append(entry)
    for verdict in ("ok", "invalid", "failed", "error"):
        rows = buckets.get(verdict, [])
        print(f"{verdict}: {len(rows)}")
    live = sorted(buckets.get("ok", []), key=lambda e: -e["ms"])
    print(f"\n用时最长的 20 个（一秒里花得越多，越可能是在真搜索）:")
    for entry in live[:20]:
        print(f"  {entry['uid']}  {entry['ms']:>7.1f} ms  {entry['mib']:>6.1f} MiB  -> {entry['output']}")
    Path(args.json_path).write_text(json.dumps(entries, indent=1), encoding="utf-8")
    print(f"\nwrote {args.json_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())

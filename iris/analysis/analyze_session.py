#!/usr/bin/env python3
"""Turn an AquaCue pool log and a video tally into the session report.

The firmware CSV is the device record. The tally CSV is what a person counted
on the video. Empty video cells are left as unknown rather than guessed.

Synthetic self-test:

    python3 analyze_session.py --self-test
"""

from __future__ import annotations

import argparse
import csv
import sys
from pathlib import Path

BLOCKS = ("easy1", "altered", "easy2")
MARK_OF = {"easy1": 1, "altered": 2, "easy2": 3}


def read_log(path: Path) -> list[dict]:
    rows = []
    with path.open(newline="") as handle:
        for line in handle:
            if not line.strip() or line.startswith("#"):
                continue
            rows.append(line)
    reader = csv.DictReader(rows)
    parsed = []
    for row in reader:
        parsed.append(
            {
                "t_ms": int(float(row["t_ms"])),
                "angle": float(row["angle_deg"]),
                "stroke": int(row["stroke"]),
                "haptic": int(row["haptic"]),
                "mark": int(row["mark"]),
                "mode": row["mode"].strip(),
            }
        )
    return parsed


def read_tally(path: Path) -> dict[str, dict]:
    grouped = {name: {"strokes": 0, "buzzes": 0, "buzz_known": False, "lengths": 0} for name in BLOCKS}
    with path.open(newline="") as handle:
        for row in csv.DictReader(handle):
            block = row["block"].strip()
            if block not in grouped:
                continue
            strokes = row.get("video_strokes", "").strip()
            buzzes = row.get("video_buzzes", "").strip()
            if strokes == "":
                continue
            grouped[block]["strokes"] += int(strokes)
            grouped[block]["lengths"] += 1
            if buzzes != "":
                grouped[block]["buzzes"] += int(buzzes)
                grouped[block]["buzz_known"] = True
    return grouped


def mark_times(rows: list[dict]) -> dict[int, int]:
    found = {}
    for row in rows:
        if row["mark"] and row["mark"] not in found:
            found[row["mark"]] = row["t_ms"]
    return found


def in_block(row: dict, marks: dict[int, int], block: str) -> bool:
    start = marks.get(MARK_OF[block])
    if start is None or row["t_ms"] < start:
        return False
    later = [marks[k] for k in (2, 3, 4) if k in marks and marks[k] > start]
    # easy1 ends at mark 2, altered at mark 3, easy2 at mark 4.
    end_mark = MARK_OF[block] + 1
    if end_mark in marks and row["t_ms"] >= marks[end_mark]:
        return False
    if later and row["t_ms"] >= min(later):
        return False
    return True


def stats(values: list[float]) -> tuple[float, float]:
    if not values:
        return float("nan"), float("nan")
    mean = sum(values) / len(values)
    if len(values) == 1:
        return mean, 0.0
    var = sum((v - mean) ** 2 for v in values) / (len(values) - 1)
    return mean, var ** 0.5


def pct(device: int, video: int) -> str:
    if video == 0:
        return "n/a"
    return f"{100.0 * (device - video) / video:+.1f}%"


def analyze(rows: list[dict], tally: dict[str, dict] | None) -> str:
    marks = mark_times(rows)
    missing = [str(k) for k in (1, 2, 3, 4) if k not in marks]
    swim_strokes = [row for row in rows if row["mode"] == "W" and row["stroke"] == 1]
    baseline = swim_strokes[:8]
    base_mean, base_sd = stats([row["angle"] for row in baseline])

    lines = ["# AquaCue session report", ""]
    if missing:
        lines.append("Missing mark numbers: " + ", ".join(missing) + ".")
        lines.append("The button should be pressed four times: start easy1, altered, easy2, stop.")
        lines.append("")
    lines.append(f"Baseline events: {len(baseline)} of 8.")
    if baseline:
        lines.append(f"Baseline angle mean {base_mean:.2f} deg, sample sd {base_sd:.2f} deg.")
    lines.append("")
    lines.append("| block | device strokes | device haptics | event angle mean | event angle sd | video strokes | stroke error | video buzzes |")
    lines.append("| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |")

    summary = {}
    for block in BLOCKS:
        chosen = [row for row in rows if row["mode"] == "W" and in_block(row, marks, block)]
        strokes = [row for row in chosen if row["stroke"] == 1]
        if block == "easy1":
            reported = strokes[8:] if len(strokes) > 8 else []
            angle_note_rows = reported
        else:
            angle_note_rows = strokes
        mean, sd = stats([row["angle"] for row in angle_note_rows])
        haptics = sum(row["haptic"] for row in chosen)
        video_strokes = ""
        error = ""
        video_buzz = ""
        if tally is not None and tally[block]["lengths"]:
            video_strokes = str(tally[block]["strokes"])
            error = pct(len(strokes), tally[block]["strokes"])
            video_buzz = str(tally[block]["buzzes"]) if tally[block]["buzz_known"] else ""
        lines.append(
            f"| {block} | {len(strokes)} | {haptics} | {mean:.2f} | {sd:.2f} | "
            f"{video_strokes} | {error} | {video_buzz} |"
        )
        summary[block] = {
            "strokes": strokes,
            "haptics": haptics,
            "mean": mean,
            "sd": sd,
            "after_baseline": angle_note_rows,
        }

    lines.append("")
    lines.append("easy1 angles in the table exclude the first eight stroke events, which are the baseline.")
    lines.append("Haptic counts include every logged pulse in the block, including any during those eight.")
    lines.append("")

    altered = summary["altered"]["after_baseline"]
    easy_after = summary["easy1"]["after_baseline"] + summary["easy2"]["after_baseline"]
    alt_mean, alt_sd = stats([row["angle"] for row in altered])
    easy_mean, easy_sd = stats([row["angle"] for row in easy_after])
    if altered and baseline:
        shift = alt_mean - base_mean
        lines.append(
            f"Altered-entry mean minus baseline mean: {shift:+.2f} deg "
            f"(altered sd {alt_sd:.2f}, baseline sd {base_sd:.2f})."
        )
        lines.append(
            "The shift is larger than the baseline scatter."
            if abs(shift) > base_sd
            else "The shift is not larger than the baseline scatter."
        )
    if easy_after:
        lines.append(
            f"Easy lengths after the baseline, pooled angle mean {easy_mean:.2f} deg, sd {easy_sd:.2f}."
        )
    easy_h = summary["easy1"]["haptics"] + summary["easy2"]["haptics"]
    alt_h = summary["altered"]["haptics"]
    easy_n = len(summary["easy1"]["strokes"]) + len(summary["easy2"]["strokes"])
    alt_n = len(summary["altered"]["strokes"])
    lines.append(
        f"Haptic pulses: easy blocks {easy_h} in {easy_n} strokes; "
        f"altered block {alt_h} in {alt_n} strokes."
    )
    if alt_n and easy_n:
        alt_rate = alt_h / alt_n
        easy_rate = easy_h / easy_n
        if alt_rate > easy_rate:
            lines.append("The altered block produced a higher haptic rate than the easy blocks.")
        else:
            lines.append("The altered block did not produce a higher haptic rate than the easy blocks.")
    lines.append("")
    lines.append("Stroke error uses (device - video) / video. A negative value means the device under-counted.")
    lines.append("One swimmer and one session: these are descriptions of this outing, not a general error rate.")
    return "\n".join(lines) + "\n"


def write_self_test(directory: Path) -> tuple[Path, Path]:
    directory.mkdir(parents=True, exist_ok=True)
    log_path = directory / "synthetic_session.csv"
    tally_path = directory / "synthetic_tally.csv"
    lines = ["# SYNTHETIC — not a pool recording",
             "t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,angle_deg,stroke,haptic,mark,mode"]
    t = 0
    lines.append(f"{t},0,0,1,0,0,0,0,0,0,1,W")
    for i in range(20):
        t += 1000
        lines.append(f"{t},0,0,1,100,0,0,10,1,0,0,W")
    t += 1000
    lines.append(f"{t},0,0,1,0,0,0,10,0,0,2,W")
    for i in range(16):
        t += 1000
        haptic = 1 if i % 4 == 1 else 0
        lines.append(f"{t},0,0,1,100,0,0,40,1,{haptic},0,W")
    t += 1000
    lines.append(f"{t},0,0,1,0,0,0,11,0,0,3,W")
    for i in range(18):
        t += 1000
        lines.append(f"{t},0,0,1,100,0,0,11,1,0,0,W")
    t += 1000
    lines.append(f"{t},0,0,1,0,0,0,11,0,0,4,W")
    log_path.write_text("\n".join(lines) + "\n")
    tally_path.write_text(
        "block,length,video_strokes,video_buzzes,notes\n"
        "easy1,1,10,0,synthetic\n"
        "easy1,2,10,0,synthetic\n"
        "altered,1,16,4,synthetic\n"
        "easy2,1,18,0,synthetic\n"
    )
    return log_path, tally_path


def self_test() -> None:
    directory = Path(__file__).resolve().parent / "testdata"
    log_path, tally_path = write_self_test(directory)
    report = analyze(read_log(log_path), read_tally(tally_path))
    print(report)
    checks = [
        "easy1 | 20 | 0 |" in report,
        "altered | 16 | 4 |" in report,
        "easy2 | 18 | 0 |" in report,
        "+0.0%" in report,
        "Baseline angle mean 10.00 deg" in report,
        "The shift is larger than the baseline scatter." in report,
        "higher haptic rate" in report,
    ]
    if not all(checks):
        print("SELF-TEST FAILED", file=sys.stderr)
        for i, ok in enumerate(checks):
            if not ok:
                print(f" check {i} failed", file=sys.stderr)
        sys.exit(1)
    print("self-test passed")
    print("synthetic files are in analysis/testdata and are not pool data")


def main() -> None:
    parser = argparse.ArgumentParser(description="Report an AquaCue pool session.")
    parser.add_argument("log", nargs="?", type=Path)
    parser.add_argument("tally", nargs="?", type=Path)
    parser.add_argument("--self-test", action="store_true")
    args = parser.parse_args()
    if args.self_test:
        self_test()
        return
    if args.log is None:
        parser.error("give a firmware CSV, or pass --self-test")
    rows = read_log(args.log)
    tally = read_tally(args.tally) if args.tally else None
    print(analyze(rows, tally))


if __name__ == "__main__":
    main()

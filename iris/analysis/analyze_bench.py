#!/usr/bin/env python3
"""RMSE of the protractor bench against the nominal angles.

CSV columns: nominal_deg,mean_deg,sd_deg
"""

from __future__ import annotations

import argparse
import csv
import math
import sys
from pathlib import Path


def load(path: Path) -> list[tuple[float, float, float]]:
    rows = []
    with path.open(newline="") as handle:
        for row in csv.DictReader(handle):
            if not row["nominal_deg"].strip() or not row["mean_deg"].strip():
                continue
            sd = row["sd_deg"].strip()
            rows.append(
                (
                    float(row["nominal_deg"]),
                    float(row["mean_deg"]),
                    float(sd) if sd else float("nan"),
                )
            )
    return rows


def report(rows: list[tuple[float, float, float]]) -> str:
    if not rows:
        return "No bench rows yet. Fill nominal_deg and mean_deg, then run this again.\n"
    lines = ["| nominal deg | device mean | error | sd |", "| ---: | ---: | ---: | ---: |"]
    sq = 0.0
    for nominal, mean, sd in rows:
        err = mean - nominal
        sq += err * err
        sd_text = f"{sd:.2f}" if sd == sd else ""
        lines.append(f"| {nominal:.1f} | {mean:.2f} | {err:+.2f} | {sd_text} |")
    rmse = math.sqrt(sq / len(rows))
    lines.append("")
    lines.append(f"RMSE: {rmse:.2f} deg over {len(rows)} poses.")
    lines.append("Paste this table into the bench section of the paper.")
    return "\n".join(lines) + "\n"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("csv", type=Path)
    args = parser.parse_args()
    if not args.csv.exists():
        print(f"missing {args.csv}", file=sys.stderr)
        sys.exit(1)
    print(report(load(args.csv)))


if __name__ == "__main__":
    main()

"""Create clean made-up IMU data for Assignment 2, Part 1.

Run from this folder:
    python temp.py

The script writes synthetic_imu_data.csv beside this file.  The data has four
25 m laps, short rests between laps, a small left/right movement, and slower
later laps.  It is only for testing the Part 1 calculations until a usable
Kaggle dataset is available.
"""

from __future__ import annotations

import argparse
import csv
import math
import random
from pathlib import Path


DEFAULT_OUTPUT = Path(__file__).with_name("synthetic_imu_data.csv")


def add_rest(rows: list[dict[str, float | int]], time_s: float, rest_s: float, dt: float, rng: random.Random) -> float:
    """Add nearly still sensor readings for a rest between two laps."""
    number_of_rows = round(rest_s / dt)
    for _ in range(number_of_rows):
        rows.append(
            {
                "timestamp": round(time_s, 3),
                "ax": round(rng.gauss(0, 0.01), 5),
                "ay": round(rng.gauss(0, 0.01), 5),
                "az": round(rng.gauss(0, 0.01), 5),
                "lap_id": 0,
                "is_swimming": 0,
            }
        )
        time_s += dt
    return time_s


def add_lap(
    rows: list[dict[str, float | int]],
    time_s: float,
    lap_id: int,
    lap_time_s: float,
    lane_length_m: float,
    dt: float,
    rng: random.Random,
) -> float:
    """Add one smooth forward lap.

    The swimmer starts and finishes at speed zero.  The speed is highest in
    the middle of the lap.  A gentle sideways wave gives the line metrics
    something realistic to measure.
    """
    number_of_rows = round(lap_time_s / dt)
    peak_forward_speed = lane_length_m * math.pi / (2 * lap_time_s)
    sideways_distance_m = 0.20

    for sample_number in range(number_of_rows):
        local_time_s = sample_number * dt
        progress = local_time_s / lap_time_s  # 0 at start and nearly 1 at end

        # Forward motion is along +Y.  Sideways motion is along X.
        forward_acceleration = (
            peak_forward_speed * math.pi / lap_time_s * math.cos(math.pi * progress)
        )
        sideways_acceleration = (
            -sideways_distance_m
            * (2 * math.pi / lap_time_s) ** 2
            * math.sin(2 * math.pi * progress)
        )

        # A small up/down signal makes the Z column non-empty.
        vertical_acceleration = 0.18 * math.sin(4 * math.pi * progress)

        rows.append(
            {
                "timestamp": round(time_s, 3),
                "ax": round(sideways_acceleration + rng.gauss(0, 0.01), 5),
                "ay": round(forward_acceleration + rng.gauss(0, 0.01), 5),
                "az": round(vertical_acceleration + rng.gauss(0, 0.01), 5),
                "lap_id": lap_id,
                "is_swimming": 1,
            }
        )
        time_s += dt

    return time_s


def generate_data(
    output_path: Path,
    laps: int = 4,
    lane_length_m: float = 25.0,
    first_lap_time_s: float = 18.0,
    final_lap_slower_by_percent: float = 12.0,
    rest_between_laps_s: float = 3.0,
    sample_time_s: float = 0.05,
    seed: int = 42,
) -> int:
    """Create the CSV file and return the number of rows written."""
    if laps < 1:
        raise ValueError("laps must be at least 1")
    if lane_length_m <= 0 or first_lap_time_s <= 0 or sample_time_s <= 0:
        raise ValueError("lane length, lap time, and sample time must be positive")

    rng = random.Random(seed)
    rows: list[dict[str, float | int]] = []
    time_s = 0.0

    for lap_id in range(1, laps + 1):
        # First lap is quickest.  The last lap is slower by the chosen percent.
        if laps == 1:
            slower_fraction = 0.0
        else:
            slower_fraction = (lap_id - 1) / (laps - 1)
        lap_time_s = first_lap_time_s * (
            1 + (final_lap_slower_by_percent / 100) * slower_fraction
        )

        time_s = add_lap(
            rows, time_s, lap_id, lap_time_s, lane_length_m, sample_time_s, rng
        )
        if lap_id < laps:
            time_s = add_rest(rows, time_s, rest_between_laps_s, sample_time_s, rng)

    output_path.parent.mkdir(parents=True, exist_ok=True)
    with output_path.open("w", newline="", encoding="utf-8") as data_file:
        writer = csv.DictWriter(
            data_file,
            fieldnames=["timestamp", "ax", "ay", "az", "lap_id", "is_swimming"],
        )
        writer.writeheader()
        writer.writerows(rows)

    return len(rows)


def main() -> None:
    parser = argparse.ArgumentParser(description="Make clean temporary swimming IMU data.")
    parser.add_argument(
        "--output",
        type=Path,
        default=DEFAULT_OUTPUT,
        help=f"CSV file to create (default: {DEFAULT_OUTPUT.name})",
    )
    parser.add_argument("--laps", type=int, default=4, help="Number of laps (default: 4)")
    parser.add_argument("--seed", type=int, default=42, help="Random seed (default: 42)")
    args = parser.parse_args()

    row_count = generate_data(args.output, laps=args.laps, seed=args.seed)
    print(f"Created {args.output} with {row_count} rows.")
    print("Columns: timestamp, ax, ay, az, lap_id, is_swimming")


if __name__ == "__main__":
    main()

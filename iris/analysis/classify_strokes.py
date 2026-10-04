#!/usr/bin/env python3
"""Separate stroke families in the local IMU clips.

The files in dataset/ are short overlapping slices of one bout per stroke.
A random split would test the same strokes twice. This script keeps
non-overlapping windows and trains on the first half of each bout, then
tests on the second half.

Primary features use the gyroscope only, which is what a technique claim
can defend. A second model also sees mean acceleration, including gravity,
and is reported as a sensitivity check.

Right-wrist freestyle and backstroke are the same bouts as the left wrist.
They are not used for training. They are scored afterwards as a placement check.
"""

from __future__ import annotations

import csv
from datetime import datetime
from pathlib import Path

import numpy as np

ROOT = Path(__file__).resolve().parents[2]
DATA = ROOT / "dataset"
OUT = Path(__file__).resolve().parent / "offline_results.md"

PRIMARY = {
    "00 Breaststroke": ("breaststroke", "single"),
    "01 Butterfly": ("butterfly", "single"),
    "02 Backstroke (Left)": ("backstroke", "left"),
    "04 Front Crawl - Freestyle (Left)": ("freestyle", "left"),
}
RIGHT = {
    "03 Backstroke (Right)": "backstroke",
    "05 Front Crawl - Freestyle (Right)": "freestyle",
}
OTHER = {
    "06 Flip Turn - Somersault": "flip_turn",
    "07 Turn": "turn",
    "08 Push-off": "push_off",
    "09 Dives from Starting Block": "dive",
}
CLASSES = ["breaststroke", "butterfly", "backstroke", "freestyle"]
GYRO_NAMES = [
    "gyro_std_x",
    "gyro_std_y",
    "gyro_std_z",
    "gyro_energy_frac_x",
    "gyro_energy_frac_y",
    "gyro_energy_frac_z",
    "gyro_mag_mean",
    "gyro_mag_peak",
    "zero_cross_rate",
    "peak_freq_hz",
]


def parse_time(text: str) -> datetime:
    return datetime.strptime(text, "%H:%M:%S.%f")


def load_clip(path: Path) -> dict:
    with path.open(newline="") as handle:
        rows = list(csv.DictReader(handle))
    times = [parse_time(row["hh:mm:ss.ms"]) for row in rows]
    origin = times[0]
    sec = np.array([(t - origin).total_seconds() for t in times], dtype=float)
    gyro = np.column_stack(
        [
            [float(row["gyroX[mdps]"]) for row in rows],
            [float(row["gyroY[mdps]"]) for row in rows],
            [float(row["gyroZ[mdps]"]) for row in rows],
        ]
    ) / 1000.0
    acc = np.column_stack(
        [
            [float(row["accX[mg]"]) for row in rows],
            [float(row["accY[mg]"]) for row in rows],
            [float(row["accZ[mg]"]) for row in rows],
        ]
    ) / 1000.0
    return {
        "path": path.name,
        "t0": times[0],
        "t1": times[-1],
        "sec": sec,
        "gyro": gyro,
        "acc": acc,
    }


def gyro_features(clip: dict) -> np.ndarray:
    gyro = clip["gyro"]
    sec = clip["sec"]
    dt = float(np.median(np.diff(sec))) if len(sec) > 1 else 0.005
    dur = max(float(sec[-1] - sec[0]), dt)
    std = gyro.std(axis=0)
    energy = std ** 2
    frac = energy / max(float(energy.sum()), 1e-9)
    mag = np.linalg.norm(gyro, axis=1)
    dominant = gyro[:, int(np.argmax(energy))]
    signs = np.sign(dominant)
    signs[signs == 0] = 1
    zero_cross = float(np.count_nonzero(np.diff(signs))) / dur
    centered = mag - mag.mean()
    spec = np.abs(np.fft.rfft(centered))
    freqs = np.fft.rfftfreq(len(mag), d=dt)
    band = (freqs >= 0.3) & (freqs <= 3.0)
    peak = float(freqs[band][np.argmax(spec[band])]) if np.any(band) else 0.0
    return np.array(
        [
            std[0],
            std[1],
            std[2],
            frac[0],
            frac[1],
            frac[2],
            float(mag.mean()),
            float(mag.max()),
            zero_cross,
            peak,
        ],
        dtype=float,
    )


def with_gravity(clip: dict, gyro_feat: np.ndarray) -> np.ndarray:
    return np.concatenate([gyro_feat, clip["acc"].mean(axis=0)])


def nonoverlapping(clips: list[dict]) -> list[dict]:
    kept = []
    last_end = None
    for clip in sorted(clips, key=lambda item: item["t0"]):
        if last_end is None or (clip["t0"] - last_end).total_seconds() >= -0.05:
            kept.append(clip)
            last_end = clip["t1"]
    return kept


def load_group(folder: str) -> list[dict]:
    clips = [load_clip(path) for path in sorted((DATA / folder).glob("*.csv"))]
    return nonoverlapping(clips)


def midpoint(clips: list[dict]) -> datetime:
    start = min(clip["t0"] for clip in clips)
    end = max(clip["t1"] for clip in clips)
    return start + (end - start) / 2


def standardize(train: np.ndarray, test: np.ndarray):
    mu = train.mean(axis=0)
    sigma = train.std(axis=0)
    sigma[sigma < 1e-6] = 1.0
    return (train - mu) / sigma, (test - mu) / sigma, mu, sigma


def centroids(train: np.ndarray, labels: list[str]) -> dict[str, np.ndarray]:
    centers = {}
    for name in CLASSES:
        rows = train[np.array([label == name for label in labels])]
        centers[name] = rows.mean(axis=0)
    return centers


def predict(points: np.ndarray, centers: dict[str, np.ndarray]) -> list[str]:
    names = CLASSES
    stack = np.vstack([centers[name] for name in names])
    distances = ((points[:, None, :] - stack[None, :, :]) ** 2).sum(axis=2)
    return [names[int(i)] for i in distances.argmin(axis=1)]


def accuracy(truth: list[str], guess: list[str]) -> float:
    if not truth:
        return float("nan")
    return float(np.mean([a == b for a, b in zip(truth, guess)]))


def confusion(truth: list[str], guess: list[str]) -> np.ndarray:
    matrix = np.zeros((len(CLASSES), len(CLASSES)), dtype=int)
    index = {name: i for i, name in enumerate(CLASSES)}
    for true, pred in zip(truth, guess):
        matrix[index[true], index[pred]] += 1
    return matrix


def recall_lines(matrix: np.ndarray) -> list[str]:
    lines = []
    for i, name in enumerate(CLASSES):
        total = int(matrix[i].sum())
        hit = int(matrix[i, i])
        rate = hit / total if total else float("nan")
        lines.append(f"- {name}: {hit}/{total} = {rate:.3f}")
    return lines


def matrix_table(matrix: np.ndarray) -> str:
    header = "| true \\ predicted | " + " | ".join(CLASSES) + " |"
    rule = "| --- | " + " | ".join(["---"] * len(CLASSES)) + " |"
    rows = [header, rule]
    for i, name in enumerate(CLASSES):
        rows.append("| " + name + " | " + " | ".join(str(int(v)) for v in matrix[i]) + " |")
    return "\n".join(rows)


def evaluate(train_x, train_y, test_x, test_y):
    z_train, z_test, _, _ = standardize(train_x, test_x)
    centers = centroids(z_train, train_y)
    guess = predict(z_test, centers)
    majority = max(CLASSES, key=lambda name: train_y.count(name))
    baseline = [majority] * len(test_y)
    return {
        "accuracy": accuracy(test_y, guess),
        "baseline": accuracy(test_y, baseline),
        "majority": majority,
        "guess": guess,
        "matrix": confusion(test_y, guess),
        "centers": centers,
        "mu_sigma": standardize(train_x, test_x)[:2],
    }


def main() -> None:
    primary = []
    raw_primary = 0
    for folder, (label, wrist) in PRIMARY.items():
        raw_primary += len(list((DATA / folder).glob("*.csv")))
        for clip in load_group(folder):
            clip["label"] = label
            clip["wrist"] = wrist
            clip["gyro_feat"] = gyro_features(clip)
            clip["full_feat"] = with_gravity(clip, clip["gyro_feat"])
            primary.append(clip)

    by_class = {name: [c for c in primary if c["label"] == name] for name in CLASSES}
    mids = {name: midpoint(items) for name, items in by_class.items()}

    train, test = [], []
    for clip in primary:
        if clip["t0"] < mids[clip["label"]]:
            train.append(clip)
        else:
            test.append(clip)

    def pack(rows, key):
        return np.vstack([row[key] for row in rows]), [row["label"] for row in rows]

    gyro_train_x, train_y = pack(train, "gyro_feat")
    gyro_test_x, test_y = pack(test, "gyro_feat")
    full_train_x, _ = pack(train, "full_feat")
    full_test_x, _ = pack(test, "full_feat")

    gyro = evaluate(gyro_train_x, train_y, gyro_test_x, test_y)
    full = evaluate(full_train_x, train_y, full_test_x, test_y)
    top_count = max(train_y.count(name) for name in CLASSES)
    tied = [name for name in CLASSES if train_y.count(name) == top_count]
    if len(tied) == 1:
        baseline_sentence = (
            f"Majority-class baseline (always `{gyro['majority']}`): **{gyro['baseline']:.3f}**."
        )
    else:
        joined = " and ".join(tied)
        baseline_sentence = (
            f"Training counts tie between {joined} at {top_count} windows each. "
            f"Predicting `{gyro['majority']}` for every test window scores **{gyro['baseline']:.3f}**, "
            "the same rate as chance on this balanced four-class split."
        )

    right_lines = []
    for folder, label in RIGHT.items():
        windows = []
        for clip in load_group(folder):
            if clip["t0"] >= mids[label]:
                clip["gyro_feat"] = gyro_features(clip)
                windows.append(clip)
        if not windows:
            right_lines.append(f"- {label} right wrist: no second-half windows")
            continue
        points = np.vstack([clip["gyro_feat"] for clip in windows])
        # Reuse the gyro model's standardization from the primary training set.
        mu = gyro_train_x.mean(axis=0)
        sigma = gyro_train_x.std(axis=0)
        sigma[sigma < 1e-6] = 1.0
        z_train = (gyro_train_x - mu) / sigma
        centers = centroids(z_train, train_y)
        guess = predict((points - mu) / sigma, centers)
        truth = [label] * len(guess)
        right_lines.append(
            f"- {label} right wrist, second half: {accuracy(truth, guess):.3f} "
            f"on {len(guess)} windows"
        )

    holdout_lines = []
    mu = gyro_train_x.mean(axis=0)
    sigma = gyro_train_x.std(axis=0)
    sigma[sigma < 1e-6] = 1.0
    z_train = (gyro_train_x - mu) / sigma
    centers = centroids(z_train, train_y)
    for folder, label in OTHER.items():
        windows = load_group(folder)
        if not windows:
            continue
        points = np.vstack([gyro_features(clip) for clip in windows])
        guess = predict((points - mu) / sigma, centers)
        counts = {name: guess.count(name) for name in CLASSES}
        summary = ", ".join(f"{name} {counts[name]}" for name in CLASSES)
        holdout_lines.append(f"- {label}: {len(guess)} windows mapped to {summary}")

    count_lines = []
    for name in CLASSES:
        n_train = train_y.count(name)
        n_test = test_y.count(name)
        count_lines.append(f"- {name}: {n_train} train windows, {n_test} test windows")

    text = f"""# Offline stroke-family separation

Source clips: `{DATA}`.
Public set these folders follow: Ruslan Bredun, Swimming styles IMU data, Kaggle,
https://www.kaggle.com/datasets/ruslanbredun/swimming-styles-imu-data

The local copy uses English folder names. Timestamps show one continuous bout per
stroke family, sliced into overlapping windows of about 2.4–2.8 s. Left and right
freestyle, and left and right backstroke, occupy the same clock time.

Raw primary files (breaststroke, butterfly, backstroke left, freestyle left): {raw_primary}.
Non-overlapping windows kept: {len(primary)}.
Train windows (first half of each bout): {len(train)}.
Test windows (second half of each bout): {len(test)}.

Window counts:

{chr(10).join(count_lines)}

Classifier: nearest class centroid on z-scored features.
Features ({len(GYRO_NAMES)}): {", ".join(GYRO_NAMES)}.
Magnetometer channels were not used. The prototype's MPU-6050 does not have one.

## Result

Gyro-only accuracy on the second half of each bout: **{gyro["accuracy"]:.3f}**.
{baseline_sentence}

Confusion matrix, rows are the true stroke, columns are the prediction:

{matrix_table(gyro["matrix"])}

Recall:

{chr(10).join(recall_lines(gyro["matrix"]))}

Adding mean acceleration, which includes gravity and therefore how the sensor sat
on the wrist: accuracy **{full["accuracy"]:.3f}** against the same baseline
**{full["baseline"]:.3f}**. That number is easier, and it is not the primary result.

## Right wrist, same bouts

These windows were not in the training set. They are still the same swimmer and
the same lengths, recorded from the other wrist.

{chr(10).join(right_lines)}

## Other clip types

These were not given a fifth class. The four-stroke centroids assign them as follows.
This is a description, not an accuracy.

{chr(10).join(holdout_lines)}

## What this does not show

One bout per stroke cannot support a claim about other swimmers, other pools, or
fatigue. Overlapping slices were removed so the test windows are not copies of the
training windows, but the two halves are still the same swim. Pool logs from the
AquaCue prototype are a separate experiment.
"""
    OUT.write_text(text)
    print(text)
    print(f"wrote {OUT}")


if __name__ == "__main__":
    main()

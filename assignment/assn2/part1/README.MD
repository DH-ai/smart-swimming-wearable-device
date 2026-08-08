# Assignment 2 — Part 1: Swimming Metrics

## What is this part about?

We have data from a swimming wearable. It records the time and the movement of
the swimmer. In Part 1, we use that data to answer simple questions:

- How far did the swimmer go?
- How fast did they swim?
- Did they keep a steady speed?
- Did they swim in a straight line?
- Did they get slower near the end?

There are **20 metrics** (measurements) in total. The Kaggle data can be added
later; this file explains what we will calculate from it.

For the assignment problem statement, see
[PROBLEM_STATEMENT.md](PROBLEM_STATEMENT.md).

## Simple assumptions

To keep this part simple, we assume:

1. The sensor data is already cleaned and calibrated.
2. `+Y` means **forward** (towards the other end of the pool).
3. `X` means left/right movement across the lane.
4. The swimmer starts from rest, so starting speed is zero.
5. A lap is one swimming length. A short rest or turn separates laps.

We do **not** need GPS, a camera, or sensor fusion for this part. We estimate
speed and position from the acceleration data. This can slowly become less
accurate over a long time, so later we should check the answer against the
known pool length or lap labels if the dataset provides them.

## Data needed

The data file should have one row for every sensor reading.

| Column | What it means | Unit |
| --- | --- | --- |
| `timestamp` | When the reading was taken | seconds |
| `ax` | Acceleration left/right | m/s² |
| `ay` | Acceleration forward/back | m/s² |
| `az` | Acceleration up/down | m/s² |

If the Kaggle data uses different column names, we can rename them in the code.

## The 20 measurements

### 1. Performance — distance and speed

| # | Measurement | What it tells us | How to calculate it |
| --- | --- | --- | --- |
| 1 | Total distance | Total distance travelled | Add `speed × time` for every small time gap. |
| 2 | Number of laps | Number of pool lengths | Count the detected laps. |
| 3 | Total swimming time | Time spent swimming | Add the time for swimming rows. Do not count rest time. |
| 4 | Average speed | Usual speed for the session | `total distance / total swimming time` |
| 5 | Maximum speed | Fastest moment | Largest speed value. |
| 6 | Minimum speed | Slowest swimming moment | Smallest speed value while swimming. |
| 7 | Median speed | Middle speed | Put all speeds in order and take the middle one. |

### 2. Consistency — was the pace steady?

| # | Measurement | What it tells us | How to calculate it |
| --- | --- | --- | --- |
| 8 | Speed standard deviation | How much speed changes | A small value means steadier speed. Python can calculate it with `.std()`. |
| 9 | Speed variance | Another way to show speed changes | Standard deviation × standard deviation. |
| 10 | Lap-time standard deviation | Whether lap times are similar | Find the standard deviation of all lap times. A small value means similar laps. |

### 3. Swimming line — did the swimmer go straight?

| # | Measurement | What it tells us | How to calculate it |
| --- | --- | --- | --- |
| 11 | Mean heading deviation | Average turning away from forward | Find the angle between movement and `+Y`, then average its size. |
| 12 | Maximum heading deviation | Biggest turn away from forward | Largest heading angle. |
| 13 | Heading-deviation standard deviation | Whether direction changed a lot | Find the standard deviation of the heading angles. |
| 14 | Mean lateral deviation | Average distance away from lane centre | Average `abs(x position - lane centre)`. |
| 15 | Maximum lateral deviation | Furthest distance away from lane centre | Largest `abs(x position - lane centre)`. |

`0°` heading deviation means the swimmer moved exactly forward. Smaller heading
and lateral values usually mean a straighter swimming line.

### 4. Motion — what did the sensor feel?

| # | Measurement | What it tells us | How to calculate it |
| --- | --- | --- | --- |
| 16 | Mean acceleration magnitude | Average amount of movement | Average of `sqrt(ax² + ay² + az²)`. |
| 17 | Maximum acceleration magnitude | Strongest movement | Largest acceleration magnitude. |
| 18 | Acceleration standard deviation | How much movement changed | Standard deviation of acceleration magnitude. |

### 5. Getting tired — did performance drop?

| # | Measurement | What it tells us | How to calculate it |
| --- | --- | --- | --- |
| 19 | Lap-time degradation (%) | Whether late laps take longer | `((late lap time - early lap time) / early lap time) × 100` |
| 20 | Speed degradation (%) | Whether late laps are slower | `((early speed - late speed) / early speed) × 100` |

For the last two measurements, compare the first few laps with the last few
laps. A positive result means the swimmer slowed down. If there is only one
lap, write `N/A` because there is nothing to compare.

## How the calculation works

```text
sensor data
    ↓
find the time gap between rows
    ↓
use acceleration to estimate speed and position
    ↓
find swimming parts, rests, and laps
    ↓
calculate the 20 measurements
```

### Step 1: Find the time gap

For each row, subtract the previous time from the current time:

```text
time_gap = current_timestamp - previous_timestamp
```

### Step 2: Estimate speed and position

Acceleration tells us how speed changes. For each small time gap:

```text
new speed = old speed + acceleration × time_gap
new position = old position + speed × time_gap
```

We do this for X, Y, and Z. For swimming speed, use the X and Y directions:

```text
speed = sqrt(x_speed² + y_speed²)
```

### Step 3: Find laps

At first, use a simple rule: the swimmer is active when speed is above a small
chosen value, such as `0.20 m/s`. A rest or turn is when the speed stays below
that value for a short time. Each active part between rests is one lap.

The exact speed value must be chosen after we see the Kaggle data. If the data
already has lap labels, use them instead and compare them with the automatic
lap count.

## Example Python code ( the example code is in python becuase if it would be in C++ things would be easier )

This is an example, not the final program. It shows the main calculations in
simple code. The temporary generator creates a no-header text file with six
space-separated values per line: `timestamp ax ay az lap_id is_swimming`.
 If you want to test the below code and get a feel for it install `pandas` and `numpy` when you are ready to run it.

```python
import numpy as np
import pandas as pd

# Load temporary text data. There is no header line in this file.
df = pd.read_csv(
    "synthetic_swimming_data.txt",
    sep=r"\s+",
    header=None,
    names=["timestamp", "ax", "ay", "az", "lap_id", "is_swimming"],
)
# For a later Kaggle CSV, use this instead:
# df = pd.read_csv("imu_data.csv")
df = df.sort_values("timestamp").dropna()

# Time between two sensor readings.
df["dt"] = df["timestamp"].diff().fillna(0)

# Estimate speed in each direction. We assume starting speed is zero.
for axis in ["x", "y", "z"]:
    df[f"v{axis}"] = (df[f"a{axis}"] * df["dt"]).cumsum()
    df[axis] = (df[f"v{axis}"] * df["dt"]).cumsum()

# Speed in the pool plane and total acceleration.
df["speed"] = np.sqrt(df["vx"] ** 2 + df["vy"] ** 2)
df["acceleration_size"] = np.sqrt(
    df["ax"] ** 2 + df["ay"] ** 2 + df["az"] ** 2
)

# Simple swimming rule. Change 0.20 after checking the real data.
df["is_swimming"] = df["speed"] >= 0.20
df["swim_dt"] = df["dt"].where(df["is_swimming"], 0)
swim = df[df["is_swimming"]].copy()
swim["distance_step"] = swim["speed"] * swim["swim_dt"]

# Direction: 0 degrees means straight forward (+Y).
swim["heading_deg"] = np.degrees(np.arctan2(swim["vx"], swim["vy"]))
swim["lateral_distance"] = abs(swim["x"])  # lane centre is X = 0

# Performance, consistency, line, and IMU measurements.
total_distance = swim["distance_step"].sum()
total_time = swim["swim_dt"].sum()
results = {
    "total_distance_m": total_distance,
    "total_swimming_time_s": total_time,
    "average_speed_mps": total_distance / total_time,
    "maximum_speed_mps": swim["speed"].max(),
    "minimum_speed_mps": swim["speed"].min(),
    "median_speed_mps": swim["speed"].median(),
    "speed_std_mps": swim["speed"].std(ddof=0),
    "speed_variance": swim["speed"].var(ddof=0),
    "mean_heading_deviation_deg": swim["heading_deg"].abs().mean(),
    "maximum_heading_deviation_deg": swim["heading_deg"].abs().max(),
    "heading_deviation_std_deg": swim["heading_deg"].std(ddof=0),
    "mean_lateral_deviation_m": swim["lateral_distance"].mean(),
    "maximum_lateral_deviation_m": swim["lateral_distance"].max(),
    "mean_acceleration_mps2": swim["acceleration_size"].mean(),
    "maximum_acceleration_mps2": swim["acceleration_size"].max(),
    "acceleration_std_mps2": swim["acceleration_size"].std(ddof=0),
}

print(pd.Series(results))
```

### Example code for lap measurements and tiredness

After we have a `lap_id` for every row (from lap labels or the rest/turn rule),
this code calculates the remaining four lap-based values.

```python
# Each lap gets one row with its time, distance, and average speed.
laps = swim.groupby("lap_id").agg(
    lap_time_s=("swim_dt", "sum"),
    lap_distance_m=("distance_step", "sum"),
)
laps["average_speed_mps"] = laps["lap_distance_m"] / laps["lap_time_s"]

results["number_of_laps"] = len(laps)
results["lap_time_std_s"] = laps["lap_time_s"].std(ddof=0)

if len(laps) >= 2:
    early = laps.iloc[0]
    late = laps.iloc[-1]
    results["lap_time_degradation_pct"] = (
        (late["lap_time_s"] - early["lap_time_s"]) / early["lap_time_s"] * 100
    )
    results["speed_degradation_pct"] = (
        (early["average_speed_mps"] - late["average_speed_mps"])
        / early["average_speed_mps"] * 100
    )
else:
    results["lap_time_degradation_pct"] = "N/A"
    results["speed_degradation_pct"] = "N/A"
```

> The example uses the first and last lap to stay easy to understand. For a
> longer session, we can average the first few laps and last few laps instead.

## What to submit later

When the data is available, submit:

1. The C++ file .
2. One table with all 20 measurements for each swimming session, we have only 1.
3. A small lap table: lap number, lap time, lap distance, and average speed.
4. A short note explaining the chosen active-speed value and any problems in
   the data.

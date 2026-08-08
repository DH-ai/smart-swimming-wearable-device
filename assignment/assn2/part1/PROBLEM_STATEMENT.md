# Problem Statement — Smart Swimming Wearable, Part 1

## The problem

A swimming wearable collects movement data while a person swims. Raw sensor
numbers are hard to understand by themselves. For example, a list of X, Y, and
Z acceleration values does not directly tell us if the swimmer was fast,
steady, straight, or tired.

The task is to change that raw data into simple measurements that describe a
swimming session.

## Main goal

Use time-stamped acceleration data to calculate 20 measurements for one swim.
The measurements should show:

- **Performance:** distance, laps, time, and speed;
- **Consistency:** whether speed and lap times stay similar;
- **Swimming line:** whether the swimmer keeps moving straight forward;
- **Motion:** how much the sensor moves; and
- **Tiredness:** whether the swimmer slows down near the end.

## Input

The Kaggle dataset will be added later. We expect it to give us:

- the time of each reading;
- X, Y, and Z acceleration values; and
- if available, lap labels or pool length.

## Simple rules for this part

- The sensor data is already calibrated.
- `+Y` is the forward direction in the pool.
- `X` is the left/right direction.
- The swimmer starts at rest.
- One lap means one swimming length.
- We do not use GPS, video, or sensor fusion in Part 1.

## What the solution must do

1. Read and sort the sensor data by time.
2. Use acceleration and time to estimate speed and position.
3. Find when the swimmer is moving and when they are resting or turning.
4. Split the swim into laps.
5. Calculate all 20 measurements.
6. Show `N/A` if a measurement cannot be calculated. For example, tiredness
   cannot be compared when there is only one lap.

## Expected result

For every swimming session, the program should produce one clear result table.
It should contain all 20 measurements with units, such as metres, seconds,
metres per second, degrees, and percent. It should also produce a small table
for each lap with its time, distance, and average speed.

The full list of measurements, their simple explanations, formulas, and Python
examples are in [README.MD](README.MD).

## Limits of this part

The calculated values are estimates. Small sensor errors can grow when we use
acceleration to estimate speed and distance. Therefore, later we should compare
lap count and distance with known pool information if the Kaggle data provides
it. This Part 1 work is a simple starting point, not a perfect tracking system.

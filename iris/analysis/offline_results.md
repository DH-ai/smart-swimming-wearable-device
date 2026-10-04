# Offline stroke-family separation

Source clips: `/home/dhruv/obscureP/smart-swimming-wearable-device/dataset`.
Public set these folders follow: Ruslan Bredun, Swimming styles IMU data, Kaggle,
https://www.kaggle.com/datasets/ruslanbredun/swimming-styles-imu-data

The local copy uses English folder names. Timestamps show one continuous bout per
stroke family, sliced into overlapping windows of about 2.4–2.8 s. Left and right
freestyle, and left and right backstroke, occupy the same clock time.

Raw primary files (breaststroke, butterfly, backstroke left, freestyle left): 129.
Non-overlapping windows kept: 68.
Train windows (first half of each bout): 36.
Test windows (second half of each bout): 32.

Window counts:

- breaststroke: 10 train windows, 8 test windows
- butterfly: 9 train windows, 9 test windows
- backstroke: 7 train windows, 7 test windows
- freestyle: 10 train windows, 8 test windows

Classifier: nearest class centroid on z-scored features.
Features (10): gyro_std_x, gyro_std_y, gyro_std_z, gyro_energy_frac_x, gyro_energy_frac_y, gyro_energy_frac_z, gyro_mag_mean, gyro_mag_peak, zero_cross_rate, peak_freq_hz.
Magnetometer channels were not used. The prototype's MPU-6050 does not have one.

## Result

Gyro-only accuracy on the second half of each bout: **0.500**.
Training counts tie between breaststroke and freestyle at 10 windows each. Predicting `breaststroke` for every test window scores **0.250**, the same rate as chance on this balanced four-class split.

Confusion matrix, rows are the true stroke, columns are the prediction:

| true \ predicted | breaststroke | butterfly | backstroke | freestyle |
| --- | --- | --- | --- | --- |
| breaststroke | 6 | 2 | 0 | 0 |
| butterfly | 8 | 1 | 0 | 0 |
| backstroke | 5 | 0 | 1 | 1 |
| freestyle | 0 | 0 | 0 | 8 |

Recall:

- breaststroke: 6/8 = 0.750
- butterfly: 1/9 = 0.111
- backstroke: 1/7 = 0.143
- freestyle: 8/8 = 1.000

Adding mean acceleration, which includes gravity and therefore how the sensor sat
on the wrist: accuracy **0.719** against the same baseline
**0.250**. That number is easier, and it is not the primary result.

## Right wrist, same bouts

These windows were not in the training set. They are still the same swimmer and
the same lengths, recorded from the other wrist.

- backstroke right wrist, second half: 0.286 on 7 windows
- freestyle right wrist, second half: 1.000 on 8 windows

## Other clip types

These were not given a fifth class. The four-stroke centroids assign them as follows.
This is a description, not an accuracy.

- flip_turn: 16 windows mapped to breaststroke 0, butterfly 1, backstroke 0, freestyle 15
- turn: 23 windows mapped to breaststroke 0, butterfly 0, backstroke 21, freestyle 2
- push_off: 43 windows mapped to breaststroke 7, butterfly 3, backstroke 14, freestyle 19
- dive: 16 windows mapped to breaststroke 0, butterfly 0, backstroke 12, freestyle 4

## What this does not show

One bout per stroke cannot support a claim about other swimmers, other pools, or
fatigue. Overlapping slices were removed so the test windows are not copies of the
training windows, but the two halves are still the same swim. Pool logs from the
AquaCue prototype are a separate experiment.

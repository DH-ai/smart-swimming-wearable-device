# Bench protractor procedure

Do this before any pool session. It checks the angle estimator that the paper describes. The pool does not replace it.

## Mount

1. Tape the GY-521 flat on a ruler. The chip faces up.
2. Put the ruler’s end at the protractor’s origin so the board can tilt like a hinged lid.
3. In the serial monitor at 115200 baud, with the board flat, `az_g` should be near +1 or −1 and larger in magnitude than `ax_g` and `ay_g`. If X or Y is the large one, remount until Z is the up axis.
4. The swim filter’s bench check uses rotation about the board’s Y axis (pitch). Turn the ruler the way that makes `angle_deg` increase. Use that direction for every protractor setting.

## Capture

1. Hold the BOOT/button GPIO (the button on GPIO 27) down, press the ESP32 reset button, and keep holding for about a second. The serial line `mode=bench` confirms it.
2. Leave the board still and flat until it prints that the zero is locked. That pose is 0°.
3. For each setting 0°, 30°, 45°, and 90°, hold the ruler steady for about two seconds, then short-press the button.
4. Copy each `BENCH` line from the serial monitor. Write the protractor’s number yourself. The firmware does not know it.

Save the four rows as `iris/protocol/bench-results.csv` with this header:

```
nominal_deg,mean_deg,sd_deg
```

Then run:

```
python3 iris/analysis/analyze_bench.py iris/protocol/bench-results.csv
```

Paste the printed RMSE and the four errors into the bench table in `paper/paper.md`.

## What a usable result looks like

The error at each setting should be a few degrees, not tens of degrees. A consistent sign flip means the ruler was turned the wrong way; repeat the four poses. A growing error only at 90° is still reportable: say so, and do not average it away.

This bench uses a quiet tilt. During a real stroke the accelerometer is also measuring motion, so the firmware trusts the gyroscope until the acceleration magnitude is near 1 g again. The protractor test does not prove the swimming case. It proves the filter, the axis, and the sign before you depend on them.

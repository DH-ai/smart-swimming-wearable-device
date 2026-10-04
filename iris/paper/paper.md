# AquaCue: a forearm IMU that counts freestyle strokes and cues a drift in wrist angle

Category: Embedded Systems (IRIS National Fair)

Student name:

Class and school:

## Abstract

AquaCue is a forearm-mounted logger built from an ESP32, an MPU-6050, a microSD card, and a vibration motor. On a freestyle length it counts strokes of the instrumented arm and pulses the motor when the wrist angle at those strokes drifts away from that swimmer’s own first strokes of the session. The angle is the tilt estimated at the gyro peak. It is a wrist angle at a defined event, and the pool protocol tests whether that angle changes when the swimmer deliberately raises the hand entry.

A desktop test checks the filter, the stroke detector, and the cue rule. A protractor procedure checks the angle on the real board. One pool session compares easy freestyle with a higher hand entry, using deck video as the stroke count. Those two hardware results are filled in from the log files after the session. They are not estimated here.

Separately, 68 non-overlapping windows from a public IMU clip set were split by time within each bout. A gyro-only nearest-centroid classifier scored 0.500 on the second half of each bout. The chance baseline on that split was 0.250. All 8 freestyle test windows were classified as freestyle. Butterfly and backstroke were not separated cleanly. The pool device therefore assumes freestyle, which the protocol enforces, and does not try to name the stroke.

## 1. Question

A swimmer can hold a stroke rate that looks busy while each stroke covers less of the pool. Wrist-worn watches often report laps and stroke rate. They do not, by themselves, show whether the wrist angle at the start of the stroke is staying consistent.

The question for this prototype is narrower than a full technique diagnosis:

On freestyle, can a single forearm IMU count arm strokes in agreement with video, and can a haptic cue fire when the wrist angle at the stroke event shifts under a deliberate change of entry, while staying quiet on easy swimming?

Fatigue, propulsive force, and speed through the water are outside this experiment. One session cannot separate fatigue from pace and turns. Speed from double-integrating a wrist accelerometer is not used. Dadashi, Crettenand, Millet, and Aminian showed that even a sacrum-mounted IMU needs biomechanical constraints and a tethered reference to estimate front-crawl velocity [1].

## 2. What the device measures

The prototype straps to the forearm. The MPU-6050 is fixed to the shell face that bears on the arm. A loose board is treated as a failed trial, because the angle would then include motion of the board inside the box.

Each accepted stroke is one peak of the dominant gyroscope axis, in the direction chosen during ten practice swings, with a refractory period of 0.45 s so a single stroke is not counted twice. The stored angle is the Kalman tilt at that peak.

The personal baseline is the circular mean of the first eight stroke angles after the swimmer presses the button to start. A cue is a 200 ms vibration, mirrored by an LED so a deck camera can see it during the arm recovery. The cue fires only after two strokes in a row fall outside the threshold, and then waits 5 s. The threshold is the larger of 8° and two baseline standard deviations.

Three blocks of four lengths test that rule: easy, deliberately higher hand entry, easy again. The higher entry is the controlled change. The second easy block asks whether the cue goes quiet again.

## 3. Related work

Delhaye and colleagues used a single IMU and a neural network to recognise swimming activity and assess lap time, with video and stopwatch references in the protocol [2]. Their result is on a sacrum-mounted sensor and a model that sees the whole window. It supports the idea that an IMU can recognise swimming, and it is not a wrist-angle result that this project can copy.

Bosch Sensortec has described embedded swim analytics that report style, length, and stroke count from a wearable IMU [3]. That is a useful comparison for stroke counting. It is not a measurement of propulsive efficiency.

The offline section below uses the public Swimming styles IMU clips released by Bredun [4]. In the copy stored with this project the folders are named in English. The timestamps show one bout per stroke, cut into overlapping slices of about 2.4 s to 2.8 s.

## 4. Hardware

The circuit is an ESP32 development board, a GY-521 breakout (MPU-6050), an SPI microSD module, a coin vibration motor driven by a transistor, a white LED, and a 5 V supply from a power bank or from a LiPo through a boost converter. The LiPo is not connected to the 3.3 V pin. The SD card’s chip-select is GPIO 14 so a boot-strapping pin is not held by the card. Wiring, the bill of materials, and the seal test are in the project protocol notes.

The accelerometer full scale is ±8 g (4096 LSB/g). The gyroscope full scale is ±1000 °/s (32.768 LSB per °/s). A digital low-pass filter is enabled and the sensor sample divider is set for 100 Hz. The firmware also schedules its own 100 Hz loop and uses the real interval in the filter.

Samples are written to a CSV file on the card: time, acceleration in g, angular rate in °/s, filtered angle in degrees, stroke flag, haptic flag, block mark, and mode. The pool session can be re-analysed from that file. Feedback does not depend on a radio. The motor decision is made on the board.

A printed shell is in `cad/forearm_pod.scad`. The internal cavity is 102 × 58 × 32 mm, with a pocket for the GY-521 and a recess for the motor. A sealed food box is an acceptable substitute if the sensor is still fixed to the face on the arm and the box passes a 10-minute bucket test. Neither container is sold as a waterproof rating.

## 5. Angle estimator

The filter state is the tilt angle θ and the gyroscope bias b, both in radians.

While the arm is held still for two seconds (200 quiet samples at 100 Hz), the firmware stores the mean gyroscope reading and subtracts it from later samples. The filter’s own bias state then tracks whatever offset remains.

Prediction over a step Δt, with measured rate ω:

θ_pred = θ + (ω − b) Δt

b_pred = b

The accelerometer measurement depends on which axis dominated the practice swings.

- Rotation about X: θ_acc = atan2(a_y, a_z)
- Rotation about Y: θ_acc = atan2(−a_x, sqrt(a_y² + a_z²))
- Rotation about Z is yaw. Gravity does not observe it. If practice selects Z, the firmware warns and integrates the gyroscope without an accelerometer correction. That mounting should be redone before the pool.

The innovation is wrapped into (−π, π] so a crossing of ±180° is not treated as a huge error:

y = wrap(θ_acc − θ_pred)

The update adds a fraction of that innovation. The fraction is the Kalman gain. If the measurement is larger than the prediction, the angle increases:

θ_new = θ_pred + K_θ y

b_new = b_pred + K_b y

K comes from the state covariance and from the measurement variance R. Process noise is Q_θ = 0.001 and Q_b = 0.003, with R = 0.03, in radian units, the usual starting point for this one-axis tilt filter. When the acceleration magnitude is within 0.25 g of 1 g, R is used as-is. When the magnitude is far from 1 g, R is increased by up to 40 times, because the accelerometer is then measuring the stroke, not gravity. During the fast part of a stroke the angle follows the gyroscope. When the arm is quieter, gravity pulls the estimate back.

The same update is compiled into a desktop test. That test checks the update sign, a 30° pitch constructed from accelerometer components, recovery of a constant gyroscope bias, rejection of a second peak inside the refractory interval, and the two-stroke cue with its 5 s cooldown. On the device the refractory interval is 0.45 s. Passing the desktop test means the code matches these equations. It does not mean the angle is correct on a swimmer. The protractor does that job.

## 6. Hypotheses for the pool session

H1. On the easy blocks, the device stroke count is within 10% of the video count for the instrumented arm.

H2. The mean event angle in the higher-entry block differs from the eight-stroke baseline by more than the baseline standard deviation.

H3. The haptic rate in the higher-entry block is higher than the haptic rate in the easy blocks.

H1 can fail if the threshold is wrong for that swimmer or the strap moves. H2 can fail if a “higher entry” does not change the wrist angle at the gyro peak, which is a real and reportable outcome. H3 can fail even if H2 holds, if the shift is smaller than the cue threshold. Those are results, not reasons to change the definition after seeing the video.

## 7. Methods

### 7.1 Bench

The GY-521 is taped flat on a ruler, chip up, Z axis up (the axis nearest ±1 g while flat). Bench mode is selected by holding the button during reset. After one quiet second the current pitch is zero. The ruler is tilted about Y, in the direction that makes the printed angle increase, to 0°, 30°, 45°, and 90°. Each pose is held and captured with the button. `analyze_bench.py` computes the error of each pose and the root-mean-square error. The sheet is `protocol/bench-results.csv`.

### 7.2 Pool

One swimmer, freestyle only, pool length recorded on the tally. A phone on the deck films the lane. The LED faces away from the arm.

After the still calibration and ten practice swings, the button is pressed at the wall four times: before four easy lengths, before four lengths with a deliberately higher hand entry, before four easy lengths, and once to stop. Rests are taken on the wall with the arms still, before the next press. The wall flash marks the block. It is not counted as a cue.

Video counts are strokes of the instrumented arm only, excluding the push-off, and LED flashes during the length. If a flash was not visible, the cell is left blank. `analyze_session.py` splits the CSV on the four marks and prints stroke-count error, event-angle mean and standard deviation, and haptic counts. The first eight strokes are the baseline and are excluded from the easy-block angle mean.

### 7.3 Offline clips

Primary windows are breaststroke, butterfly, backstroke (left wrist), and freestyle (left wrist). Overlapping slices were dropped by keeping a window only when it starts after the previous kept window ends. Within each stroke, the first half of the bout is the training set and the second half is the test set. Features are gyroscope standard deviations, energy fractions, magnitude mean and peak, zero-crossing rate, and the dominant frequency between 0.3 Hz and 3 Hz. Magnetometer axes are ignored so the features match an MPU-6050. The classifier is a nearest centroid after z-scoring on the training windows. The baseline predicts the most common training label for every test window. Right-wrist windows from the same bouts are scored afterwards and are not used to train. Turns, push-offs, and dives are not given a class; the report only says which centroid they fall nearest.

## 8. Results

### 8.1 Offline separation

From 129 raw primary files, 68 non-overlapping windows remained. Training used 36 and testing used 32.

| Stroke | Train windows | Test windows | Correct |
| --- | ---: | ---: | ---: |
| Breaststroke | 10 | 8 | 6 |
| Butterfly | 9 | 9 | 1 |
| Backstroke | 7 | 7 | 1 |
| Freestyle | 10 | 8 | 8 |

Gyro-only accuracy: 0.500 (16 of 32). Breaststroke and freestyle tied for the largest training count, at 10 windows. Predicting breaststroke for every test window scores 0.250, which is the chance rate on this balanced split.

The 8 freestyle test windows were all labelled freestyle. The right-wrist freestyle windows from the second half of the same bout, which were not in training, were also 8 of 8. Butterfly was labelled breaststroke in 8 of 9 test windows. Backstroke was correct in 1 of 7, and the right wrist scored 0.286. Adding the mean acceleration, which includes gravity and therefore how the sensor was worn, raised accuracy to 0.719. That figure is reported so the gravity shortcut is visible. It is not the result used to justify the device.

The full matrix and the hold-out description are in `analysis/offline_results.md`, produced by `analysis/classify_strokes.py`.

### 8.2 Protractor

Run `python3 iris/analysis/analyze_bench.py iris/protocol/bench-results.csv` and paste the table and the RMSE here. Leave this section empty until that file has the four poses. Do not copy an error from another project.

| Nominal (deg) | Device mean (deg) | Error (deg) | SD (deg) |
| ---: | ---: | ---: | ---: |
| 0 | | | |
| 30 | | | |
| 45 | | | |
| 90 | | | |

RMSE: ________ deg

### 8.3 Pool session

Paste the report from `analyze_session.py` after the twelve lengths. The synthetic file in `analysis/testdata/` is a software check of the report script. Its angles and counts must not be copied into this table.

| Block | Device strokes | Video strokes | Stroke error | Event-angle mean (deg) | SD (deg) | Device haptics | Video flashes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| Baseline (first 8) | 8 | | | | | | |
| Easy 1, after baseline | | | | | | | |
| Higher entry | | | | | | | |
| Easy 2 | | | | | | | |

H1 (easy counts within 10%): ________

H2 (angle shift larger than baseline SD): ________

H3 (higher haptic rate on the altered block): ________

Pool length: ________ m. Date: ________. Notes on strap, visibility, and any length that was stopped:

## 9. Discussion

The offline test answers a different question from the pool. It asks whether short gyro windows from one recording of each stroke are separable. Freestyle was separable on both wrists of that recording. Butterfly was not separable from breaststroke with these features. That is why AquaCue does not claim a four-stroke classifier. The pool protocol is freestyle only, and the stroke count is checked against video rather than against the clip labels.

A time split inside one bout is stricter than shuffling overlapping slices, and it is still the same swim. Nothing in section 8.1 estimates accuracy on a new swimmer.

The pool hypotheses are descriptive of one swimmer and one outing. Strokes inside a length are not independent samples, so this paper does not attach a p-value to them. A shift that is large compared with the baseline scatter is evidence for this session. A shift that is small means the gyro-peak angle did not track the change the swimmer intended, which is useful to know before adding sensors.

The cue can be wrong in two directions. A loose strap or a turn can look like a new angle. A real change in entry can leave the wrist angle at the gyro peak almost unchanged. The LED and the video exist so those cases can be seen, not so the log can be edited to match the hypothesis.

Further work, after this submission’s data exist, would be a second swimmer, a repeat of the same protocol on another day, and only then a sensor that can see the hand rather than the forearm. A pressure sensor or a second IMU on the hand is that kind of addition. It is not required to test H1–H3.

## 10. Conclusion

AquaCue logs forearm acceleration and angular rate, counts freestyle stroke events on the board, and vibrates when the wrist angle at those events leaves the swimmer’s own early baseline. The filter update, the refractory period, and the cue rule match a desktop test. On the public clips, gyro features alone recognised the freestyle windows in this copy of the data and did not reliably separate butterfly from breaststroke. The protractor error and the twelve-length pool comparison are the measurements that complete the project, and they belong in section 8 only after the CSV files exist.

## References

[1] F. Dadashi, F. Crettenand, G. P. Millet, and K. Aminian, “Front-crawl instantaneous velocity estimation using a wearable inertial measurement unit,” *Sensors*, vol. 12, no. 10, pp. 12927–12939, 2012. https://doi.org/10.3390/s121012927

[2] E. Delhaye, A. Bouvet, G. Nicolas, J. P. Vilas-Boas, B. Bideau, and N. Bideau, “Automatic swimming activity recognition and lap time assessment based on a single IMU: a deep learning approach,” *Sensors*, vol. 22, no. 15, art. 5786, 2022. https://doi.org/10.3390/s22155786

[3] Bosch Sensortec, “Swim analytics,” white paper, November 2020. https://www.bosch-sensortec.com/media/boschsensortec/downloads/white_papers/white-paper_ai-swimming_november-2020.pdf

[4] R. Bredun, “Swimming styles IMU data,” Kaggle. https://www.kaggle.com/datasets/ruslanbredun/swimming-styles-imu-data

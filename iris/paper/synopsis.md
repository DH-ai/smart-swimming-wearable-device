# Synopsis — AquaCue

Category: Embedded Systems

Student name, class, school:

AquaCue is a forearm logger that counts freestyle strokes and vibrates when the swimmer’s wrist angle at those strokes drifts from their own baseline. The prototype is an ESP32, an MPU-6050 accelerometer and gyroscope, a microSD card, and a vibration motor with an LED that flashes on the same cue. Decisions are made on the device. The card stores the raw samples so the session can be checked later.

The angle comes from a one-axis Kalman filter. The gyroscope predicts the next tilt, including a bias term. The accelerometer provides a tilt measurement from gravity, and the filter adds a fraction of the difference between that measurement and the prediction. During a stroke, when acceleration is far from 1 g, the accelerometer is trusted less, because it is measuring motion as well as gravity. The stroke event is a peak on the gyroscope axis that moved most during ten practice swings. The baseline is the average angle of the first eight events. The motor pulses only after two events in a row pass a threshold, and then it waits five seconds.

The pool test is freestyle in a known pool: four easy lengths, four lengths with a deliberately higher hand entry, and four easy lengths again. Deck video is the count for strokes and for visible flashes. The higher entry is the change under test. Fatigue is not claimed, because one outing cannot separate tiredness from pace and turns. The device does not estimate speed by integrating acceleration. Published sacrum-IMU work already shows that velocity needs extra constraints and a reference measurement.

A separate check used public swimming IMU clips (Bredun, Kaggle). Overlapping slices were removed, and each stroke bout was split in time so the test windows were the second half of the same recording. Using gyroscope features only, a nearest-centroid classifier scored 0.500 against a 0.250 chance baseline. Every freestyle test window was classified as freestyle, on both wrists. Butterfly was usually labelled breaststroke. AquaCue therefore does not try to name the stroke in the pool. The protocol is freestyle only.

The protractor errors and the pool counts are recorded from the device logs and the video tally. Those numbers are part of the full paper’s results tables once the session has been run. The engineering already fixed in the project is the sensing circuit, the on-device cue, the logging format, and the analysis scripts that turn a log into stroke error, angle shift, and haptic rate.

Keywords: freestyle, IMU, Kalman filter, haptic feedback, stroke count

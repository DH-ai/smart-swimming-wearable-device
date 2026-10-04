# Pool protocol

One swimmer, freestyle only, one session. Do the protractor bench and the bucket test first.

The controlled change is a deliberately higher hand entry, not fatigue. One outing cannot separate fatigue from pace, turns, and technique.

## Setup

- Known pool length. Write it on the tally sheet. A 25 m pool is easier to film than a 50 m pool.
- Phone fixed on the deck, side view, the whole lane in frame. Start the video before the first button press.
- Pod strapped tight. LED visible off the back of the forearm.
- A second person runs the camera and the tally. The swimmer does not count their own strokes from memory after the fact if the video exists.

## On-deck calibration

1. Boot the device in normal mode (do not hold the button).
2. Hold the forearm still until the motor buzzes once. That is the two-second gyro bias calibration. If you move, it starts over.
3. Do 10 slow, deliberate freestyle arm recoveries. Each accepted swing flashes the LED. The serial monitor, if you still have USB, prints the axis and the threshold.
4. If the monitor says the dominant axis is Z, remount so the biggest rotation is roll or pitch and reboot. Z is yaw. Gravity cannot correct it.
5. Hold a neutral pose for a few seconds so the angle can settle. Then get in at the wall.

## Twelve lengths

Press the button only while holding the wall, immediately before the push-off, except the last press which ends the log. Each press flashes the LED. That wall flash is a block marker, not a technique cue. Do not count it as a buzz.

Rest only while holding the wall with both arms still, and rest before the button press that starts the next block. Fidgeting arms can look like strokes.

| Press | What you swim next | Tally block |
| --- | --- | --- |
| 1 | 4 easy freestyle lengths, your normal entry | easy1 |
| 2 | 4 lengths with a deliberately higher hand entry | altered |
| 3 | 4 easy lengths, normal entry again | easy2 |
| 4 | Stop. No more swimming. | end |

The first eight stroke events after press 1 are the personal baseline. The motor should stay quiet during those eight. After that, a cue means two stroke events in a row sat beyond the larger of 8° and two baseline standard deviations. Cues then wait 5 seconds.

The higher entry should be obvious on camera: a higher hand before the catch, same effort, same stroke count as far as you can manage. Do not sprint the altered block and easy the others. Pace is a confound.

## What to count on the video

Use [video-tally.csv](video-tally.csv). One row per length.

- `video_strokes`: arm cycles of the instrumented arm only, from the push-off glide until the wall touch. Do not count the push-off as a stroke.
- `video_buzzes`: LED flashes during that length. Leave the cell empty if the arm was not visible. Do not guess. The wall-marker flash is not a buzz.
- `notes`: anything that breaks the comparison (stopped mid-length, loose strap, turned around).

## After

1. Dry the outside before opening.
2. Copy the `aq*.csv` file off the card.
3. Run:

```
python3 iris/analysis/analyze_session.py path/to/aq0.csv iris/protocol/video-tally.csv
```

4. Paste that report into the pool table in the paper. Keep the CSV files with the project. They are the experimental data.

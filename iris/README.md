# AquaCue IRIS sprint

Minimal wrist logger for the IRIS National Fair (Embedded Systems). Submission window closes **13 October 2026**: a synopsis, this paper, and a 90-second video, uploaded at [iris.exstemplar.com](https://iris.exstemplar.com/).

The device counts freestyle strokes and vibrates when the wrist angle at the stroke event drifts from the swimmer’s own first strokes. It does not measure anatomical hand-entry angle, propulsive efficiency, or fatigue.

## Order of work

1. Register the project if that is not already done. Fee is ₹5,000 plus tax per project. Category: Embedded Systems.
2. Buy the parts in [protocol/buy-list.md](protocol/buy-list.md).
3. Flash [firmware/aquacue/aquacue.ino](firmware/aquacue/aquacue.ino). Wiring is in the buy list and at the top of the sketch.
4. Bench test: [protocol/bench-protractor.md](protocol/bench-protractor.md).
5. Seal and bucket test: [protocol/bucket-seal.md](protocol/bucket-seal.md).
6. Pool: [protocol/pool-protocol.md](protocol/pool-protocol.md) and [protocol/video-tally.csv](protocol/video-tally.csv).
7. Run `python3 analysis/analyze_session.py` on the SD card file and the tally.
8. Paste the bench RMSE and the session report into the empty results tables in [paper/paper.md](paper/paper.md). Do not invent those numbers.
9. Film [paper/video-shot-list.md](paper/video-shot-list.md) and upload with [paper/synopsis.md](paper/synopsis.md).

Offline clip results are already in [analysis/offline_results.md](analysis/offline_results.md). Those clips are short public windows, not a pool session.

## What is already verified on this machine

- Host tests for the Kalman update, stroke peak detector, and haptic rule: `g++` build of `firmware/test_logic.cpp`.
- Stroke-family separation on the local clips: `analysis/classify_strokes.py`.
- Session report script, including a synthetic self-test that is not pool data: `analysis/analyze_session.py --self-test`.

The pool session, the protractor sheet, and a waterproof seal still have to be done with the hardware.

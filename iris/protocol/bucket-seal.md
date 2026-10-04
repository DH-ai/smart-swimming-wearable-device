# Bucket seal checklist

Do this the day before the pool, after the sketch logs on the bench. A leak found at the pool wastes the only session.

The printed pod in `cad/forearm_pod.scad` is a silicone-sealed shell, not a rated waterproof watch. A food box with the same seal steps is an acceptable fallback.

## Before it goes under water

- Firmware logs a file on the SD card and the serial monitor shows angles.
- The motor buzzes once at the end of the still calibration, and the white LED flashes with it.
- The MPU-6050 is taped or glued flat to the wall that will sit on the forearm. It does not rattle.
- Wires are short, strain-relieved, and cannot pull a pad off the board.
- The SD card is seated. The USB power bank or the 5 V boost is inside the box, not dangling.
- Silicone is on the lid seam only, not on the MPU, the USB connector, or the SD contacts.
- The lid is closed and the extra silicone has skinned enough that it will not wash into the boards. Follow the tube’s skin time.

## The test

1. Start a log (normal boot, not bench mode). Let it finish the still calibration in air so you hear the confirmation buzz.
2. Hold the sealed box under water in a bucket for 10 minutes. Turn it over once. Do not swim with it yet.
3. Take it out, dry the outside, and open it.
4. Look for droplets, fog on the chips, and a damp SD card.

If the inside is dry, close it the same way for the pool. If you find water, do not swim with it. Reseal and repeat the bucket test.

## At the pool, still before the protocol

- Strap the box so the MPU side bears on the forearm and the LED faces away from the arm, visible from the deck during recovery.
- Do the 10 practice swings and the wall start only after the strap is tight.
- One person stays out of the water to film and to take the box if the seal fails.

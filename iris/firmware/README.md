# Firmware notes

Flash `aquacue/aquacue.ino` with the Arduino IDE or arduino-cli.

- Board: ESP32 Dev Module
- The sketch uses only the built-in `Wire`, `SPI`, and `SD` libraries
- Serial monitor: 115200 baud

Hold the GPIO 27 button down, press reset, and keep holding until the monitor prints `mode=bench`. A normal boot prints `mode=swim`.

The filter, stroke detector, and cue rule live in the headers next to the sketch so the same code can be checked on the computer:

```
g++ -std=c++11 -Wall -Wextra -Iaquacue -o /tmp/aquacue_test test_logic.cpp -lm
/tmp/aquacue_test
```

That test passed on this machine. It does not talk to an MPU-6050, and the ESP32 toolchain was not installed here, so the sketch still needs one upload to the board before the protractor test. The protractor procedure is what checks the angle on the real sensor.

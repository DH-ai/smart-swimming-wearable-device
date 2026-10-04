# Buy list

Buy this locally (Robu, Robocraze, or a shop) before worrying about a custom case. Budget about ₹2,000–3,500. Prices move; check the listing. Skip LoRa, a pressure sensor, and a custom PCB for this submission.

## Parts

- ESP32 dev board, 30-pin or 38-pin DevKit class, with the USB connector and the onboard 5 V regulator. About ₹450–700.
- MPU-6050 breakout, usually sold as GY-521. About ₹150–250. This is the sensor named in the paper.
- SPI microSD module, plus an 8 GB or 16 GB card formatted FAT32. About ₹300–500 together.
- 10 mm coin vibration motor. About ₹40–80.
- 2N2222 or BC547 transistor, a 1 kΩ resistor, a 220 Ω resistor, a 1N4148 diode, and a 5 mm white LED. A few rupees each.
- Power, pick one:
  - Preferred: a small 5 V USB power bank that fits in the box, and a short USB cable. The dev board is made for 5 V on the USB / VIN pin.
  - Smaller: a protected 3.7 V LiPo (about 500 mAh), a TP4056 charger, and an MT3608 boost set to 5.0 V into VIN.
- Velcro strap, about 20 mm wide and long enough for a forearm.
- Neutral-cure silicone, electrical tape, dupont wires for the bench, and solder for the pool build.
- A small rigid box if you are not printing `cad/forearm_pod.scad`.

Do not connect a LiPo straight to the ESP32 3.3 V pin. A full cell is 4.2 V. Do not charge a LiPo unattended, and do not power the board from USB and a battery at the same time unless the board’s own charger is designed for that.

## Why these pins

GPIO 5, 0, 2, 12, and 15 are strapped at boot. SD chip-select is GPIO 14 so the card cannot hold the board in the wrong boot mode. The ESP32 is not 5 V tolerant on its GPIO pins, so power the SD module and the MPU-6050 from 3.3 V.

| Signal | GPIO | Connects to |
| --- | --- | --- |
| I2C SDA | 21 | MPU SDA |
| I2C SCL | 22 | MPU SCL |
| SPI SCK | 18 | SD SCK |
| SPI MISO | 19 | SD MISO |
| SPI MOSI | 23 | SD MOSI |
| SD CS | 14 | SD CS |
| Motor | 26 | 1 kΩ, then transistor base |
| Button | 27 | Button to GND. Internal pull-up. |
| Sync LED | 13 | 220 Ω, then LED to GND |

MPU AD0 to GND (I2C address 0x68). MPU and SD VCC to 3.3 V, grounds common.

Motor: transistor emitter to GND, collector to the motor’s negative lead, motor positive to 3.3 V. Diode across the motor, cathode (band) on the 3.3 V side, anode on the collector. The LED is the visible twin of the motor so a deck camera can see cues during the arm recovery.

## Bench wiring, then pool wiring

Prove the sketch on dupont wires first. Then solder the pool build, glue the MPU-6050 to the inner face that straps against the forearm, and only then seal the box. A loose board makes the angle meaningless.

## Tools you already need

- A laptop with the Arduino IDE or `arduino-cli`, and the ESP32 board package from Espressif.
- A phone that can film the lane from the deck.
- A protractor and a ruler for the bench.
- A bucket for the leak test.

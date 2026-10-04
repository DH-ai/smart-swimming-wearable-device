# Forearm pod

`forearm_pod.scad` is a two-part shell for the parts in the buy list.

| Dimension | Size |
| --- | --- |
| Internal cavity | 102 × 58 × 32 mm |
| Bottom outside, without the strap loops | 106.8 × 62.8 × 34.6 mm |
| Strap loops | 20 mm slot, about 16 mm out from each long side |
| GY-521 pocket | 22 × 16 mm, about 1.2 mm of plastic between the board and the arm |
| Motor recess | 10.8 mm diameter, in the floor |
| Screws | 4 × M3 self-tapping into 2.7 mm pilots. Lid holes are 3.4 mm |

OpenSCAD was not installed on the machine that generated this file, so export an STL and look at it before printing.

```
openscad -D 'part="bottom"' -o bottom.stl forearm_pod.scad
openscad -D 'part="lid"' -o lid.stl forearm_pod.scad
```

Print both pieces with the large flat face on the bed. Use 0.2 mm layers, 4 perimeters, and about 20% infill. PETG holds up to a hot pool deck better than PLA. After printing, tap the posts with M3 screws, sit the GY-521 in the pocket with the chip toward the arm, and run a bead of neutral-cure silicone in the lid skirt. This seal is for a supervised pool session after the bucket test. It is not an IP or ATM rating.

A small USB power bank may be wider than 58 mm. If it does not sit in the cavity, use the LiPo and 5 V boost from the buy list, or increase `inner_l` and `inner_w` at the top of the script and export again.

## If there is no time to print

Use a rigid food container that closes tightly.

1. Tape the GY-521 to the inside face that will lie on the forearm. The board must not move relative to that face.
2. Put the ESP32, SD module, and battery beside it. Keep the wires short.
3. Drill or melt a 5 mm hole for the LED only if you can seal it. Otherwise glue the LED against the inside of a clear lid so the camera can see it.
4. Close the lid onto a bead of silicone. No USB cable may leave the box during the swim.
5. Strap the container with velcro so the sensor face is tight on the forearm.
6. Do the bucket test in `protocol/bucket-seal.md` before the pool.

A rattling sensor makes the angle in the paper meaningless, even if the box is dry.

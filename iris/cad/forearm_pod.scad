// AquaCue forearm pod for an ESP32 dev board, GY-521, SPI microSD module,
// and a small power bank or a LiPo plus boost.
//
// This is a silicone-sealed prototype shell. It is not a tested waterproof rating.
//
// Preview both parts:  openscad forearm_pod.scad
// Export the bottom:   openscad -D 'part="bottom"' -o bottom.stl forearm_pod.scad
// Export the lid:      openscad -D 'part="lid"' -o lid.stl forearm_pod.scad
//
// Print each part flat on the bed, 0.2 mm layers, 4 perimeters, 20% infill.
// PETG is a better pool plastic than PLA. Seal the skirt with neutral-cure silicone.

part = "both"; // [both, bottom, lid]

wall = 2.4;
inner_l = 102;
inner_w = 58;
cavity_h = 32;
floor_t = 2.6;
lid_t = 2.6;
skirt_drop = 6;
skirt_gap = 0.4;
corner_r = 3;
post_r = 4.2;
post_hole = 2.7;     // M3 self-tapping into the post
lid_hole = 3.4;
$fn = 48;

outer_l = inner_l + 2 * wall;
outer_w = inner_w + 2 * wall;
bottom_h = floor_t + cavity_h;

module rounded_box(l, w, h, r) {
    hull() {
        for (x = [r, l - r])
            for (y = [r, w - r])
                translate([x, y, 0])
                    cylinder(h = h, r = r);
    }
}

module post_centers() {
    inset = wall + post_r + 1.2;
    for (x = [inset, outer_l - inset])
        for (y = [inset, outer_w - inset])
            translate([x, y, floor_t])
                children();
}

// A velcro loop on the side of the case. Local +Y points into the case wall.
// The slot stops before the cavity, so the strap tunnel is not a leak into the box.
module side_lug() {
    difference() {
        translate([-13, -16, 0]) cube([26, 18, 14]);
        translate([-10, -18, 4.5]) cube([20, 16, 4]);
    }
}

module bottom() {
    difference() {
        union() {
            difference() {
                rounded_box(outer_l, outer_w, bottom_h, corner_r);
                translate([wall, wall, floor_t])
                    cube([inner_l, inner_w, cavity_h + 1]);
                // GY-521 pocket. About 1.2 mm of floor remains toward the arm.
                translate([wall + 8, wall + 8, 1.2])
                    cube([22, 16, floor_t]);
                // Coin-motor recess, so the vibration couples into the shell.
                translate([outer_l - wall - 18, outer_w / 2, 1.0])
                    cylinder(h = 2.2, r = 5.4);
            }
            for (x = [34, outer_l - 34]) {
                translate([x, 0, 0]) side_lug();
                translate([x, outer_w, 0]) mirror([0, 1, 0]) side_lug();
            }
            post_centers() cylinder(h = cavity_h, r = post_r);
        }
        post_centers() cylinder(h = cavity_h + 1, r = post_hole);
    }
}

module lid() {
    lid_l = outer_l + 2 * skirt_gap + 2 * wall;
    lid_w = outer_w + 2 * skirt_gap + 2 * wall;
    // Build around the same origin as the bottom, then the skirt hangs outside it.
    translate([-wall - skirt_gap, -wall - skirt_gap, 0])
        difference() {
            union() {
                rounded_box(lid_l, lid_w, lid_t + skirt_drop, corner_r);
            }
            // Leave a plate of lid_t, and a skirt cavity that fits over the bottom.
            translate([wall, wall, lid_t])
                cube([outer_l + 2 * skirt_gap, outer_w + 2 * skirt_gap, skirt_drop + 1]);
            // Clearance holes line up with the bottom posts after the skirt offset.
            hole = wall + skirt_gap + wall + post_r + 1.2;
            for (x = [hole, wall + skirt_gap + outer_l - (wall + post_r + 1.2)])
                for (y = [hole, wall + skirt_gap + outer_w - (wall + post_r + 1.2)])
                    translate([x, y, -1])
                        cylinder(h = lid_t + 2, r = lid_hole);
        }
}

if (part == "bottom") bottom();
else if (part == "lid") lid();
else {
    bottom();
    translate([0, outer_w + 30, 0]) lid();
}

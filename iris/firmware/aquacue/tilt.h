#pragma once

#include <math.h>

// Signed tilt from a specific gyro axis, in radians.
// axis 0 (X): roll, atan2(ay, az)
// axis 1 (Y): pitch, atan2(-ax, hypot(ay, az))
// axis 2 (Z): yaw is not observable from gravity; returns 0.
inline float accel_tilt_rad(int axis, float ax, float ay, float az) {
  if (axis == 0) return atan2f(ay, az);
  if (axis == 1) return atan2f(-ax, sqrtf(ay * ay + az * az));
  return 0.0f;
}

// |a| far from 1 g means the accelerometer tilt is contaminated by motion.
inline float accel_trust_scale(float ax, float ay, float az) {
  float mag = sqrtf(ax * ax + ay * ay + az * az);
  float err = fabsf(mag - 1.0f);
  if (err < 0.25f) return 1.0f;
  if (err > 0.80f) return 40.0f;
  return 1.0f + (err - 0.25f) * (39.0f / 0.55f);
}

#pragma once

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// 1D Kalman filter for tilt.
// State is [angle, gyro bias].
// Prediction uses the gyroscope. The update uses an accelerometer tilt.
// Update: angle += K * (measurement - prediction), with the innovation wrapped
// to [-pi, pi]. This is the sign the paper must use.

struct Kalman1D {
  float angle;
  float bias;
  float P00, P01, P10, P11;
  float Q_angle;
  float Q_bias;
  float R;
};

inline float kalman_wrap_pi(float a) {
  while (a > (float)M_PI) a -= 2.0f * (float)M_PI;
  while (a < -(float)M_PI) a += 2.0f * (float)M_PI;
  return a;
}

inline void kalman_init(Kalman1D* k, float angle, float bias) {
  k->angle = angle;
  k->bias = bias;
  k->P00 = 1.0f;
  k->P01 = 0.0f;
  k->P10 = 0.0f;
  k->P11 = 1.0f;
  k->Q_angle = 0.001f;
  k->Q_bias = 0.003f;
  k->R = 0.03f;
}

// omega is the measured angular rate about the tilt axis, in rad/s.
inline void kalman_predict(Kalman1D* k, float omega, float dt) {
  k->angle = k->angle + dt * (omega - k->bias);

  float p00 = k->P00;
  float p01 = k->P01;
  float p10 = k->P10;
  float p11 = k->P11;

  float a00 = p00 - dt * p10;
  float a01 = p01 - dt * p11;
  float a10 = p10;
  float a11 = p11;

  k->P00 = a00 - dt * a01 + k->Q_angle;
  k->P01 = a01;
  k->P10 = a10 - dt * a11;
  k->P11 = a11 + k->Q_bias;
}

// r_scale > 1 distrusts the accelerometer (used when |a| is far from 1 g).
inline void kalman_update(Kalman1D* k, float measured_angle, float r_scale) {
  float y = kalman_wrap_pi(measured_angle - k->angle);
  float r = k->R * r_scale;
  if (r < 1e-6f) r = 1e-6f;
  float s = k->P00 + r;
  float k0 = k->P00 / s;
  float k1 = k->P10 / s;

  float p00 = k->P00;
  float p01 = k->P01;
  float p10 = k->P10;
  float p11 = k->P11;

  k->angle = kalman_wrap_pi(k->angle + k0 * y);
  k->bias = k->bias + k1 * y;

  k->P00 = (1.0f - k0) * p00;
  k->P01 = (1.0f - k0) * p01;
  k->P10 = p10 - k1 * p00;
  k->P11 = p11 - k1 * p01;
}

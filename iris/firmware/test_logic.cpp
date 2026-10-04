// Host checks for the filter, stroke detector, and haptic rule shared with aquacue.ino.
// Build from iris/firmware:
//   g++ -std=c++11 -Wall -Wextra -Iaquacue -o /tmp/test_logic test_logic.cpp -lm

#include <math.h>
#include <stdio.h>
#include <stdlib.h>

#include "cue.h"
#include "kalman1d.h"
#include "stroke.h"
#include "tilt.h"

static int failures = 0;

static void expect(int cond, const char* msg) {
  if (!cond) {
    printf("FAIL %s\n", msg);
    failures++;
  } else {
    printf("ok   %s\n", msg);
  }
}

static void test_update_sign() {
  Kalman1D k;
  kalman_init(&k, 0.0f, 0.0f);
  kalman_predict(&k, 0.0f, 0.01f);
  float before = k.angle;
  kalman_update(&k, 0.20f, 1.0f);
  expect(k.angle > before, "measurement above prediction increases the angle");
}

static void test_tilt_pitch() {
  float rad30 = 30.0f * (float)M_PI / 180.0f;
  float ax = -sinf(rad30);
  float az = cosf(rad30);
  float est = accel_tilt_rad(1, ax, 0.0f, az);
  float err = fabsf(est - rad30) * 180.0f / (float)M_PI;
  expect(err < 0.2f, "pitch of 30 degrees reads back as 30");
  float flat = accel_tilt_rad(1, 0.0f, 0.0f, 1.0f);
  expect(fabsf(flat) < 0.01f, "flat board reads about 0 pitch");
}

static void test_bias_and_tilt() {
  Kalman1D k;
  kalman_init(&k, 0.0f, 0.0f);
  const float dt = 0.01f;
  const float bias = 2.0f * (float)M_PI / 180.0f;  // 2 deg/s
  const float target = 30.0f * (float)M_PI / 180.0f;
  float true_angle = 0.0f;
  for (int i = 0; i < 300; i++) {
    float rate = (target - true_angle) * 3.0f;
    true_angle += rate * dt;
    kalman_predict(&k, rate + bias, dt);
    kalman_update(&k, true_angle, 1.0f);
  }
  for (int i = 0; i < 200; i++) {
    kalman_predict(&k, bias, dt);
    kalman_update(&k, target, 1.0f);
  }
  float angle_err = fabsf(k.angle - target) * 180.0f / (float)M_PI;
  float bias_err = fabsf(k.bias - bias) * 180.0f / (float)M_PI;
  printf("     angle_err_deg=%.3f bias_err_dps=%.3f\n", angle_err, bias_err);
  expect(angle_err < 1.0f, "filter settles within 1 degree of a held tilt");
  expect(bias_err < 0.5f, "filter estimates a constant gyro bias");
}

static void test_motion_distrusts_accel() {
  expect(accel_trust_scale(0, 0, 1) < 1.5f, "1 g keeps accelerometer trust");
  expect(accel_trust_scale(0, 0, 3) > 10.0f, "3 g inflates the accelerometer variance");
}

static void test_strokes() {
  StrokeDetect s;
  stroke_init(&s, 0, +1, 80.0f, 0.40f);
  int count = 0;
  float t = 0;
  for (int pulse = 0; pulse < 3; pulse++) {
    float base = 0.50f + pulse * 1.00f;
    for (int i = 0; i < 100; i++) {
      t = base + i * 0.01f;
      float tri = 0;
      if (i >= 10 && i <= 30) {
        int d = i <= 20 ? (i - 10) : (30 - i);
        tri = 20.0f * d;
      }
      float g[3] = {tri, 0, 0};
      count += stroke_push(&s, t, g);
    }
  }
  expect(count == 3, "three separated pulses make three strokes");

  stroke_init(&s, 0, +1, 80.0f, 0.40f);
  int early = 0;
  float g1[3] = {0, 0, 0};
  float g2[3] = {200, 0, 0};
  float g3[3] = {0, 0, 0};
  stroke_push(&s, 0.00f, g1);
  stroke_push(&s, 0.01f, g2);
  early += stroke_push(&s, 0.02f, g3);
  stroke_push(&s, 0.10f, g1);
  stroke_push(&s, 0.11f, g2);
  early += stroke_push(&s, 0.12f, g3);
  expect(early == 1, "a second peak inside the refractory period is ignored");
}

static void test_cue() {
  CueState c;
  cue_init(&c);
  int fires = 0;
  for (int i = 0; i < 8; i++) fires += cue_on_event(&c, 10.0f, (float)i);
  expect(fires == 0, "baseline strokes do not cue");
  expect(c.ready == 1, "baseline freezes after eight events");
  fires = cue_on_event(&c, 40.0f, 10.0f);
  expect(fires == 0, "one deviant stroke is not enough");
  fires = cue_on_event(&c, 40.0f, 11.0f);
  expect(fires == 1, "two deviant strokes cue");
  fires = cue_on_event(&c, 40.0f, 12.0f);
  int second = cue_on_event(&c, 40.0f, 13.0f);
  expect(fires == 0 && second == 0, "cooldown blocks another cue");
  fires = cue_on_event(&c, 40.0f, 18.2f);
  second = cue_on_event(&c, 40.0f, 19.0f);
  expect(second == 1, "cue returns after the cooldown and two new deviant strokes");
  cue_on_event(&c, 10.0f, 30.0f);
  expect(c.consec == 0, "a normal stroke clears the consecutive count");
}

int main() {
  test_update_sign();
  test_tilt_pitch();
  test_bias_and_tilt();
  test_motion_distrusts_accel();
  test_strokes();
  test_cue();
  if (failures) {
    printf("%d failed\n", failures);
    return 1;
  }
  printf("all passed\n");
  return 0;
}

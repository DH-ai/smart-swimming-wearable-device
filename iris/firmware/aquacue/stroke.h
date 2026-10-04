#pragma once

// Causal peak detector on one signed gyro axis.
// A sample is an event when it is a local maximum of (sign * gyro[axis]),
// the peak exceeds the threshold, and the previous event was at least
// refractory_s earlier. The peak is reported one sample late; the caller
// should use the angle from the previous sample.

struct StrokeDetect {
  int axis;
  int sign;
  float threshold_dps;
  float refractory_s;
  float last_peak_t;
  float g_prev;
  float g_mid;
  int have;
};

inline void stroke_init(StrokeDetect* s, int axis, int sign, float threshold_dps,
                        float refractory_s) {
  s->axis = axis;
  s->sign = (sign >= 0) ? 1 : -1;
  s->threshold_dps = threshold_dps;
  s->refractory_s = refractory_s;
  s->last_peak_t = -1.0e9f;
  s->g_prev = 0.0f;
  s->g_mid = 0.0f;
  s->have = 0;
}

// g_dps is bias-removed gyro, degrees per second. t is seconds.
// Returns 1 when the previous sample was an accepted peak.
inline int stroke_push(StrokeDetect* s, float t, const float g_dps[3]) {
  float v = g_dps[s->axis] * (float)s->sign;
  int fired = 0;
  if (s->have >= 2) {
    if (s->g_mid >= s->g_prev && s->g_mid > v && s->g_mid > s->threshold_dps) {
      if ((t - s->last_peak_t) >= s->refractory_s) {
        fired = 1;
        s->last_peak_t = t;
      }
    }
  }
  s->g_prev = s->g_mid;
  s->g_mid = v;
  if (s->have < 2) s->have++;
  return fired;
}

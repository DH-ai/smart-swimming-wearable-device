#pragma once

#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Personal baseline from the first CUE_BASELINE_N stroke events.
// After that, two events in a row beyond max(CUE_SD_MULT * sd, CUE_MIN_DEG)
// fire one cue, then a cooldown. Angles are degrees.

static const int CUE_BASELINE_N = 8;
static const float CUE_SD_MULT = 2.0f;
static const float CUE_MIN_DEG = 8.0f;
static const float CUE_COOLDOWN_S = 5.0f;

struct CueState {
  float angles[8];
  int n;
  int consec;
  int ready;
  float mean_deg;
  float sd_deg;
  float cooldown_until;
};

inline float cue_wrap180(float deg) {
  while (deg > 180.0f) deg -= 360.0f;
  while (deg < -180.0f) deg += 360.0f;
  return deg;
}

inline void cue_init(CueState* c) {
  c->n = 0;
  c->consec = 0;
  c->ready = 0;
  c->mean_deg = 0.0f;
  c->sd_deg = 0.0f;
  c->cooldown_until = 0.0f;
}

inline void cue_freeze(CueState* c) {
  float ss = 0.0f;
  float sc = 0.0f;
  int i;
  for (i = 0; i < c->n; i++) {
    float r = c->angles[i] * ((float)M_PI / 180.0f);
    ss += sinf(r);
    sc += cosf(r);
  }
  c->mean_deg = atan2f(ss, sc) * (180.0f / (float)M_PI);
  float var = 0.0f;
  for (i = 0; i < c->n; i++) {
    float d = cue_wrap180(c->angles[i] - c->mean_deg);
    var += d * d;
  }
  if (c->n > 1) c->sd_deg = sqrtf(var / (float)(c->n - 1));
  else c->sd_deg = 0.0f;
  c->ready = 1;
}

// Returns 1 if the motor should pulse now.
inline int cue_on_event(CueState* c, float angle_deg, float t_s) {
  if (c->n < CUE_BASELINE_N) {
    c->angles[c->n++] = angle_deg;
    if (c->n == CUE_BASELINE_N) cue_freeze(c);
    return 0;
  }
  if (t_s < c->cooldown_until) return 0;
  float d = cue_wrap180(angle_deg - c->mean_deg);
  float thresh = CUE_SD_MULT * c->sd_deg;
  if (thresh < CUE_MIN_DEG) thresh = CUE_MIN_DEG;
  if (fabsf(d) > thresh) c->consec++;
  else c->consec = 0;
  if (c->consec >= 2 && t_s >= c->cooldown_until) {
    c->cooldown_until = t_s + CUE_COOLDOWN_S;
    c->consec = 0;
    return 1;
  }
  return 0;
}

// AquaCue firmware
// ESP32 + MPU-6050 + microSD + vibration motor.
//
// Wiring (also in iris/protocol/buy-list.md):
//   MPU SDA 21, SCL 22, AD0 GND, VCC 3V3
//   SD  SCK 18, MISO 19, MOSI 23, CS 14, VCC 3V3
//   Motor GPIO 26 -> 1k -> transistor base. Motor between 3V3 and collector.
//   Button GPIO 27 to GND (internal pull-up)
//   LED GPIO 13 -> 220 ohm -> LED -> GND
//
// Hold the button while pressing reset to enter bench mode.
// Normal boot: still bias, 10 practice swings, then button marks
// easy1 / altered / easy2 / stop.
//
// CSV:
// t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,angle_deg,stroke,haptic,mark,mode

#include <SPI.h>
#include <SD.h>
#include <Wire.h>
#include <stdio.h>
#include <string.h>

#include "cue.h"
#include "kalman1d.h"
#include "stroke.h"
#include "tilt.h"

static const int PIN_SDA = 21;
static const int PIN_SCL = 22;
static const int PIN_SD_CS = 14;
static const int PIN_SD_SCK = 18;
static const int PIN_SD_MISO = 19;
static const int PIN_SD_MOSI = 23;
static const int PIN_MOTOR = 26;
static const int PIN_BUTTON = 27;
static const int PIN_LED = 13;

static const int MPU_ADDR = 0x68;
static const float ACCEL_LSB_PER_G = 4096.0f;
static const float GYRO_LSB_PER_DPS = 32.768f;
static const int PRACTICE_N = 10;
static const float PRACTICE_GATE_DPS = 50.0f;
static const float PRACTICE_REFRACTORY_S = 0.45f;
static const float STROKE_REFRACTORY_S = 0.45f;
static const float MIN_THRESHOLD_DPS = 60.0f;
static const unsigned long SAMPLE_US = 10000;
static const unsigned long HAPTIC_MS = 200;

enum RunMode { MODE_STILL, MODE_PRACTICE, MODE_ARMED, MODE_SWIM, MODE_DONE, MODE_BENCH };

static RunMode mode = MODE_STILL;
static bool sd_ok = false;
static File log_file;
static char log_buf[3072];
static size_t log_len = 0;
static unsigned long last_flush_ms = 0;
static unsigned long session_start_ms = 0;

static Kalman1D kf;
static StrokeDetect detector;
static CueState cue;

static float still_bias_dps[3] = {0, 0, 0};
static float still_sum[3] = {0, 0, 0};
static int still_count = 0;

static float prac_mag_prev = 0;
static float prac_mag_mid = 0;
static float prac_g_mid[3] = {0, 0, 0};
static int prac_have = 0;
static float prac_last_t = -1.0e9f;
static int prac_count = 0;
static int axis_votes[3] = {0, 0, 0};
static int sign_votes[3][2] = {{0, 0}, {0, 0}, {0, 0}};
static float prac_peaks[PRACTICE_N];
static int prac_peak_axis[PRACTICE_N];

static int chosen_axis = 1;
static int chosen_sign = 1;
static float prev_angle_deg = 0;
static bool have_prev_angle = false;
static float last_ax = 0, last_ay = 0, last_az = 1;

static int mark_index = 0;
static int pending_mark = 0;
static bool stop_requested = false;

static bool button_down = false;
static unsigned long last_press_ms = 0;
static unsigned long motor_off_ms = 0;
static unsigned long led_off_ms = 0;
static bool motor_on = false;

static bool bench_zero_locked = false;
static float bench_zero_deg = 0;
static int bench_quiet = 0;
static float bench_ring[100];
static int bench_ring_i = 0;
static int bench_ring_n = 0;
static int bench_index = 0;

static uint32_t last_sample_us = 0;
static float time_s = 0;

static bool mpu_write(uint8_t reg, uint8_t val) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(reg);
  Wire.write(val);
  return Wire.endTransmission() == 0;
}

static bool mpu_begin() {
  Wire.begin(PIN_SDA, PIN_SCL);
  Wire.setClock(400000);
  if (!mpu_write(0x6B, 0x00)) return false;
  delay(50);
  if (!mpu_write(0x1B, 0x10)) return false;
  if (!mpu_write(0x1C, 0x10)) return false;
  if (!mpu_write(0x1A, 0x03)) return false;
  if (!mpu_write(0x19, 0x09)) return false;
  return true;
}

static bool mpu_read(float* ax, float* ay, float* az, float* gx, float* gy, float* gz) {
  Wire.beginTransmission(MPU_ADDR);
  Wire.write(0x3B);
  if (Wire.endTransmission(false) != 0) return false;
  if (Wire.requestFrom(MPU_ADDR, 14, true) != 14) return false;
  int16_t raw[7];
  for (int i = 0; i < 7; i++) raw[i] = (int16_t)((Wire.read() << 8) | Wire.read());
  *ax = raw[0] / ACCEL_LSB_PER_G;
  *ay = raw[1] / ACCEL_LSB_PER_G;
  *az = raw[2] / ACCEL_LSB_PER_G;
  *gx = raw[4] / GYRO_LSB_PER_DPS;
  *gy = raw[5] / GYRO_LSB_PER_DPS;
  *gz = raw[6] / GYRO_LSB_PER_DPS;
  return true;
}

static void set_led(bool on) { digitalWrite(PIN_LED, on ? HIGH : LOW); }

static void pulse_motor(unsigned long now_ms) {
  digitalWrite(PIN_MOTOR, HIGH);
  motor_on = true;
  motor_off_ms = now_ms + HAPTIC_MS;
  set_led(true);
  led_off_ms = now_ms + HAPTIC_MS;
}

static void flash_led(unsigned long now_ms, unsigned long dur_ms) {
  set_led(true);
  if (now_ms + dur_ms > led_off_ms) led_off_ms = now_ms + dur_ms;
}

static void service_outputs(unsigned long now_ms) {
  if (motor_on && (long)(now_ms - motor_off_ms) >= 0) {
    digitalWrite(PIN_MOTOR, LOW);
    motor_on = false;
  }
  if (led_off_ms != 0 && (long)(now_ms - led_off_ms) >= 0) set_led(false);
}

static char mode_char() {
  switch (mode) {
    case MODE_STILL: return 'S';
    case MODE_PRACTICE: return 'P';
    case MODE_ARMED: return 'A';
    case MODE_SWIM: return 'W';
    case MODE_DONE: return 'D';
    case MODE_BENCH: return 'B';
  }
  return '?';
}

static void log_flush() {
  if (!sd_ok || log_len == 0) return;
  log_file.write((const uint8_t*)log_buf, log_len);
  log_file.flush();
  log_len = 0;
  last_flush_ms = millis();
}

static void log_line(unsigned long t_ms, float ax, float ay, float az, float gx, float gy,
                     float gz, float angle_deg, int stroke, int haptic, int mark) {
  char line[180];
  int n = snprintf(line, sizeof(line),
                   "%lu,%.3f,%.3f,%.3f,%.2f,%.2f,%.2f,%.2f,%d,%d,%d,%c\n", t_ms, ax, ay, az,
                   gx, gy, gz, angle_deg, stroke, haptic, mark, mode_char());
  if (n <= 0) return;
  if (log_len + (size_t)n >= sizeof(log_buf)) log_flush();
  if (log_len + (size_t)n >= sizeof(log_buf)) return;
  memcpy(log_buf + log_len, line, (size_t)n);
  log_len += (size_t)n;
  if (stroke || haptic || mark) log_flush();
}

static bool sd_open() {
  SPI.begin(PIN_SD_SCK, PIN_SD_MISO, PIN_SD_MOSI, PIN_SD_CS);
  if (!SD.begin(PIN_SD_CS)) {
    Serial.println("SD failed. Continuing on serial only.");
    return false;
  }
  for (int i = 0; i < 50; i++) {
    char name[16];
    snprintf(name, sizeof(name), "/aq%d.csv", i);
    if (SD.exists(name)) continue;
    log_file = SD.open(name, FILE_WRITE);
    if (!log_file) return false;
    log_file.println(
        "t_ms,ax_g,ay_g,az_g,gx_dps,gy_dps,gz_dps,angle_deg,stroke,haptic,mark,mode");
    log_file.flush();
    Serial.print("Logging to ");
    Serial.println(name);
    return true;
  }
  return false;
}

static float median_sorted(float* v, int n) {
  for (int i = 1; i < n; i++) {
    float key = v[i];
    int j = i - 1;
    while (j >= 0 && v[j] > key) {
      v[j + 1] = v[j];
      j--;
    }
    v[j + 1] = key;
  }
  return v[n / 2];
}

static void finish_practice() {
  chosen_axis = 0;
  if (axis_votes[1] > axis_votes[chosen_axis]) chosen_axis = 1;
  if (axis_votes[2] > axis_votes[chosen_axis]) chosen_axis = 2;
  chosen_sign = (sign_votes[chosen_axis][1] >= sign_votes[chosen_axis][0]) ? 1 : -1;

  float axis_peaks[PRACTICE_N];
  int n_axis = 0;
  for (int i = 0; i < prac_count; i++) {
    if (prac_peak_axis[i] == chosen_axis) axis_peaks[n_axis++] = prac_peaks[i];
  }
  float med = (n_axis > 0) ? median_sorted(axis_peaks, n_axis) : 200.0f;
  float thr = 0.45f * med;
  if (thr < MIN_THRESHOLD_DPS) thr = MIN_THRESHOLD_DPS;

  stroke_init(&detector, chosen_axis, chosen_sign, thr, STROKE_REFRACTORY_S);
  float a0 = (chosen_axis == 2) ? 0.0f : accel_tilt_rad(chosen_axis, last_ax, last_ay, last_az);
  kalman_init(&kf, a0, 0);
  have_prev_angle = false;
  mode = MODE_ARMED;

  Serial.print("Practice done. axis=");
  Serial.print(chosen_axis);
  Serial.print(" sign=");
  Serial.print(chosen_sign);
  Serial.print(" threshold_dps=");
  Serial.println(thr, 1);
  if (chosen_axis == 2) {
    Serial.println("WARNING: dominant axis is Z (yaw). Gravity cannot correct it. Remount and reboot.");
  }
  Serial.println("Hold still, then press the button at the wall to start easy1.");
}

static void capture_bench() {
  if (!bench_zero_locked || bench_ring_n < 20) {
    Serial.println("Hold the angle steady, then press again.");
    return;
  }
  float sum = 0;
  float sumsq = 0;
  for (int i = 0; i < bench_ring_n; i++) {
    sum += bench_ring[i];
    sumsq += bench_ring[i] * bench_ring[i];
  }
  float mean = sum / (float)bench_ring_n;
  float var = sumsq / (float)bench_ring_n - mean * mean;
  if (var < 0) var = 0;
  bench_index++;
  Serial.print("BENCH ");
  Serial.print(bench_index);
  Serial.print(" mean_deg=");
  Serial.print(mean, 2);
  Serial.print(" sd_deg=");
  Serial.println(sqrtf(var), 2);
  log_line(millis() - session_start_ms, last_ax, last_ay, last_az, 0, 0, 0, mean, 0, 0,
           bench_index);
}

static void on_button(unsigned long now_ms) {
  if (mode == MODE_BENCH) {
    capture_bench();
    flash_led(now_ms, 200);
    return;
  }
  if (mode != MODE_ARMED && mode != MODE_SWIM) return;
  if (mode == MODE_ARMED) mode = MODE_SWIM;
  mark_index++;
  pending_mark = mark_index;
  Serial.print("MARK ");
  Serial.println(mark_index);
  flash_led(now_ms, 400);
  if (mark_index >= 4) stop_requested = true;
}

static void service_button(unsigned long now_ms) {
  bool down = digitalRead(PIN_BUTTON) == LOW;
  if (down && !button_down && (now_ms - last_press_ms) > 250) {
    last_press_ms = now_ms;
    on_button(now_ms);
  }
  button_down = down;
}

static void handle_still(float gx, float gy, float gz) {
  float mag = sqrtf(gx * gx + gy * gy + gz * gz);
  if (mag > 12.0f) {
    still_count = 0;
    still_sum[0] = still_sum[1] = still_sum[2] = 0;
    return;
  }
  still_sum[0] += gx;
  still_sum[1] += gy;
  still_sum[2] += gz;
  still_count++;
  if (still_count < 200) return;
  for (int i = 0; i < 3; i++) still_bias_dps[i] = still_sum[i] / (float)still_count;
  Serial.print("Still bias dps: ");
  Serial.print(still_bias_dps[0], 2);
  Serial.print(", ");
  Serial.print(still_bias_dps[1], 2);
  Serial.print(", ");
  Serial.println(still_bias_dps[2], 2);
  pulse_motor(millis());
  mode = MODE_PRACTICE;
  Serial.println("Do 10 slow freestyle arm swings.");
}

static void handle_practice(float gx, float gy, float gz) {
  float mag = sqrtf(gx * gx + gy * gy + gz * gz);
  if (prac_have >= 2 && prac_mag_mid >= prac_mag_prev && prac_mag_mid > mag &&
      prac_mag_mid > PRACTICE_GATE_DPS && (time_s - prac_last_t) >= PRACTICE_REFRACTORY_S) {
    int ax = 0;
    if (fabsf(prac_g_mid[1]) > fabsf(prac_g_mid[ax])) ax = 1;
    if (fabsf(prac_g_mid[2]) > fabsf(prac_g_mid[ax])) ax = 2;
    int positive = prac_g_mid[ax] >= 0.0f;
    axis_votes[ax]++;
    sign_votes[ax][positive ? 1 : 0]++;
    prac_peaks[prac_count] = fabsf(prac_g_mid[ax]);
    prac_peak_axis[prac_count] = ax;
    prac_count++;
    prac_last_t = time_s;
    flash_led(millis(), 80);
    Serial.print("Practice swing ");
    Serial.print(prac_count);
    Serial.print(" axis ");
    Serial.print(ax);
    Serial.print(" peak_dps ");
    Serial.println(prac_peaks[prac_count - 1], 1);
    if (prac_count >= PRACTICE_N) finish_practice();
  }
  prac_mag_prev = prac_mag_mid;
  prac_mag_mid = mag;
  prac_g_mid[0] = gx;
  prac_g_mid[1] = gy;
  prac_g_mid[2] = gz;
  if (prac_have < 2) prac_have++;
}

static float filter_angle_deg(float ax, float ay, float az, float gx, float gy, float gz,
                              float dt) {
  float g[3] = {gx, gy, gz};
  kalman_predict(&kf, g[chosen_axis] * ((float)M_PI / 180.0f), dt);
  if (chosen_axis != 2) {
    kalman_update(&kf, accel_tilt_rad(chosen_axis, ax, ay, az), accel_trust_scale(ax, ay, az));
  }
  return kf.angle * (180.0f / (float)M_PI);
}

static void push_bench_angle(float rel) {
  bench_ring[bench_ring_i] = rel;
  bench_ring_i = (bench_ring_i + 1) % 100;
  if (bench_ring_n < 100) bench_ring_n++;
}

static void setup() {
  pinMode(PIN_MOTOR, OUTPUT);
  digitalWrite(PIN_MOTOR, LOW);
  pinMode(PIN_LED, OUTPUT);
  set_led(false);
  pinMode(PIN_BUTTON, INPUT_PULLUP);

  Serial.begin(115200);
  delay(200);
  session_start_ms = millis();

  if (digitalRead(PIN_BUTTON) == LOW) {
    delay(800);
    if (digitalRead(PIN_BUTTON) == LOW) mode = MODE_BENCH;
  }

  Serial.println(mode == MODE_BENCH ? "mode=bench" : "mode=swim");
  for (int tries = 0; tries < 20 && !mpu_begin(); tries++) {
    Serial.println("MPU-6050 not found. Check 3V3, GND, SDA=21, SCL=22.");
    delay(500);
  }
  sd_ok = sd_open();
  cue_init(&cue);
  kalman_init(&kf, 0, 0);
  last_sample_us = micros();
  if (mode == MODE_BENCH) Serial.println("Lay the board flat. Zero locks after one quiet second.");
  else Serial.println("Hold still for bias calibration.");
}

static void loop() {
  unsigned long now_ms = millis();
  service_button(now_ms);
  service_outputs(now_ms);

  if (mode == MODE_DONE) {
    log_flush();
    if (sd_ok) {
      log_file.close();
      sd_ok = false;
      Serial.println("File closed.");
    }
    delay(100);
    return;
  }

  uint32_t now_us = micros();
  if ((uint32_t)(now_us - last_sample_us) < SAMPLE_US) return;
  float dt = (float)(uint32_t)(now_us - last_sample_us) * 1.0e-6f;
  last_sample_us = now_us;
  if (dt < 0.001f) dt = 0.001f;
  if (dt > 0.05f) dt = 0.05f;
  time_s += dt;

  float ax, ay, az, gx, gy, gz;
  if (!mpu_read(&ax, &ay, &az, &gx, &gy, &gz)) return;
  last_ax = ax;
  last_ay = ay;
  last_az = az;
  unsigned long t_ms = now_ms - session_start_ms;

  if (mode == MODE_STILL) {
    handle_still(gx, gy, gz);
    return;
  }

  gx -= still_bias_dps[0];
  gy -= still_bias_dps[1];
  gz -= still_bias_dps[2];

  if (mode == MODE_PRACTICE) {
    handle_practice(gx, gy, gz);
    log_line(t_ms, ax, ay, az, gx, gy, gz, 0, 0, 0, 0);
  } else if (mode == MODE_BENCH) {
    float grate = sqrtf(gx * gx + gy * gy + gz * gz);
    float amag = sqrtf(ax * ax + ay * ay + az * az);
    if (!bench_zero_locked) {
      if (fabsf(amag - 1.0f) < 0.15f && grate < 8.0f) bench_quiet++;
      else bench_quiet = 0;
      if (bench_quiet >= 100) {
        bench_zero_deg = accel_tilt_rad(1, ax, ay, az) * (180.0f / (float)M_PI);
        chosen_axis = 1;
        kalman_init(&kf, bench_zero_deg * ((float)M_PI / 180.0f), 0);
        bench_zero_locked = true;
        Serial.print("Zero locked. ax ay az ");
        Serial.print(ax, 3);
        Serial.print(" ");
        Serial.print(ay, 3);
        Serial.print(" ");
        Serial.println(az, 3);
        Serial.println("Tilt about Y so angle_deg rises. Short-press to record a pose.");
      }
    } else {
      float angle_deg = filter_angle_deg(ax, ay, az, gx, gy, gz, dt);
      float rel = cue_wrap180(angle_deg - bench_zero_deg);
      push_bench_angle(rel);
      if ((t_ms % 200) < 20) {
        Serial.print("angle_deg=");
        Serial.println(rel, 2);
      }
      log_line(t_ms, ax, ay, az, gx, gy, gz, rel, 0, 0, 0);
    }
  } else if (mode == MODE_ARMED || mode == MODE_SWIM) {
    float angle_deg = filter_angle_deg(ax, ay, az, gx, gy, gz, dt);
    int stroke = 0;
    int haptic = 0;
    float logged_angle = angle_deg;
    if (mode == MODE_SWIM) {
      float g[3] = {gx, gy, gz};
      if (stroke_push(&detector, time_s, g) && have_prev_angle) {
        stroke = 1;
        logged_angle = prev_angle_deg;
        if (cue_on_event(&cue, prev_angle_deg, time_s)) {
          haptic = 1;
          pulse_motor(now_ms);
        }
        Serial.print("stroke angle_deg=");
        Serial.print(prev_angle_deg, 1);
        Serial.print(" haptic=");
        Serial.println(haptic);
      }
    } else if ((t_ms % 1000) < 20) {
      Serial.print("armed angle_deg=");
      Serial.println(angle_deg, 1);
    }
    int mark = pending_mark;
    pending_mark = 0;
    log_line(t_ms, ax, ay, az, gx, gy, gz, logged_angle, stroke, haptic, mark);
    prev_angle_deg = angle_deg;
    have_prev_angle = true;
    if (stop_requested) mode = MODE_DONE;
  }

  if ((now_ms - last_flush_ms) > 250) log_flush();
}

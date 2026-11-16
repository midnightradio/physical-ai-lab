/*
 * ESP32-S3 camera -> USB Web Serial -> JPG -> Edge Impulse.
 * Arduino-ESP32 3.x; esp_camera is included in the ESP32 board package.
 * Select XIAO_ESP32S3 for XIAO Sense, or ESP32S3 Dev Module for the
 * Freenove-compatible WROOM N16R8 CAM board used in the lecture.
 * Open collector.html in desktop Chrome/Edge after uploading this file.
 * No Wi-Fi credentials, SD card or Edge Impulse API key are required.
 */
#include <Arduino.h>
#include "esp_camera.h"
#include <esp_arduino_version.h>
#include <esp_log.h>

#if ESP_ARDUINO_VERSION_MAJOR < 3
#error "Install esp32 by Espressif Systems 3.x in Boards Manager."
#endif
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Select XIAO_ESP32S3 or ESP32S3 Dev Module."
#endif

// Normally no edit is needed: the Arduino board selection chooses the pins.
// For an instructor override: 1 = XIAO Sense, 2 = Freenove-compatible S3 CAM.
// #define CAMERA_PROFILE 1
#ifndef CAMERA_PROFILE
#if defined(ARDUINO_XIAO_ESP32S3)
#define CAMERA_PROFILE 1
#else
#define CAMERA_PROFILE 2
#endif
#endif
#if CAMERA_PROFILE != 1 && CAMERA_PROFILE != 2
#error "CAMERA_PROFILE must be 1 or 2."
#endif

constexpr uint32_t SERIAL_BAUD = 460800;
constexpr uint32_t CAMERA_XCLK_HZ = 20000000;
constexpr size_t MAX_JPEG_BYTES = 1024 * 1024;
static bool cameraReady = false;
static const char *cameraError = "camera_init";
static esp_err_t initError = ESP_OK;
static framesize_t currentSize = FRAMESIZE_VGA;
static unsigned currentMirror = 0, currentFlip = 0;

static const char *profileName() {
#if CAMERA_PROFILE == 1
  return "XIAO_SENSE";
#else
  return "WROOM_CAM";
#endif
}

static const char *sensorName() {
  sensor_t *s = esp_camera_sensor_get();
  if (!s) return "UNKNOWN";
  switch (s->id.PID) {
    case OV2640_PID: return "OV2640";
    case OV3660_PID: return "OV3660";
    case OV5640_PID: return "OV5640";
    default: return "OTHER";
  }
}

static void reportReady() {
  if (!cameraReady) {
    Serial.printf("ERROR %s %08lX\n", cameraError, (unsigned long)initError);
    return;
  }
  Serial.printf("READY_IMAGE1 %s %s 640 480\n", profileName(), sensorName());
}

static bool startCamera() {
  // Both supported boards have PSRAM. Report a clear error if it is disabled.
  if (!psramFound()) { cameraError = "psram"; return false; }
  camera_config_t c = {};
  c.ledc_channel = LEDC_CHANNEL_0;
  c.ledc_timer = LEDC_TIMER_0;
  c.pin_pwdn = -1;
  c.pin_reset = -1;
#if CAMERA_PROFILE == 1
  // Seeed XIAO ESP32S3 Sense expansion board.
  c.pin_xclk = 10; c.pin_sccb_sda = 40; c.pin_sccb_scl = 39;
  c.pin_d0 = 15; c.pin_d1 = 17; c.pin_d2 = 18; c.pin_d3 = 16;
  c.pin_d4 = 14; c.pin_d5 = 12; c.pin_d6 = 11; c.pin_d7 = 48;
  c.pin_vsync = 38; c.pin_href = 47; c.pin_pclk = 13;
#else
  // Freenove-compatible ESP32-S3 WROOM CAM: ESP32S3_EYE camera pin map.
  // N16R8 specifies memory, not camera wiring. Other boards may differ.
  c.pin_xclk = 15; c.pin_sccb_sda = 4; c.pin_sccb_scl = 5;
  c.pin_d0 = 11; c.pin_d1 = 9; c.pin_d2 = 8; c.pin_d3 = 10;
  c.pin_d4 = 12; c.pin_d5 = 18; c.pin_d6 = 17; c.pin_d7 = 16;
  c.pin_vsync = 6; c.pin_href = 7; c.pin_pclk = 13;
#endif
  c.xclk_freq_hz = CAMERA_XCLK_HZ;
  c.frame_size = FRAMESIZE_VGA;
  c.pixel_format = PIXFORMAT_JPEG;
  c.jpeg_quality = 12;
  c.fb_count = 1;
  c.fb_location = CAMERA_FB_IN_PSRAM;
  c.grab_mode = CAMERA_GRAB_WHEN_EMPTY;
  initError = esp_camera_init(&c);
  if (initError != ESP_OK) return false;
  sensor_t *s = esp_camera_sensor_get();
  if (!s) { cameraError = "camera_sensor"; return false; }
  s->set_hmirror(s, 0);
  s->set_vflip(s, 0);
  return true;
}

static uint32_t crc32(const uint8_t *data, size_t size) {
  uint32_t crc = 0xFFFFFFFFUL;
  while (size--) {
    crc ^= *data++;
    for (int bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320UL : 0);
  }
  return crc ^ 0xFFFFFFFFUL;
}

static bool sendBytes(const uint8_t *data, size_t size) {
  uint32_t lastProgress = millis();
  while (size) {
    size_t count = Serial.write(data, size > 1024 ? 1024 : size);
    if (count) { data += count; size -= count; lastProgress = millis(); }
    else if (millis() - lastProgress > 5000) return false;
    delay(1);
  }
  return true;
}

static void takePhoto(unsigned width, unsigned height, unsigned mirror, unsigned flip) {
  if (!cameraReady) { reportReady(); return; }
  sensor_t *s = esp_camera_sensor_get();
  const framesize_t requested = width == 640 ? FRAMESIZE_VGA : FRAMESIZE_QVGA;
  const bool changed = requested != currentSize || mirror != currentMirror || flip != currentFlip;
  if ((requested != currentSize && s->set_framesize(s, requested) != 0) ||
      (mirror != currentMirror && s->set_hmirror(s, mirror) != 0) ||
      (flip != currentFlip && s->set_vflip(s, flip) != 0)) {
    Serial.println("ERROR camera_setting");
    return;
  }
  currentSize = requested; currentMirror = mirror; currentFlip = flip;
  // Discard queued/stale frames, including those from before a setting change.
  // Capture on demand, rather than streaming and retaining an old preview frame.
  for (unsigned n = 0; n < (changed ? 2U : 1U); ++n) {
    camera_fb_t *old = esp_camera_fb_get();
    if (!old) { Serial.println("ERROR camera_frame"); return; }
    esp_camera_fb_return(old);
  }
  camera_fb_t *fb = esp_camera_fb_get();
  if (!fb) { Serial.println("ERROR camera_frame"); return; }
  if (fb->format != PIXFORMAT_JPEG || fb->len < 4 || fb->len > MAX_JPEG_BYTES ||
      fb->buf[0] != 0xFF || fb->buf[1] != 0xD8 ||
      fb->width != width || fb->height != height) {
    esp_camera_fb_return(fb);
    Serial.println("ERROR camera_format");
    return;
  }
  const uint32_t check = crc32(fb->buf, fb->len);
  Serial.printf("JPG %lu %08lX %u %u\n", (unsigned long)fb->len,
                (unsigned long)check, (unsigned)fb->width, (unsigned)fb->height);
  const bool sent = sendBytes(fb->buf, fb->len);
  esp_camera_fb_return(fb);
  Serial.println();
  if (!sent) Serial.println("ERROR serial_write");
}

static void handleCommand(const char *line) {
  if (!strcmp(line, "PING")) { reportReady(); return; }
  unsigned width = 0, height = 0, mirror = 0, flip = 0;
  char extra;
  if (sscanf(line, "CAP %u %u %u %u %c", &width, &height, &mirror, &flip, &extra) == 4 &&
      ((width == 320 && height == 240) || (width == 640 && height == 480)) &&
      mirror <= 1 && flip <= 1) {
    takePhoto(width, height, mirror, flip);
  } else Serial.println("ERROR command");
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  // Avoid unrelated ESP log output in the middle of a binary JPEG packet.
  Serial.setDebugOutput(false);
  esp_log_level_set("*", ESP_LOG_NONE);
  cameraReady = startCamera();
  delay(300);
  reportReady();
}

void loop() {
  static char line[64];
  static size_t used = 0;
  static bool overflow = false;
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      line[used] = '\0';
      if (overflow) Serial.println("ERROR command");
      else if (used) handleCommand(line);
      used = 0; overflow = false;
    } else if (used + 1 < sizeof(line) && !overflow) line[used++] = c;
    else overflow = true;
  }
  delay(1);
}

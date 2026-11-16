/*
 * ESP32-S3 audio collection for Edge Impulse
 * Arduino-ESP32 3.x / built-in ESP_I2S library; no extra libraries.
 * XIAO_ESP32S3 selected in Tools > Board: built-in PDM mic (42, 41).
 * ESP32S3 Dev Module selected: external INMP441 (SCK=4, WS=5, SD=6).
 * Output: 16000 Hz, mono, signed 16-bit little-endian PCM WAV.
 * Open recorder.html in desktop Chrome/Edge after uploading this sketch.
 */
#include <Arduino.h>
#include <ESP_I2S.h>
#include <esp_arduino_version.h>

#if ESP_ARDUINO_VERSION_MAJOR < 3
#error "Install esp32 by Espressif Systems version 3.x in Boards Manager."
#endif
#if !defined(CONFIG_IDF_TARGET_ESP32S3)
#error "Select XIAO_ESP32S3 or ESP32S3 Dev Module."
#endif

// Normally no edit is needed. To override automatic mic selection, uncomment:
// #define MIC_PROFILE 1  // 1: XIAO Sense built-in PDM; 2: external INMP441
#ifndef MIC_PROFILE
#if defined(ARDUINO_XIAO_ESP32S3)
#define MIC_PROFILE 1
#else
#define MIC_PROFILE 2
#endif
#endif
#if MIC_PROFILE != 1 && MIC_PROFILE != 2
#error "MIC_PROFILE must be 1 (PDM) or 2 (INMP441)."
#endif

constexpr uint32_t SAMPLE_RATE = 16000;
constexpr uint32_t SERIAL_BAUD = 460800;
constexpr unsigned MAX_SECONDS = 5;
constexpr size_t BLOCK_SAMPLES = 256;
constexpr int PCM_GAIN = 1;  // Fixed gain; use the same value later for inference.
constexpr int INMP_BCLK = 4;
constexpr int INMP_WS = 5;
constexpr int INMP_SD = 6;
I2SClass mic;

static const char *profileName() {
#if MIC_PROFILE == 1
  return "XIAO_PDM";
#else
  return "INMP441";
#endif
}

static void ready() {
  Serial.printf("READY_AUDIO1 %s %lu %d\n", profileName(),
                (unsigned long)SAMPLE_RATE, PCM_GAIN);
}

static bool startMic() {
#if MIC_PROFILE == 1
  mic.setPinsPdmRx(42, 41);  // PDM clock, PDM data
  return mic.begin(I2S_MODE_PDM_RX, SAMPLE_RATE,
                   I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO);
#else
  mic.setPins(INMP_BCLK, INMP_WS, -1, INMP_SD); // BCLK, WS, DOUT, DIN
  // INMP441 sends 24 valid bits inside 32-bit slots: 64 BCLK / stereo frame.
  // L/R must be tied to GND, selecting the LEFT slot.
  return mic.begin(I2S_MODE_STD, SAMPLE_RATE,
                   I2S_DATA_BIT_WIDTH_32BIT, I2S_SLOT_MODE_MONO,
                   I2S_STD_SLOT_LEFT);
#endif
}

static int16_t scaleSample(int32_t sample) {
  int32_t value = sample * PCM_GAIN;
  if (value > 32767) value = 32767;
  if (value < -32768) value = -32768;
  return (int16_t)value;
}

// Only this function and startMic() differ between microphone types.
static bool readPcm(int16_t *dest, size_t samples) {
#if MIC_PROFILE == 1
  if (mic.readBytes((char *)dest, samples * 2) != samples * 2) return false;
  for (size_t i = 0; i < samples; ++i) dest[i] = scaleSample(dest[i]);
#else
  int32_t raw[BLOCK_SAMPLES];
  if (mic.readBytes((char *)raw, samples * 4) != samples * 4) return false;
  for (size_t i = 0; i < samples; ++i) {
    // Keep the signed high 16 bits. Do not cast 32-bit samples directly to int16_t.
    dest[i] = scaleSample(raw[i] >> 16);
  }
#endif
  return true;
}

static void put16(uint8_t *p, uint16_t value) {
  p[0] = value; p[1] = value >> 8;
}
static void put32(uint8_t *p, uint32_t value) {
  for (int i = 0; i < 4; ++i) p[i] = value >> (8 * i);
}
static void makeWavHeader(uint8_t *h, uint32_t pcmBytes) {
  memset(h, 0, 44);
  memcpy(h, "RIFF", 4); put32(h + 4, 36 + pcmBytes);
  memcpy(h + 8, "WAVEfmt ", 8); put32(h + 16, 16);
  put16(h + 20, 1); put16(h + 22, 1);  // PCM, mono
  put32(h + 24, SAMPLE_RATE); put32(h + 28, SAMPLE_RATE * 2);
  put16(h + 32, 2); put16(h + 34, 16);
  memcpy(h + 36, "data", 4); put32(h + 40, pcmBytes);
}
static uint32_t updateCrc(uint32_t crc, const uint8_t *data, size_t size) {
  while (size--) {
    crc ^= *data++;
    for (int bit = 0; bit < 8; ++bit)
      crc = (crc >> 1) ^ ((crc & 1) ? 0xEDB88320UL : 0);
  }
  return crc;
}
static bool sendBytes(const uint8_t *data, size_t size) {
  uint32_t lastProgress = millis();
  while (size) {
    size_t count = Serial.write(data, size > 1024 ? 1024 : size);
    if (count) {
      data += count; size -= count; lastProgress = millis();
    } else if (millis() - lastProgress > 5000) {
      return false;
    }
    delay(1);
  }
  return true;
}

static void recordAudio(unsigned seconds) {
  const size_t samples = SAMPLE_RATE * seconds;
  const size_t pcmBytes = samples * sizeof(int16_t);
  int16_t *pcm = nullptr;
  if (psramFound()) pcm = (int16_t *)ps_malloc(pcmBytes);
  if (!pcm) pcm = (int16_t *)malloc(pcmBytes);
  if (!pcm) { Serial.println("ERROR memory"); return; }

  if (!startMic()) {
    mic.end(); free(pcm); Serial.println("ERROR microphone_init"); return;
  }
  // Discard about 200 ms after starting clocks to remove startup transients.
  int16_t discard[BLOCK_SAMPLES];
  bool ok = true;
  for (unsigned n = 0; n < 13 && ok; ++n) ok = readPcm(discard, BLOCK_SAMPLES);
  if (ok) Serial.printf("RECORDING %u\n", seconds);
  for (size_t pos = 0; pos < samples && ok;) {
    const size_t count = min(BLOCK_SAMPLES, samples - pos);
    ok = readPcm(pcm + pos, count);
    pos += count;
  }
  mic.end();
  if (!ok) { free(pcm); Serial.println("ERROR microphone_read"); return; }

  // Finish capturing before serial transmission, avoiding serial-induced gaps.
  uint8_t header[44];
  makeWavHeader(header, pcmBytes);
  uint32_t crc = updateCrc(0xFFFFFFFFUL, header, sizeof(header));
  crc = updateCrc(crc, (const uint8_t *)pcm, pcmBytes) ^ 0xFFFFFFFFUL;
  Serial.printf("WAV %lu %08lX\n", (unsigned long)(44 + pcmBytes),
                (unsigned long)crc);
  if (sendBytes(header, sizeof(header))) sendBytes((const uint8_t *)pcm, pcmBytes);
  free(pcm);
}

static void handleCommand(const char *line) {
  if (!strcmp(line, "PING")) { ready(); return; }
  unsigned seconds = 0;
  char extra;
  if (sscanf(line, "REC %u %c", &seconds, &extra) == 1 &&
      seconds >= 1 && seconds <= MAX_SECONDS) {
    recordAudio(seconds);
  } else {
    Serial.println("ERROR command");
  }
}

void setup() {
  Serial.begin(SERIAL_BAUD);
  // Commands work even if this startup message is missed during USB reconnect.
  delay(300);
  ready();
}

void loop() {
  static char line[40];
  static size_t used = 0;
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      line[used] = '\0';
      if (used) handleCommand(line);
      used = 0;
    } else if (used + 1 < sizeof(line)) {
      line[used++] = c;
    } else {
      used = 0;
    }
  }
  delay(1);
}

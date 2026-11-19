#pragma once
/*
  브라우저 수집기의 USB 연결 규칙만 처리하는 수업용 연결 파일입니다.
  녹음, 마이크 읽기, 비트 변환, WAV 생성은 포함하지 않습니다.
  그 기능은 .ino에서 공식 ESP_I2S 라이브러리를 직접 호출합니다.
*/
#include <Arduino.h>
#include <esp_rom_crc.h>

const char *collectorMicName = "XIAO_PDM";

void collectorReady() {
  Serial.printf("READY_AUDIO1 %s 16000 1\n", collectorMicName);
}

void collectorBegin(const char *micName) {
  collectorMicName = micName;
  Serial.begin(460800);
  delay(300);
  collectorReady();
}

void collectorError(const char *reason) {
  Serial.printf("ERROR %s\n", reason);
}

int collectorReadSeconds() {
  static char line[40];
  static size_t used = 0;
  while (Serial.available()) {
    const char c = Serial.read();
    if (c == '\r') continue;
    if (c == '\n') {
      line[used] = '\0';
      const bool hasCommand = used != 0;
      used = 0;
      if (!hasCommand) continue;
      if (!strcmp(line, "PING")) {
        collectorReady();
        continue;
      }
      unsigned seconds = 0;
      char extra;
      if (sscanf(line, "REC %u %c", &seconds, &extra) == 1 && seconds >= 1 && seconds <= 10) {
        return seconds;
      }
      collectorError("command");
    } else if (used + 1 < sizeof(line)) {
      line[used++] = c;
    } else {
      used = 0;
    }
  }
  return 0;
}

void collectorRecording(int seconds) {
  Serial.printf("RECORDING %d\n", seconds);
}

bool collectorSendWav(const uint8_t *wav, size_t wavBytes) {
  if (wav == nullptr || wavBytes == 0) return false;
  // CRC calculation also uses an Espressif-provided function.
  const uint32_t crc = esp_rom_crc32_le(0, wav, wavBytes);
  Serial.printf("WAV %lu %08lX\n", (unsigned long)wavBytes, (unsigned long)crc);
  size_t sent = 0;
  unsigned long lastProgress = millis();
  while (sent < wavBytes) {
    const size_t remaining = wavBytes - sent;
    const size_t n = Serial.write(wav + sent, remaining > 1024 ? 1024 : remaining);
    if (n > 0) {
      sent += n;
      lastProgress = millis();
    } else if (millis() - lastProgress > 5000) {
      return false;
    }
    delay(1);
  }
  return true;
}

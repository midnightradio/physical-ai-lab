/*
  ESP32-S3-WROOM-1 N16R8 CAM + INMP441 → 기존 브라우저 오디오 수집기
  보드: ESP32S3 Dev Module / esp32 3.3.12 / PSRAM: OPI PSRAM
  INMP441: 3V3, GND, SCK=1, WS=2, SD=42, L/R=GND
  카메라·내장 SD 핀과 겹치지 않으며, 04 SD 예제와 같은 배선입니다.
*/
#include <ESP_I2S.h>
#include "CollectorLink.h"  // USB 연결만 담당하는 파일입니다.

I2SClass microphone;

void setup() {
  collectorBegin("INMP441");
}

void loop() {
  int seconds = collectorReadSeconds();
  if (seconds > 0) {
    recordSound(seconds);
  }
  delay(1);
}

void recordSound(int seconds) {
  if (psramFound() == false) {
    collectorError("memory");
    return;
  }

  microphone.setPins(1, 2, -1, 42);  // BCLK, WS, 미사용 출력, 마이크 입력
  if (microphone.begin(I2S_MODE_STD, 16000, I2S_DATA_BIT_WIDTH_32BIT,
                       I2S_SLOT_MODE_MONO, I2S_STD_SLOT_LEFT) == false) {
    microphone.end();
    collectorError("microphone_init");
    return;
  }
  // INMP441의 32비트 입력을 라이브러리에서 16비트로 바꿉니다.
  if (microphone.configureRX(16000, I2S_DATA_BIT_WIDTH_32BIT,
      I2S_SLOT_MODE_MONO, I2S_RX_TRANSFORM_32_TO_16) == false) {
    microphone.end();
    collectorError("microphone_init");
    return;
  }
  microphone.setTimeout(12000);

  size_t wavBytes = 0;

  // 처음 1초의 예열 녹음은 저장하지 않습니다.
  uint8_t *warmup = microphone.recordWAV(1, &wavBytes);
  if (warmup == nullptr) {
    microphone.end();
    collectorError("microphone_read");
    return;
  }
  free(warmup);

  collectorRecording(seconds);
  uint8_t *wav = microphone.recordWAV(seconds, &wavBytes);  // 실제 녹음
  microphone.end();

  if (wav == nullptr) {
    collectorError("microphone_read");
    return;
  }
  collectorSendWav(wav, wavBytes);
  free(wav);
}

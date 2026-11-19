/*
  XIAO ESP32S3 Sense → 기존 브라우저 오디오 수집기
  공식 ESP_I2S의 recordWAV()로 녹음과 WAV 생성을 처리합니다.
  보드: XIAO_ESP32S3 / esp32 3.3.12 / PSRAM: OPI PSRAM
*/
#include <ESP_I2S.h>
#include "CollectorLink.h"  // USB 연결만 담당하는 파일입니다.

I2SClass microphone;

void setup() {
  collectorBegin("XIAO_PDM");
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

  // 내장 PDM 마이크: CLK=42, DATA=41
  microphone.setPinsPdmRx(42, 41);
  if (microphone.begin(I2S_MODE_PDM_RX, 16000,
                       I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO) == false) {
    microphone.end();
    collectorError("microphone_init");
    return;
  }
  microphone.setTimeout(12000);  // 최대 10초 녹음을 충분히 기다립니다.

  size_t wavBytes = 0;  // WAV 파일 크기가 여기에 기록됩니다.

  // 처음 1초는 시작 잡음을 피하기 위한 예열입니다. 저장하지 않습니다.
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
  collectorSendWav(wav, wavBytes);  // 완성된 WAV를 PC로 보냅니다.
  free(wav);                       // 사용한 메모리를 돌려줍니다.
}

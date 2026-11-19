/*
  수집기 없이 녹음하는 기초 예제: XIAO ESP32S3 Sense + microSD
  공식 ESP_I2S로 WAV를 만들고, 공식 SD 라이브러리로 파일에 씁니다.
  시리얼 모니터 115200에서 r을 보내면 RECORD_SECONDS만큼 녹음합니다.
  보드: XIAO_ESP32S3 / esp32 3.3.12 / PSRAM: OPI PSRAM
*/
#include <ESP_I2S.h>
#include <SD.h>

const int RECORD_SECONDS = 3;  // 녹음 시간입니다. 10으로 바꾸면 10초입니다.

I2SClass microphone;
bool ready = false;
int fileNumber = 1;

void setup() {
  Serial.begin(115200);
  delay(500);
  if (psramFound() == false || SD.begin(21) == false) {
    Serial.println("PSRAM 설정과 FAT32 microSD 카드를 확인하세요.");
    return;
  }
  microphone.setPinsPdmRx(42, 41);
  microphone.setTimeout(12000);
  ready = true;
  Serial.printf("r을 보내면 %d초 녹음하여 SD 카드에 저장합니다.\n", RECORD_SECONDS);
}

void loop() {
  if (ready && Serial.available() && Serial.read() == 'r') {
    recordToSd();
  }
  delay(10);
}

void recordToSd() {
  if (microphone.begin(I2S_MODE_PDM_RX, 16000,
                       I2S_DATA_BIT_WIDTH_16BIT, I2S_SLOT_MODE_MONO) == false) {
    microphone.end();
    Serial.println("마이크 시작에 실패했습니다.");
    return;
  }
  size_t wavBytes = 0;
  Serial.println("마이크 예열 중...");
  uint8_t *warmup = microphone.recordWAV(1, &wavBytes);
  if (warmup == nullptr) {
    microphone.end();
    Serial.println("마이크 읽기에 실패했습니다.");
    return;
  }
  free(warmup);

  Serial.printf("지금부터 %d초 동안 녹음합니다.\n", RECORD_SECONDS);
  uint8_t *wav = microphone.recordWAV(RECORD_SECONDS, &wavBytes);
  microphone.end();
  if (wav == nullptr) {
    Serial.println("녹음에 실패했습니다.");
    return;
  }

  // 기존 파일을 덮어쓰지 않도록 다음 번호를 찾습니다.
  String path = "/recording_" + String(fileNumber) + ".wav";
  while (SD.exists(path)) {
    fileNumber = fileNumber + 1;
    path = "/recording_" + String(fileNumber) + ".wav";
  }
  File file = SD.open(path, FILE_WRITE);
  if (file == false) {
    free(wav);
    Serial.println("파일을 열 수 없습니다.");
    return;
  }
  size_t written = file.write(wav, wavBytes);  // WAV 전체를 파일에 저장
  file.close();
  free(wav);

  if (written == wavBytes) {
    Serial.print("저장 완료: ");
    Serial.println(path);
  } else {
    Serial.println("SD 카드에 저장하는 중 오류가 발생했습니다.");
  }
}
